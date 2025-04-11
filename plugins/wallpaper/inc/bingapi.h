#include <neobox/httplib.h>
#include <wallpaper.h>
#include <wallbase.h>
#include <neobox/systemapi.h>

#include <filesystem>

namespace Wall {

class BingApi : public WallBase {
public:
  explicit BingApi(YJson &setting);
  virtual ~BingApi();

public:
  ImageInfoX GetNext() override;
  void SetJson(const YJson &json) override;
  inline static const auto m_Name = u8"必应壁纸"s;

private:
  void AutoDownload();
  YJson &InitSetting(YJson &setting);
  void InitData();
  Bool CheckData();
  static std::u8string GetToday();
  bool GetAutoDownload() const;
  std::u8string GetImageName(YJson &imgInfo);
  class NeoTimer *m_Timer;

private:
  // YJson *m_Data;
  std::unique_ptr<YJson> m_Data;
  std::unique_ptr<class HttpLib> m_DataRequest;
  const fs::path m_DataPath = m_DataDir / u8"BingApiData.json";
};

}
