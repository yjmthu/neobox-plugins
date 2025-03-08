#include <yjson/yjson.h>
#include <neobox/httplib.h>
#include <neobox/systemapi.h>
#include <neobox/neotimer.h>
#include <neobox/pluginmgr.h>

#include <stdexcept>
#include <download.h>
#include <platform.hpp>
#include <favorie.h>

#include <wallpaper.h>

namespace fs = std::filesystem;
using namespace std::literals;
using Void = Wallpaper::Void;
using Favorite = Wall::Favorite;

// extern std::unordered_set<fs::path> g_UsingFiles;

constexpr char Wallpaper::m_szWallScript[];


Wallpaper::Wallpaper(YJson& settings)
  : m_Settings(settings)
  , m_Config(GetConfigData())
  , m_Wallpaper(WallBase::Initialize(*m_Config))
  , m_Timer(NeoTimer::New())
  , m_Favorites(dynamic_cast<Favorite*>(WallBase::GetInstance(WallBase::FAVORITE)))
  , m_BingWallpaper(WallBase::GetInstance(WallBase::BINGAPI))
{
  ReadBlacklist();
  SetImageType(m_Settings.GetImageType());
  SetTimeInterval(m_Settings.GetTimeInterval());
  SetFirstChange(m_Settings.GetFirstChange());
}

Wallpaper::~Wallpaper() {
  m_Timer->Destroy();

  DownloadJob::ClearPool();
  
  WallBase::Uuinitialize();
}

YJson* Wallpaper::GetConfigData()
{
  YJson* data = nullptr;
  try {
    data = new YJson(WallBase::m_ConfigPath, YJson::UTF8);
  } catch (std::runtime_error error) {
    delete data;
    data = new YJson(YJson::Object);
  }

  return data;
}

Void Wallpaper::SetSlot(OperatorType type) {
  switch (type) {
    case OperatorType::Next:
      co_await SetNext().awaiter();
      break;
    case OperatorType::UNext:
      UnSetNext();
      break;
    case OperatorType::Dislike:
      co_await SetDislike().awaiter();
      break;
    case OperatorType::UDislike:
      UnSetDislike();
      break;
    case OperatorType::Favorite:
      SetFavorite();
      break;
    case OperatorType::UFavorite:
      co_await UnSetFavorite().awaiter();
      break;
    default:
      break;
  }
  // WriteSettings();
}

bool Wallpaper::IsCurImageFavorite() {
  Locker locker(m_DataMutex);
  auto curImage = GetCurIamge();
  if (!curImage) return false;
  return m_Favorites->IsFileFavorite(*curImage);
}

void Wallpaper::SetTimeInterval(int minute) {
  if (m_Settings.GetTimeInterval() != minute) {
    m_Settings.SetTimeInterval(minute);
    // m_Settings.SaveData();
  }
  m_Timer->Expire();
  if (!m_Settings.GetAutoChange())
    return;
  auto m = std::chrono::minutes(minute);
  m_Timer->StartTimer(m, [this] {
    SetSlot(OperatorType::Next).get();
  });
}

std::filesystem::path Wallpaper::Url2Name(const std::u8string& url)
{
  std::filesystem::path imagePath = m_Settings.GetDropDir();
  if (imagePath.is_relative()) {
    imagePath = fs::absolute(imagePath);
    imagePath.make_preferred();
    m_Settings.SetDropDir(imagePath.u8string());
  }
  // }
  auto const iter = url.rfind(u8'/') + 1;
  if (m_Settings.GetDropImgUseUrlName()) {
    // url's separator is the '/'.
    imagePath /= url.substr(iter);
  } else {
    auto const extension = url.rfind(u8'.');                       // all url is img file
    auto utc = std::chrono::system_clock::now();

    auto fmt = Utf82WideString(m_Settings.GetDropNameFmt() + url.substr(extension));
    auto const time = std::chrono::current_zone()->to_local(utc);
    auto const name = Utf82WideString(url.substr(iter, extension - iter));
    auto const fmtedName = std::vformat(fmt, std::make_wformat_args(time, name));
    imagePath.append(fmtedName);
  }

  return imagePath.make_preferred();
}

