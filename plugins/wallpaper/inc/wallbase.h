#ifndef APICLASS_H
#define APICLASS_H

#include <neobox/httplib.h>
#include <yjson/yjson.h>

#include <atomic>
#include <filesystem>
#include <string>
#include <functional>
#include <mutex>

namespace fs =std::filesystem;

struct ImageInfo {
  enum Errors : uint32_t { NoErr, NetErr, FileErr, RunErr, CfgErr, DataErr };
  std::u8string ImagePath;
  std::u8string ImageUrl;
  std::u8string ErrorMsg;
  uint32_t ErrorCode;
};

class WallBase {
public:
  typedef std::lock_guard<std::mutex> Locker;
  typedef std::unique_lock<std::mutex> LockerEx;
protected:
  static std::mutex m_DataMutex;
  static const fs::path m_DataDir;
  static std::function<void()> SaveSetting;
  typedef HttpAction<void> Void;
  typedef HttpAction<bool> Bool;
  typedef HttpAction<int> Int;

  // 获取用户图片目录，并返回下面的“桌面壁纸”目录
  static fs::path GetHomePicLocation();

  // 获取“桌面壁纸”目录下面的“name”目录，桌面壁纸由 GetHomePicLocation 返回
  static std::u8string GetStantardDir(const std::u8string& name);

  static std::string_view Utf8AsString(std::u8string_view str)
  { return std::string_view(reinterpret_cast<const char*>(str.data()), str.size()); };

  static std::u8string_view StringAsUtf8(std::string_view str)
  { return std::u8string_view(reinterpret_cast<const char8_t *>(str.data()), str.size()); };

public:
  enum { WALLHAVEN = 0, BINGAPI, DIRECTAPI, NATIVE, SCRIPTOUTPUT, FAVORITE, NONE };
  static WallBase* GetInstance(uint32_t type);
  explicit WallBase(YJson& setting):
    m_Setting(setting)
    {
      fs::create_directory(m_DataDir);
    }
  virtual ~WallBase() {}
  static std::nullptr_t Initialize(YJson& setting);
  static void Uuinitialize();
  static const fs::path m_ConfigPath;
  static std::atomic_bool m_QuitFlag;
  YJson& m_Setting;

public:
  virtual HttpAction<ImageInfo> GetNext() = 0;
  virtual void Dislike(std::u8string_view sImgPath);
  virtual void UndoDislike(std::u8string_view sImgPath);
  virtual void SetJson(const YJson& json);
  YJson GetJson() const;

 private:
  static std::array<WallBase*, NONE> m_Instances;
  friend class Wallpaper;
};

#endif
