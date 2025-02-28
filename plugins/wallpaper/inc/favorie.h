#include <wallpaper.h>
#include <neobox/systemapi.h>

#include <filesystem>

class Favorite : public WallBase {
public:
  explicit Favorite(YJson& setting);
  ~Favorite();
public:
  YJson& InitSetting(YJson& setting);
  // void InitData();
  HttpAction<ImageInfoEx> GetNext() override;
  fs::path GetImageDir() const;
  inline static const auto m_Name = u8"收藏壁纸"s;

  void Dislike(std::u8string_view sImgPath) override;
  void UndoDislike(std::u8string_view sImgPath) override;

private:
  size_t GetFileCount() const;
  bool GetFileList();

private:
  std::vector<std::u8string> m_FileList;
};
