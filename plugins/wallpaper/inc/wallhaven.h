#include <neobox/httplib.h>
#include <wallbase.h>
#include <array>

namespace Wall {

class WallhavenData {
public:
  typedef std::function<void()> Callback;
public:
  explicit WallhavenData(Locker::mutex_type& mutex,
    fs::path path)
    : m_DataPath(std::move(path))
    , m_Mutex(mutex)
    , m_Data(InitData())
    , m_ApiUrl(m_Data[u8"Api"].getValueString())
    , m_Used(m_Data[u8"Used"].getArray())
    , m_Unused(m_Data[u8"Unused"].getArray())
    , m_Blacklist(m_Data[u8"Blacklist"].getArray())
  {}
private:
  typedef std::array<int, 2> Range;
  const fs::path m_DataPath;
  YJson m_Data;
  Locker::mutex_type& m_Mutex;
  YJson InitData();
  void HandleResult(std::vector<std::u8string>& array, const YJson& data);
  Bool DownloadAll(const Range& range);
public:
  std::u8string& m_ApiUrl;
  YJson::ArrayType& m_Used;
  YJson::ArrayType& m_Unused;
  YJson::ArrayType& m_Blacklist;
  bool IsEmpty() const;
  void ClearAll();
  Bool DownloadUrl(Range range);
  void SaveData();
};

class Wallhaven : public WallBase {
 private:
  static bool IsPngFile(std::u8string& str);
 public:
  ImageInfoX GetNext() override;
  void Dislike(std::u8string_view sImgPath) override;
  void UndoDislike(std::u8string_view sImgPath) override;
public:
  explicit Wallhaven(YJson& setting);
  virtual ~Wallhaven();
  void SetJson(const YJson& json) override;
  inline static const auto m_Name = u8"壁纸天堂"s;

private:
  YJson& InitSetting(YJson& setting);
  YJson& GetCurInfo();
  YJson& GetCurInfo() const
  { return const_cast<Wallhaven*>(this)->GetCurInfo(); };

  std::string IsWallhavenFile(std::string name);
  Bool CheckData();
  std::u8string GetApiPathUrl() const;
private:
  WallhavenData m_Data;
};

}
