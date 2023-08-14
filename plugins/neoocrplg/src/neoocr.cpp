#include <systemapi.h>
#include <neoocr.h>
#include <yjson.h>
#include <httplib.h>
#include <pluginmgr.h>
#include <ocrconfig.h>

#include <ranges>
#include <set>
#include <regex>
#include <mutex>
#include <thread>

#include <QByteArray>
#include <QImage>
#include <QBuffer>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <leptonica/pix_internal.h>

std::unique_ptr<Pix, void(*)(Pix*)> QImage2Pix(const QImage& qImage);
namespace fs = std::filesystem;
static std::mutex s_ThreadMutex;

#ifdef _WIN32
#include <Unknwn.h>
#include <MemoryBuffer.h>

#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/windows.Graphics.imaging.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Media.Speechrecognition.h>

// #include <wrl/client.h>

#include <Windows.h>

#pragma comment(lib, "pathcch")
#pragma comment(lib, "windowsapp.lib")

using Windows::Foundation::IMemoryBufferByteAccess;
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Globalization;
namespace WinOcr = winrt::Windows::Media::Ocr;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Media::SpeechRecognition;
#else

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

#endif

NeoOcr::NeoOcr(OcrConfig& settings)
  : m_Settings(settings)
  , m_TessApi(new tesseract::TessBaseAPI)
  , m_TrainedDataDir(fs::path(m_Settings.GetTessdataDir()).string())
{
  InitLanguagesList();
  // winrt::init_apartment();
}

NeoOcr::~NeoOcr()
{
#ifdef __linux__
  delete m_TessApi;
#endif
  std::lock_guard<std::mutex> locker(s_ThreadMutex);
}

#ifdef _WIN32
static auto OpenImageFile(std::wstring uriImage) {
  // auto const& streamRef = RandomAccessStreamReference::CreateFromFile(StorageFile::GetFileFromPathAsync(uriImage).get());
  auto file = StorageFile::GetFileFromPathAsync(uriImage).get();
  auto stream = file.OpenAsync(FileAccessMode::Read).get();
  auto const& decoder = BitmapDecoder::CreateAsync(stream);
  auto softwareBitmap = decoder.get().GetSoftwareBitmapAsync();
  return softwareBitmap;
}

IAsyncAction SaveSoftwareBitmapToFile(SoftwareBitmap& softwareBitmap)
{
  StorageFolder currentfolder = co_await StorageFolder::GetFolderFromPathAsync(std::filesystem::current_path().wstring());
  StorageFile outimagefile = co_await currentfolder.CreateFileAsync(L"NEOOCR.jpg", CreationCollisionOption::ReplaceExisting);
  IRandomAccessStream writestream = co_await outimagefile.OpenAsync(FileAccessMode::ReadWrite);
  BitmapEncoder encoder = co_await BitmapEncoder::CreateAsync(BitmapEncoder::JpegEncoderId(), writestream);
  encoder.SetSoftwareBitmap(softwareBitmap);
  encoder.FlushAsync();
  writestream.Close();
}
#endif

std::u8string NeoOcr::GetText(QImage image)
{
  using enum Engine;
  const auto engine = static_cast<Engine>(m_Settings.GetOcrEngine());
  switch (engine) {
  case Windows: {
    if (image.format() != QImage::Format_RGB32) {
      image = image.convertToFormat(QImage::Format_RGB32);
    }
    return OcrWindows(image);
  }
  case Tesseract:
    return OcrTesseract(image);
  default:
    return u8"~未知服务器~";
  }
}

