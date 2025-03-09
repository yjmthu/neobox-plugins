#include <history.h>
#include <fstream>
#include <neobox/systemapi.h>
#include <platform.hpp>
#include <wallpaper.h>

#ifdef _DEBUG
#include <iostream>

static std::ostream& operator<<(std::ostream& os, const std::u8string& str) {
  return os.write(reinterpret_cast<const char*>(str.data()), str.size());
}

#endif

WallpaperHistory::WallpaperHistory()
  : WallpaperHistoryBase()
{
  ReadSettings();
}

WallpaperHistory::~WallpaperHistory()
{
  WriteSettings();
}

void WallpaperHistory::UpdateRegString()
{
  auto res = WallpaperPlatform::GetWallpaper();

  if (!res) return;
  auto curImage = GetCurrent();
  if (!curImage || *curImage != *res) {
#ifdef _DEBUG
    std::cout << "系统之前壁纸为：" << res->u8string() << std::endl;
#endif
    PushBack(std::move(*res));
  }
}

void WallpaperHistory::ReadSettings()
{
  std::ifstream file(HISTORY_FILE, std::ios::in);
  if (file.is_open()) {
    std::string temp;
    if (std::getline(file, temp)) {
      while (std::getline(file, temp)) {
        emplace_front(temp);
        front().make_preferred();
      }
    }
    file.close();
  }
}

void WallpaperHistory::WriteSettings() {
  int m_CountLimit = 100;
  std::ofstream file(HISTORY_FILE, std::ios::out);
  if (!file.is_open())
    return;

  for (auto i = rbegin(); i != rend(); ++i) {
    file << i->string() << std::endl;
    if (!--m_CountLimit)
      break;
  }
  file.close();
}

void WallpaperHistory::Upgrade(int version)
{
  if (version == 1) {
    #ifdef _WIN32
    auto code = std::error_code();
    std::filesystem::rename(HISTORY_FILE, std::format(HISTORY_FILE ".{}", version - 1), code);
    if (code) {
      std::filesystem::remove(HISTORY_FILE);
    }
    #ifdef _DEBUG
    if (code) {
      std::cout << "备份历史记录文件失败！" << code.message() << std::endl;
    } else {
      std::cout << "备份历史记录文件成功！" << std::endl;
    }
    #endif

    std::filesystem::rename(BLACKLIST_FILE, std::format(BLACKLIST_FILE ".{}", version - 1), code);
    if (code) {
      std::filesystem::remove(BLACKLIST_FILE);
    }
    #ifdef _DEBUG
    if (code) {
      std::cout << "升级历史记录文件失败！" << code.message() << std::endl;
    } else {
      std::cout << "升级历史记录文件成功！" << std::endl;
    }
    #endif
    #endif
  }
}