Void Wallpaper::SetNext() {
  m_DataMutex.lock();
  m_PrevImgs.UpdateRegString();
  auto const bufferEmpty = m_NextImgs.empty();
  m_DataMutex.unlock();

  if (bufferEmpty) {
    auto res = co_await m_Wallpaper->GetNext().awaiter();
    if (res == std::nullopt) {
      mgr->ShowMsgbox(L"出错", L"获取壁纸失败！");
      co_return;
    }
    if (res->ErrorCode != ImageInfo::NoErr) {
      mgr->ShowMsgbox(L"出错", Utf82WideString(res->ErrorMsg));
      co_return;
    }
    // if is bing wallpaper, download it.
    if (m_Wallpaper == m_BingWallpaper) {
      auto cur = m_PrevImgs.GetCurrent();
      fs::path next = res->ImagePath;
      if (cur && fs::equivalent(*cur, next)) {
#ifdef _DEBUG
        std::cout << "bing wallpaper is the same as the current wallpaper." << std::endl;
#endif
        res = co_await m_Wallpaper->GetNext().awaiter();
        if (!res || res->ErrorCode != ImageInfo::NoErr) {
          mgr->ShowMsgbox(L"出错", L"获取必应壁纸失败！");
          co_return;
        }
      }
    }
    co_await PushBack(*res).awaiter();
  } else {
    MoveRight();
  }
}

void Wallpaper::UnSetNext() {
  LockerEx locker(m_DataMutex);
  m_PrevImgs.UpdateRegString();
  auto prev = m_PrevImgs.GetPrevious();
  if (prev) {
    auto cur = m_PrevImgs.GetCurrent();
    if (cur) {
      m_PrevImgs.EraseCurrent();
      m_NextImgs.push(std::move(*cur));
    }
    locker.unlock();
    WallpaperPlatform::SetWallpaper(*prev);
  } else {
    mgr->ShowMsgbox(L"提示", L"当前已经是第一张壁纸！");
  }
}

void Wallpaper::UnSetDislike() {
  LockerEx locker(m_DataMutex);
  m_PrevImgs.UpdateRegString();
  if (m_BlackList.empty())
    return;
  auto back = std::move(m_BlackList.back());
  m_BlackList.pop_back();

  if (!back.second.has_parent_path()) {
    locker.unlock();
    WriteBlackList();
    return;
  }

  auto parent_path = back.second.parent_path();
  std::error_code error;
  if (!fs::exists(parent_path) && !fs::create_directories(parent_path, error)) {
    return;
  }
  fs::copy(back.first, back.second, error);
  fs::remove(back.first, error);

  locker.unlock();
  if (!WallpaperPlatform::SetWallpaper(back.second))
    return;

  locker.lock();
  m_Wallpaper->UndoDislike(back.second.u8string());
  m_PrevImgs.PushBack(std::move(back.second));
  locker.unlock();
  WriteBlackList();
}

void Wallpaper::ClearJunk() {
  const auto junk = mgr->GetJunkDir();
  
  try {
    fs::remove_all(junk);
    if (fs::exists("Blacklist.txt"))
      fs::remove("Blacklist.txt");
  } catch (fs::filesystem_error error) {
    mgr->ShowMsgbox(L"出错",
      std::format(L"无法清除文件！\n错误码：{}。\n错误信息：{}",
        error.code().value(), Ansi2WideString(error.what())));
  }
  
  m_DataMutex.lock();
  m_BlackList.clear();
  m_DataMutex.unlock();
}

void Wallpaper::SetFavorite() {
  Locker locker(m_DataMutex);
  m_PrevImgs.UpdateRegString();
  if (m_Wallpaper != m_Favorites) {
    auto curImage = m_PrevImgs.GetCurrent();
    if (curImage) {
      m_Favorites->UndoDislike(curImage->u8string());
    }
  }
}

Void Wallpaper::UnSetFavorite() {
  LockerEx locker(m_DataMutex);
  m_PrevImgs.UpdateRegString();
  if (m_Wallpaper != m_Favorites) {
    auto curImage = m_PrevImgs.GetCurrent();
    if (curImage) {
      m_Favorites->Dislike(curImage->u8string());
    }
  } else {
    locker.unlock();
    co_await SetDislike().awaiter();
  }
}

Void Wallpaper::SetDropFile(std::queue<std::u8string_view> urls) {

  while (!urls.empty()) {
    std::u8string url(urls.front());
    urls.pop();
    if (!DownloadJob::IsImageFile(url)) {
      continue;
    }
    auto imageName = Url2Name(url);
    if (url.starts_with(u8"http")) {
      ImageInfo ptr {
        .ImagePath = imageName.u8string(),
        .ImageUrl = url,
        .ErrorCode = ImageInfo::Errors::NoErr,
      };
      co_await PushBack(ptr).awaiter();
    } else if (fs::path oldName = url; fs::exists(oldName)) {
      oldName.make_preferred();

      if (oldName.parent_path() != imageName.parent_path()) {
        imageName = std::move(oldName);
      } else {                     // don't move wallpaper to the same directory.
        fs::copy(oldName, imageName);
      }
      if (WallpaperPlatform::SetWallpaper(imageName)) {
        m_DataMutex.lock();
        m_PrevImgs.PushBack(std::move(imageName));
        m_DataMutex.unlock();
      }
    }
  }
}

