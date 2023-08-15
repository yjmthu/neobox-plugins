#include <wallbase.h>

class Native : public WallBase {
private:
  size_t GetFileCount();
  bool GetFileList();

public:
  explicit Native(YJson& setting);
  virtual ~Native();

public:
  void GetNext(Callback callback) override;
  void SetJson(const YJson& json) override;
  inline static const auto m_Name = u8"本地壁纸"s;

private:
  YJson& InitSetting(YJson& setting);
  YJson& GetCurInfo();
  fs::path GetImageDir() const;
  YJson& GetCurInfo() const {
    return const_cast<Native*>(this)->GetCurInfo();
  }

private:
  std::vector<std::u8string> m_FileList;
};
