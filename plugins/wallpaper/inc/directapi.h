#include <wallbase.h>

namespace Wall {

class DirectApi : public WallBase
{
public:
  explicit DirectApi(YJson& setting);
  ~DirectApi() override;

public:
  ImageInfoX GetNext() override;
  void SetJson(const YJson& json) override;
  inline static const auto m_Name = u8"直链壁纸"s;

private:
  YJson& InitSetting(YJson& setting);
  YJson& GetCurInfo();
  const YJson& GetCurInfo() const
  { return const_cast<DirectApi*>(this)->GetCurInfo(); }
  std::wstring GetImageName();
};

}
