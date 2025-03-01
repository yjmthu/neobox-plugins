#ifndef FAVORITE_H
#define FAVORITE_H

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
  HttpAction<ImageInfo> GetNext() override;
  fs::path GetImageDir() const;
  inline static const auto m_Name = u8"收藏壁纸"s;

  void Dislike(std::u8string_view sImgPath) override;
  void UndoDislike(std::u8string_view sImgPath) override;
  bool IsFileFavorite(const std::filesystem::path& path) const;

private:
  size_t GetFileCount() const;
  bool GetFileList();

private:
  std::vector<std::u8string> m_FileList;
};

#endif // FAVORITE_H