Void Wallpaper::PushBack(const ImageInfo& ptr)
{
  auto imagePath = ptr.ImagePath;
  auto err = co_await DownloadJob::DownloadImage(ptr).awaiter();
  
  if (!err || *err != DownloadJob::Error::NoError) {
    co_return;
  }
  
  if (!WallpaperPlatform::SetWallpaper(imagePath))
    co_return;

  Locker locker(m_DataMutex);
  m_PrevImgs.PushBack(imagePath);
}

bool Wallpaper::MoveRight() {
  LockerEx locker(m_DataMutex);
  fs::path next { std::move(m_NextImgs.top()) };
  m_NextImgs.pop();
  locker.unlock();
  if (!WallpaperPlatform::SetWallpaper(next)) {
    return false;
  }
  locker.lock();
  m_PrevImgs.PushBack(std::move(next));
  return true;
}

Void Wallpaper::SetDislike() {
  m_DataMutex.lock();
  m_PrevImgs.UpdateRegString();
  auto const ok = m_NextImgs.empty();
  m_DataMutex.unlock();

  auto callback = [this] {
    LockerEx locker(m_DataMutex);
    // 取出之前的图片栈顶元素，并将其设置为dislike
    auto curImage = m_PrevImgs.GetPrevious();

    if (!curImage) return ;
    m_PrevImgs.ErasePrevious();
    m_Wallpaper->Dislike(curImage->u8string());
    if (fs::exists(*curImage)) {
      locker.unlock();
      AppendBlackList(*curImage);
      locker.lock();
    }
  };

  if (ok) {
    // 设置一张新的壁纸, 并把之前的壁纸设置为dislike
    auto res = co_await m_Wallpaper->GetNext().awaiter();
    if (res == std::nullopt) co_return;
    if (res->ErrorCode != ImageInfo::NoErr)
      co_return;
    co_await PushBack(*res).awaiter();
  } else {
    MoveRight();
  }
  callback();
}

void Wallpaper::ReadBlacklist() {
  std::ifstream file("wallpaperData/Blacklist.txt", std::ios::in);
  if (file.is_open()) {
    std::string key, val;
    while (std::getline(file, key) && std::getline(file, val)) {
      m_BlackList.emplace_back(key, val);
    }
    file.close();
  }
}

void Wallpaper::AppendBlackList(const fs::path& path) {
  const auto junk = mgr->GetJunkDir();

  Locker locker(m_DataMutex);
  auto& back = m_BlackList.emplace_back(junk / path.filename(), path);
  fs::rename(back.second, back.first);

  std::ofstream file("wallpaperData/Blacklist.txt", std::ios::app);
  if (!file.is_open())
    return;
  file << back.first.string() << std::endl << back.second.string() << std::endl;
  file.close();
}

void Wallpaper::WriteBlackList() {
  std::ofstream file("wallpaperData/Blacklist.txt", std::ios::out);
  if (!file.is_open())
    return;
  
  Locker locker(m_DataMutex);
  for (auto& i : m_BlackList) {
    file << i.first.string() << std::endl << i.second.string() << std::endl;
  }
  file.close();
}

void Wallpaper::SetAutoChange(bool flag) {
  m_Timer->Expire();

  Locker locker(m_DataMutex);
  m_Settings.SetAutoChange(flag);
  // m_Settings.SaveData();
  if (flag) {
    m_Timer->StartTimer(
      std::chrono::minutes(m_Settings.GetTimeInterval()), [this] {
        SetSlot(OperatorType::Next).get();
      });
  }
}

void Wallpaper::SetFirstChange(bool flag) {
  m_DataMutex.lock();
  m_Settings.SetFirstChange(flag);
  m_DataMutex.unlock();

  if (flag) {
    auto const timer = NeoTimer::New();
    timer->StartTimer(15s, [this, timer](){
      SetNext().get();
      // 不阻塞工作线程才能顺利析构
      std::thread([timer](){ timer->Destroy(); }).detach();
    });
  }
}

bool Wallpaper::SetImageType(int index) {
  if (m_Settings.GetImageType() != index) {
    m_Settings.SetImageType(index);
    // m_Settings.SaveData();
  }

  m_Wallpaper = WallBase::GetInstance(index);
  return true;
}

// attention: thread maybe working!
