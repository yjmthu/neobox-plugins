#include <history.h>
#include <fstream>
#include <neobox/systemapi.h>
#include <platform.hpp>

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
  std::ifstream file("wallpaperData/History.txt", std::ios::in);
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
  std::ofstream file("wallpaperData/History.txt", std::ios::out);
  if (!file.is_open())
    return;

  for (auto i = rbegin(); i != rend(); ++i) {
    file << i->string() << std::endl;
    if (!--m_CountLimit)
      break;
  }
  file.close();
}
