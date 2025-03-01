#pragma once

#include <filesystem>
#include <list>
#include <queue>
#include <stack>
#include <string>

#include <wallbase.h>
#include <wallconfig.h>
#include <history.h>

namespace fs = std::filesystem;

#define NO_IGNORE_WARNING "The return value of the coroutine is not used."

enum class OperatorType {
  Next, UNext, Dislike, UDislike, Favorite, UFavorite
};

class Wallpaper {
public:
  using Locker = WallBase::Locker;
  using LockerEx = WallBase::LockerEx;
  using Void = HttpAction<void>;
private:
  fs::path Url2Name(const std::u8string& url);
  void AppendBlackList(const fs::path& path);
  void WriteBlackList();
  HttpAction<void> PushBack(const ImageInfo& ptr);
  bool MoveRight();
  static YJson* GetConfigData();
private:
  void ReadBlacklist();
  [[nodiscard(NO_IGNORE_WARNING)]] Void SetNext();
  void UnSetNext();
  [[nodiscard(NO_IGNORE_WARNING)]] Void SetDislike();
  void UnSetDislike();
  void SetFavorite();
  [[nodiscard(NO_IGNORE_WARNING)]] Void UnSetFavorite();

public:
  [[nodiscard(NO_IGNORE_WARNING)]] Void SetDropFile(std::queue<std::u8string_view> url);
  [[nodiscard(NO_IGNORE_WARNING)]] Void SetSlot(OperatorType type);
  bool IsCurImageFavorite();
  bool SetImageType(int type);
  void SetFirstChange(bool val);
  void SetTimeInterval(int minute);
  void SetAutoChange(bool val);
  void ClearJunk();
  auto GetCurIamge() {
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
  class Favorite* const m_Favorites;
  class WallBase* const m_BingWallpaper;
  WallpaperHistory m_PrevImgs;
  std::stack<fs::path> m_NextImgs;
  std::list<std::pair<fs::path, fs::path>> m_BlackList;
};
