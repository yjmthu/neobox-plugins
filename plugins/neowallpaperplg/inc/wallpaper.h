#pragma once

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>
#include <functional>

#include <wallbase.h>
#include <wallconfig.h>
#include <history.h>

namespace fs = std::filesystem;

enum class OperatorType {
  Next, UNext, Dislike, UDislike, Favorite, UFavorite
};

class Wallpaper {
public:
  using Locker = WallBase::Locker;
  using LockerEx = WallBase::LockerEx;
private:
  fs::path Url2Name(const std::u8string& url);
  void AppendBlackList(const fs::path& path);
  void WriteBlackList();
  void PushBack(ImageInfoEx ptr,
    std::optional<std::function<void()>> callback);
  bool MoveRight();
  static YJson* GetConfigData();
private:
  void ReadBlacklist();
  void SetNext();
  void UnSetNext();
  void SetDislike();
  void UnSetDislike();
  void SetFavorite();
  void UnSetFavorite();

public:
  void SetDropFile(std::queue<std::u8string_view> url);
  void SetSlot(OperatorType type);
  bool SetImageType(int type);
  void SetFirstChange(bool val);
  void SetTimeInterval(int minute);
  void SetAutoChange(bool val);
  void ClearJunk();
  std::optional<fs::path> GetCurIamge() {
    m_PrevImgs.UpdateRegString();
    return m_PrevImgs.GetCurrent();
  }
  WallBase* Engine() { return m_Wallpaper; }

public:
  WallConfig m_Settings;

private:
  static constexpr char m_szWallScript[16]{"SetWallpaper.sh"};

public:
  explicit Wallpaper(class YJson& settings);
  virtual ~Wallpaper();

private:
  YJson* const m_Config;
  std::mutex m_DataMutex;

  class NeoTimer* const m_Timer;
  class WallBase* m_Wallpaper;
  class WallBase* const m_Favorites;
  class WallBase* const m_BingWallpaper;
  WallpaperHistory m_PrevImgs;
  std::stack<fs::path> m_NextImgs;
  std::list<std::pair<fs::path, fs::path>> m_BlackList;
};
