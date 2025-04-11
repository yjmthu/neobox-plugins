#include <bingapi.h>
#include <neobox/httplib.h>
#include <stdexcept>
#include <wallpaper.h>
#include <wallbase.h>
#include <neobox/unicode.h>
#include <download.h>
#include <neobox/neotimer.h>

#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
namespace chrono = std::chrono;
using namespace std::literals;
using namespace Wall;


BingApi::BingApi(YJson& setting)
  : WallBase(InitSetting(setting))
  , m_Data { nullptr }
  , m_Timer(NeoTimer::New())
{
  InitData();
  AutoDownload();
}

BingApi::~BingApi()
{
  m_QuitFlag = true;
  m_Timer->Destroy();
}

YJson& BingApi::InitSetting(YJson& setting) {
  if (setting.isObject()) {
    auto& copyrightlink = setting[u8"copyrightlink"];
    if (!copyrightlink.isString())
      copyrightlink = YJson::String;
  } else {
    auto const initDir = GetStantardDir(m_Name);
    setting = YJson::O {
      { u8"api", u8"https://global.bing.com"},
      { u8"curday"sv,    GetToday() },
      { u8"directory"sv,  initDir },
      { u8"name-format"sv, u8"{0:%Y-%m-%d} {1}.jpg" },
      { u8"region"sv, u8"zh-CN" },
      { u8"auto-download"sv, false },
      { u8"copyrightlink"sv, YJson::String}
    };
  }
  auto& version = setting[u8"version"];
  if (!version.isNumber()) {
    version = 0;
    setting[u8"name-format"] = u8"{0:%Y-%m-%d} {1}.jpg";
  }
  return setting;
}

void BingApi::InitData()
{
  if (m_Setting[u8"curday"].getValueString() != GetToday()) {
    return;
  }
  try {
    m_Data = std::make_unique<YJson>(m_DataPath, YJson::UTF8);
  } catch (std::runtime_error error[[maybe_unused]]) {
#ifdef _DEBUG
    std::cerr << error.what() << std::endl;
#endif
  }
}

Bool BingApi::CheckData()
{
  // https://cn.bing.com/HPImageArchive.aspx?format=js&idx=0&n=8

  m_DataMutex.lock();
  auto const today = GetToday();
  if (m_Setting[u8"curday"].getValueString() != today) {
    m_Data.reset();
  }
  HttpUrl url(m_Setting[u8"api"].getValueString() + u8"/HPImageArchive.aspx?", {
    {u8"format", u8"js"},
    {u8"idx", u8"0"},
    {u8"n", u8"8"},
    {u8"mkt", m_Setting[u8"region"].getValueString()},
  });
  m_DataMutex.unlock();

  if (m_Data) co_return true;

  m_DataRequest = std::make_unique<HttpLib>(url, true);
  
  auto res = co_await m_DataRequest->GetAsync();
  if (res->status / 100 == 2) {
    m_DataMutex.lock();
    m_Data =  std::make_unique<YJson>(res->body.begin(), res->body.end());
    m_Data->toFile(m_DataPath);

    m_Setting[u8"curday"] = today;
    SaveSetting();
    m_DataMutex.unlock();
    co_return true;
  }

  co_return false;
}

ImageInfoX BingApi::GetNext() {
  static size_t s_uCurImgIndex = 0;

  // https://www.bing.com/th?id=OHR.Yellowstone150_ZH-CN055
  // 下这个接口含义，直接看后面的请求参数1084440_UHD.jpg

  auto result = co_await CheckData().awaiter();
  if (!result || !*result) {
    co_return {
      .ErrorMsg = "Bad network connection.",
      .ErrorCode = ImageInfo::NetErr
    };
  }

  Locker locker(m_DataMutex);
  const fs::path imgDir = m_Setting[u8"directory"].getValueString();
  auto& imgInfo = m_Data->find(u8"images")->second[s_uCurImgIndex];
  m_Setting[u8"copyrightlink"] = imgInfo[u8"copyrightlink"];
  SaveSetting();

  ++s_uCurImgIndex &= 0b0111;
  co_return {
    .ImagePath = (imgDir / GetImageName(imgInfo)).u8string(),
    .ImageUrl = m_Setting[u8"api"].getValueString() + imgInfo[u8"urlbase"].getValueString() + u8"_UHD.jpg",
    .ErrorCode = ImageInfo::NoErr
  };
}

void BingApi::SetJson(const YJson& json)
{
  WallBase::SetJson(json);

  m_DataMutex.lock();
  m_Data.reset();
  if (!GetAutoDownload()) { m_Timer->Expire(); }
  m_DataMutex.unlock();
  return;
}

bool BingApi::GetAutoDownload() const {
  return m_Setting[u8"auto-download"].isTrue();
}

void BingApi::AutoDownload() {
  Locker locker(m_DataMutex);
  if (!GetAutoDownload()) return;

  m_Timer->StartOnce(30s, [this] {
    auto const dataObtained = CheckData().get();
    if (!dataObtained || !*dataObtained) return;

    Locker locker(m_DataMutex);
    const fs::path imgDir = m_Setting[u8"directory"].getValueString();
    for (auto& item : m_Data->find(u8"images")->second.getArray()) {
      ImageInfo ptr {
        .ImagePath = (imgDir / GetImageName(item)).u8string(),
        .ImageUrl = m_Setting[u8"api"].getValueString() +
          item[u8"urlbase"].getValueString() + u8"_UHD.jpg",
        .ErrorCode = ImageInfo::NoErr
      };
      // ------------------------------------ //
      DownloadJob::DownloadImage(ptr).get();
    }
  });
}

std::u8string BingApi::GetToday() {
  auto utc = chrono::system_clock::now();
  std::string result =
      std::format("{:%Y-%m-%d}", chrono::current_zone()->to_local(utc));
  return std::u8string(result.begin(), result.end());
}

std::u8string BingApi::GetImageName(YJson& imgInfo) {
  // see https://codereview.stackexchange.com/questions/156695/converting-stdchronotime-point-to-from-stdstring
  const auto& fmt = m_Setting[u8"name-format"].getValueString();
  const std::string date(Utf8AsString(imgInfo[u8"enddate"].getValueString()));
  const std::u8string& copyright =
      imgInfo[u8"copyright"].getValueString();
  const std::u8string& title = imgInfo[u8"title"].getValueString();

  std::tm tm {};
  std::istringstream(date) >> std::get_time(&tm, "%Y%m%d");
  auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  auto timePoint = std::chrono::zoned_time(chrono::current_zone(), tp);

  auto titleUnicode = reinterpret_cast<const std::string&>(title);
  for (auto& c: titleUnicode) {
    if ("/\\;:"sv.find(c) != std::string::npos) {
      c = '_';
    }
  }
  auto const & copyrightUnicode = reinterpret_cast<const std::string&>(copyright);
  auto result = std::vformat(reinterpret_cast<const std::string&>(fmt), std::make_format_args(timePoint, titleUnicode, copyrightUnicode));

  return reinterpret_cast<std::u8string&>(result);
}