std::vector<OcrResult> NeoOcr::GetTextEx(const QImage& image)
{
  std::vector<OcrResult> result;
  if (m_Languages.empty()) {
    mgr->ShowMsgbox(L"error", L"You should set some language first!");
//    m_TessApi->End();
    return result;
  }
  if (m_TessApi->Init(m_TrainedDataDir.c_str(), reinterpret_cast<const char*>(m_Languages.c_str()))) {
    mgr->ShowMsgbox(L"error", L"Could not initialize tesseract.");
    return result;
  }

  auto const pix = QImage2Pix(image);
  m_TessApi->SetImage(pix.get());

  // RIL_TEXTLINE 表示识别文本行
  const auto boxes = m_TessApi->GetComponentImages(tesseract::RIL_WORD, true, NULL, NULL);

  for (int i = 0; i != boxes->n; ++i) {
    auto box = boxaGetBox(boxes, i, L_CLONE);
    m_TessApi->SetRectangle(box->x - 1, box->y - 1, box->w + 2, box->h + 2);
    char* ocrResult = m_TessApi->GetUTF8Text();
    int conf = m_TessApi->MeanTextConf();
    result.push_back(OcrResult {
      .text = reinterpret_cast<char8_t*>(ocrResult),
      .x = box->x,
      .y = box->y,
      .w = box->w,
      .h = box->h,
      .confidence = conf,
    });
    boxDestroy(&box);
    delete[] ocrResult;
  }

  m_TessApi->End();
  return result;
}

#ifdef _WIN32
IAsyncAction WinOcrGet(std::wstring name, SoftwareBitmap& softwareBitmap, std::wstring& result) {
  // co_await SaveSoftwareBitmapToFile(softwareBitmap);

  std::function engine = WinOcr::OcrEngine::TryCreateFromUserProfileLanguages;
  if (name != L"user-Profile") {
    const Language lan(name);
    if (WinOcr::OcrEngine::IsLanguageSupported(lan)) {
      engine = std::bind(&WinOcr::OcrEngine::TryCreateFromLanguage, lan);
    }
  }

  auto ocrResult = co_await engine().RecognizeAsync(softwareBitmap);

  hstring back;
  static constexpr auto notZhCN = [](const hstring& str) {
    return str.size() == 1 && str.front() < 0x80;
  };
  for (const auto& line : ocrResult.Lines()) {
    back.clear();
    for (const auto& word: line.Words()) {
      hstring text = word.Text();
      if (!back.empty() && (notZhCN(back) || notZhCN(text))) {
        result.push_back(L' ');
      }
      result += text;
      back = std::move(text);
    }
    result.push_back(L'\n');
  }
}
#endif

std::u8string NeoOcr::OcrWindows(const QImage& image)
{
#ifdef _WIN32
  SoftwareBitmap softwareBitmap(
      BitmapPixelFormat::Bgra8,
      image.width(), image.height()
  );
  {
    auto buffer = softwareBitmap.LockBuffer(BitmapBufferAccessMode::Write);
    auto reference = buffer.CreateReference();
    auto access = reference.as<IMemoryBufferByteAccess>();
    unsigned char* pPixelData = nullptr;
    unsigned capacity = 0;
    // access->GetBuffer(&pPixelData, &capacity);
    winrt::check_hresult(access->GetBuffer(&pPixelData, &capacity));
    // 获取第0帧图片
    auto bufferLayout = buffer.GetPlaneDescription(0);

    pPixelData += bufferLayout.StartIndex;
    auto plane = reinterpret_cast<uint32_t*>(pPixelData);
#if 1
    std::copy_n(reinterpret_cast<const uint32_t*>(image.bits()),
      bufferLayout.Width * bufferLayout.Height, plane);
#else
    for (int i = 0; i < bufferLayout.Height; ++i) {
      auto line = reinterpret_cast<const uint32_t*>(image.scanLine(i));
      for (int j=0; j != bufferLayout.Width; ++j) {
        plane[j] = line[j];
      }
      plane += bufferLayout.Width;
    }
#endif

    reference.Close();
    buffer.Close();

  }


  std::wstring result;
  std::thread thread([&](){
    // 不知道为啥必须要在另外一个线程里面才能正常运行
    WinOcrGet(Utf82WideString(m_Settings.GetWinLan()), softwareBitmap, result).get();
  });
  thread.join();
  return Wide2Utf8String(result);
#else
  return u8"当前平台不支持Windows引擎。";
#endif
}

std::u8string NeoOcr::OcrTesseract(const QImage& image)
{
  static const int charSizeTable[16] {
    // 单字节字符 0b0000~0b1000   0b0xxx
    1, 1, 1, 1, 1, 1, 1, 1,
    // 尾随字符   0b1000~0b1011   0b10xx
    0, 0, 0, 0,
    // 双字节字符 0b1100~0b1101   0b110x
    2, 2,
    // 三字节字符 0b1110
    3,
    // 四字节字符 0b11110
    4,
  };
  std::u8string result;
  if (m_Languages.empty()) {
    mgr->ShowMsgbox(L"error", L"You should set some language first!");
//    m_TessApi->End();
    return result;
  }
  if (m_TessApi->Init(m_TrainedDataDir.c_str(), reinterpret_cast<const char*>(m_Languages.c_str()))) {
    mgr->ShowMsgbox(L"error", L"Could not initialize tesseract.");
    return result;
  }
  auto const pix = QImage2Pix(image);
  m_TessApi->SetImage(pix.get());
  char* szText = m_TessApi->GetUTF8Text();
  std::u8string_view view = reinterpret_cast<const char8_t*>(szText);
  result.reserve(view.size());
  // 过滤掉多字节字符之间的空格，例如中文之间的空格
  for (auto ptr = view.cbegin(); ptr != view.cend(); ) {
    auto size = charSizeTable[*ptr >> 4];
    result.append(ptr, ptr + size);
    ptr += size;
    if (size > 1 && ptr != view.cend() && *ptr == ' ') {
      if (++ptr == view.cend())
        break;
      if (charSizeTable[*ptr >> 4] == 1) {
        result.push_back(' ');
      }
    }
  }
  m_TessApi->End();
  delete [] szText;
  return result;
}

#ifdef _WIN32
std::vector<std::pair<std::wstring, std::wstring>> NeoOcr::GetLanguages()
{
  auto languages = WinOcr::OcrEngine::AvailableRecognizerLanguages();
  std::vector<std::pair<std::wstring, std::wstring>> result = {
    {L"user-Profile", L"用户语言"}
  };
  for (const auto& language : languages) {
    result.emplace_back(decltype(result)::value_type {
      language.LanguageTag(), language.NativeName(),
    });
  }

  return result;
}
#endif

void NeoOcr::InitLanguagesList()
{
  m_Languages.clear();

  const auto array = m_Settings.GetLanguages();

  if (array.empty()) {
    return;
  }

  m_Languages = std::accumulate(
    std::next(array.begin()), array.end(),
    array.front().getValueString(),
    [](const std::u8string& a, const YJson& b){
      return a + u8"+" + b.getValueString();
    }
  );
}

void NeoOcr::DownloadFile(std::u8string_view url, const fs::path& path)
{
  HttpLib clt(url);
  clt.Get(path);
}

void NeoOcr::SetDropData(std::queue<std::u8string_view>& data)
{
  auto const folder = fs::path(m_Settings.GetTessdataDir());

  std::vector<std::u8string> urls;
  while (!data.empty()) {
    auto str(data.front());
    data.pop();
    if (str.starts_with(u8"http")) {
      urls.emplace_back(str);
    } else {
      fs::path path = str;
      fs::copy(str, folder / path.filename());
    }
  }
  if (!urls.empty()) {
    std::thread([urls, folder](){
      std::lock_guard<std::mutex> locker(s_ThreadMutex);
      for (auto& url: urls) {
        auto pos = url.rfind(u8'/');
        if (pos == url.npos) {
          continue;
        }
        HttpLib(HttpUrl(url)).Get(folder / url.substr(pos + 1));
      }
    }).detach();
  }
}

std::u8string NeoOcr::GetLanguageName(const std::u8string& url)
{
  auto iter = std::find(url.crbegin(), url.crend(), u8'/').base();
  return std::u8string(iter, url.end() - 12);
}

void NeoOcr::AddLanguages(const std::vector<std::u8string> &urls)
{
  auto langsArray = m_Settings.GetLanguages();
  auto langsView = langsArray | std::views::transform([](const YJson& item){ return item.getValueString(); });
  std::set<std::u8string> langsSet(langsView.begin(), langsView.end());
  for (const auto& i: urls) {
    if (!i.ends_with(u8".traineddata")) {
      continue;
    }
    auto name = GetLanguageName(i);
    fs::path path = m_Settings.GetTessdataDir();
    path /= name + u8".traineddata";
    if (langsSet.find(name) == langsSet.end()) {
      langsSet.insert(name);
    }
    if (i.starts_with(u8"http")) {
      DownloadFile(i, path);
    } else if (fs::exists(i)) {
      fs::copy(i, path);
    }
  }
  langsArray.assign(langsView.begin(), langsView.end());
  m_Settings.SetLanguages(langsArray);
  InitLanguagesList();
}

void NeoOcr::RmoveLanguages(const std::vector<std::u8string> &names)
{
  YJson jsLangArray = m_Settings.GetLanguages();
  for (const auto& i: names) {
    jsLangArray.removeByValA(i);
  }
  m_Settings.SetLanguages(std::move(jsLangArray.getArray()));
  InitLanguagesList();
}

void NeoOcr::SetDataDir(const std::u8string &dirname)
{
  fs::path path(dirname);
  path.make_preferred();
  m_Settings.SetTessdataDir(path.u8string());
}

static inline bool IsBigDuan() {
  const uint16_t s = 1;
  return *reinterpret_cast<const uint8_t*>(&s);
}

std::unique_ptr<Pix, void(*)(Pix*)> QImage2Pix(const QImage& qImage) {
  static const bool bIsBigDuan = IsBigDuan();
  if (qImage.isNull())
    return std::unique_ptr<Pix, void(*)(Pix*)>(nullptr, [](Pix* pix){pixDestroy(&pix);});
  const int width = qImage.width(), height = qImage.height();
  const int depth = qImage.depth(), bytePerLine = qImage.bytesPerLine();
  PIX* pix = pixCreate(width, height, depth);

  if (qImage.colorCount()) {
    PIXCMAP* map = pixcmapCreate(8);
    if (bIsBigDuan) {  // b g r a
      for (const auto& i : qImage.colorTable()) {
        auto cols = reinterpret_cast<const uchar*>(&i);
        pixcmapAddColor(map, cols[2], cols[1], cols[0]);
      }
    } else {  // a r g b
      for (const auto& i : qImage.colorTable()) {
        auto cols = reinterpret_cast<const uchar*>(&i);
        pixcmapAddColor(map, cols[1], cols[2], cols[3]);
      }
    }
    pixSetColormap(pix, map);
  }

  auto start = pixGetData(pix);
  auto wpld = pixGetWpl(pix);

  switch (qImage.format()) {
    case QImage::Format_Mono:
    case QImage::Format_Indexed8:
    case QImage::Format_RGB888:
      for (int i = 0; i < height; ++i) {
        std::copy_n(qImage.scanLine(i), bytePerLine,
                    reinterpret_cast<uchar*>(start + wpld * i));
      }
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      for (int i = 0; i < height; ++i) {
        auto lines = qImage.scanLine(i);
        l_uint32* lined = start + wpld * i;
        if (bIsBigDuan) {
          for (int j = 0; j < width; ++j, lines += 4) {
            l_uint32 pixel;
            composeRGBPixel(lines[2], lines[1], lines[0], &pixel);
            lined[j] = pixel;
          }
        } else {
          for (int j = 0; j < width; ++j, lines += 4) {
            l_uint32 pixel;
            composeRGBPixel(lines[1], lines[2], lines[3], &pixel);
            lined[j] = pixel;
          }
        }
      }
      break;
    default:
      break;
  }
    return std::unique_ptr<Pix, void(*)(Pix*)>(pix, [](Pix* pix){pixDestroy(&pix);});
}
