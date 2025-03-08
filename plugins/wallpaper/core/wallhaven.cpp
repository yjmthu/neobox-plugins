#include <neobox/httplib.h>

#include <wallhaven.h>
#include <wallpaper.h>
#include <wallbase.h>
#include <neobox/systemapi.h>

#include <regex>
#include <utility>
#include <random>
#include <filesystem>

using namespace std::literals;

#ifdef _DEBUG
#include <iostream>

static std::ostream& operator<<(std::ostream& os, const std::u8string& str) {
  os.write(reinterpret_cast<const char*>(str.data()), str.size());
  return os;
}
#endif

using namespace Wall;

YJson WallhavenData::InitData() {
  try {
    return YJson(m_DataPath, YJson::UTF8);
  } catch (std::runtime_error error[[maybe_unused]]) {
    return YJson::O {
      {u8"Api"s,        YJson::String},
      {u8"Unused"s,     YJson::Array},
      {u8"Used"s,       YJson::Array},
      {u8"Blacklist"s,  YJson::Array}
    };
  }
}

bool WallhavenData::IsEmpty() const {
  return m_Unused.empty() && m_Used.empty();
}

void WallhavenData::ClearAll() {
  m_Used.clear();
  m_Unused.clear();
}

void WallhavenData::SaveData() {
  m_Data.toFile(m_DataPath);
}

Bool WallhavenData::DownloadUrl(Range range)
{
  static bool working = false;

  if (working) co_return false;

  working = true;
  auto res = co_await DownloadAll(range).awaiter();
  working = false;

  co_return res != std::nullopt && *res;
}


void WallhavenData::HandleResult(std::vector<std::u8string>& dataArray, const YJson &data)
{
  for (auto& i: data.getArray()) {
    auto name = i.find(u8"path")->second.getValueString().substr(31);
    auto const iter = std::find_if(m_Blacklist.cbegin(), m_Blacklist.cend(), [&name](const YJson& j){
      return j.getValueString() == name;
    });
    if (m_Blacklist.cend() == iter) {
      dataArray.emplace_back(std::move(name));
    }
  }
}

Bool WallhavenData::DownloadAll(const Range& range)
{
#ifdef _DEBUG
  std::cout << "download all page: " << range[0] << "-" << range[1] << std::endl;
#endif
  std::vector<std::u8string> dataArray;

  for (auto index: range) {
    auto indexString = std::to_string(index);
    auto url = m_ApiUrl + u8"&page=";
    url.append(indexString.begin(), indexString.end());

#ifdef _DEBUG
    std::cout << "downloading page <" << url << ">" << std::endl;
#endif
    // m_Request = std::make_unique<HttpLib>(HttpUrl(url), true);
    auto clt = HttpLib(HttpUrl(url), true);

    auto res = co_await clt.GetAsync();

    if (res->status != 200) {
#ifdef _DEBUG
      std::cout << "download page " << index << " failed with status code: " << res->status << std::endl;
#endif
      break;
    }
    YJson root(res->body.begin(), res->body.end());
    const auto &data = root[u8"data"];
    if (data.emptyA()) {
#ifdef _DEBUG
      std::cout << "download page " << index << " failed with empty data." << std::endl;
#endif
      break;
    }
    HandleResult(dataArray, data);
#ifdef _DEBUG
    std::cout << "current data count: " << dataArray.size() << std::endl;
#endif
  }

#ifdef _DEBUG
  std::cout << dataArray.size() << " image url downloaded." << std::endl;
#endif

  if (dataArray.empty()) co_return false;

  std::mt19937 g(std::random_device{}());
  std::shuffle(dataArray.begin(), dataArray.end(), g);
  std::lock_guard<std::mutex> locker(m_Mutex);
  m_Unused.assign(dataArray.begin(), dataArray.end());
  co_return true;
}

Wallhaven::Wallhaven(YJson& setting):
  WallBase(InitSetting(setting)),
  m_Data(m_DataMutex, m_DataDir / u8"WallhaveData.json")
{
}

Wallhaven::~Wallhaven()
{
}


bool Wallhaven::IsPngFile(std::u8string& str) {
  // if (!Wallpaper::IsOnline()) return false;
  HttpLib clt(HttpUrl(u8"https://wallhaven.cc/api/v1/w/"s + str));
  auto res = clt.Get();
  if (res->status != 200)
    return false;

  Locker locker(m_DataMutex);
  YJson js(res->body.begin(), res->body.end());
  str = js[u8"data"][u8"path"].getValueString().substr(31);
  return true;
}

YJson& Wallhaven::InitSetting(YJson& setting)
{
  if (!setting.isObject()) {
    setting = YJson::O {
      {u8"WallhavenCurrent", u8"最热壁纸"},
      {u8"WallhavenApi", YJson::O {}},
      // {u8"ApiKey", YJson::Null},
      {u8"PageSize", 5},
      {u8"Parameter", YJson::O {
        // {u8"ApiKey", nullptr},
      }}
    };
  }
  if (auto& param = setting[u8"Parameter"]; param.isNull()) {
    param = YJson::Object;
  }
  if (auto& m_ApiObject = setting[u8"WallhavenApi"]; m_ApiObject.emptyO()) {
    std::initializer_list<std::tuple<std::u8string, bool, std::u8string, std::u8string>>
      paramLIst = {
        {u8"最热壁纸"s, true, u8"categories"s, u8"111"s},
        {u8"风景壁纸"s, true, u8"q"s, u8"nature"s},
        {u8"动漫壁纸"s, true, u8"categories"s, u8"010"s},
        {u8"随机壁纸"s, false, u8"sorting"s, u8"random"s},
        {u8"极简壁纸"s, false, u8"q"s, u8"minimalism"s},
        // {u8"鬼刀壁纸"s, false, u8"q"s, u8"ghostblade"s}
    };
    const auto initDir = GetStantardDir(m_Name);

    for (const auto& [i, j, k, l] : paramLIst) {
      auto& item = m_ApiObject.append(YJson::Object, i)->second;
      item[u8"Parameter"] = j ? YJson::O {
        {u8"sorting", u8"toplist"}, {k, l}}: YJson::O {{k, l},};
      item.append(initDir, u8"Directory");
      item.append(1, u8"StartPage");
    }
  }
  return setting;
}

std::u8string Wallhaven::GetApiPathUrl() const
{
  return GetCurInfo()[u8"Parameter"]
    .copy()
    .joinO(m_Setting[u8"Parameter"])
    .urlEncode(u8"https://wallhaven.cc/api/v1/search?"sv);
}

YJson& Wallhaven::GetCurInfo()
{
  return m_Setting[u8"WallhavenApi"][m_Setting[u8"WallhavenCurrent"].getValueString()];
}

Bool Wallhaven::CheckData()
{
  LockerEx locker(m_DataMutex);

  auto const apiUrl = GetApiPathUrl();
  if (m_Data.m_ApiUrl == apiUrl) {
    if (!m_Data.IsEmpty()) co_return true;
  }

  m_Data.ClearAll();
  m_Data.m_ApiUrl = apiUrl;
  auto const first = GetCurInfo()[u8"StartPage"].getValueInt();
  auto const last = m_Setting[u8"PageSize"].getValueInt() + first;
  locker.unlock();

  auto const res = co_await m_Data.DownloadUrl({first, last}).awaiter();
  
  co_return res != std::nullopt && *res;
}

ImageInfoX Wallhaven::GetNext()
{
  // https://w.wallhaven.cc/full/1k/wallhaven-1kmx19.jpg

  auto res = co_await CheckData().awaiter();
  if (!res || !*res) co_return {
    .ErrorMsg = u8"列表下载失败。",
    .ErrorCode = ImageInfo::NetErr
  };

  m_DataMutex.lock();
  if (m_Data.m_Unused.empty()) {
    m_Data.m_Unused.swap(m_Data.m_Used);
    std::vector<std::u8string> temp;
    for (auto& i : m_Data.m_Unused) {
      temp.emplace_back(std::move(i.getValueString()));
    }
    std::mt19937 g(std::random_device{}());
    std::shuffle(temp.begin(), temp.end(), g);
    m_Data.m_Unused.assign(temp.begin(), temp.end());
  }

  ImageInfo ptr {};
  if (!m_Data.m_Unused.empty()) {
    std::u8string name = m_Data.m_Unused.back().getValueString();
    ptr.ErrorCode = ImageInfo::NoErr;
    ptr.ImagePath = GetCurInfo()[u8"Directory"].getValueString() + u8"/" + name;
    ptr.ImageUrl =
        u8"https://w.wallhaven.cc/full/"s + name.substr(10, 2) + u8"/"s + name;
    m_Data.m_Used.push_back(name);
    m_Data.m_Unused.pop_back();
    m_Data.SaveData();
  } else {
    ptr.ErrorMsg = u8"列表下载失败。";
    ptr.ErrorCode = ImageInfo::NetErr;
  }
  m_DataMutex.unlock();

  co_return ptr;
}

std::string Wallhaven::IsWallhavenFile(std::string name)
{
  std::regex pattern("^.*(wallhaven-[0-9a-z]{6}).*\\.(png|jpg)$", std::regex::icase);
  std::smatch result;
  if (!std::regex_match(name, result, pattern)) {
    return std::string();
  }
  return result.str(1) + "." + result.str(2);
}

void Wallhaven::Dislike(std::u8string_view sImgPath)
{
  Locker locker(m_DataMutex);

  fs::path img = sImgPath;
  if (!img.has_filename()) return;
  auto const id = IsWallhavenFile(img.filename().string());
  if (id.empty())
    return;

  YJson const u8Id = StringAsUtf8(id);
  m_Data.m_Unused.remove(u8Id);
  m_Data.m_Used.remove(u8Id);
  m_Data.m_Blacklist.push_back(u8Id);
  m_Data.SaveData();
}

void Wallhaven::UndoDislike(std::u8string_view sImgPath)
{
  Locker locker(m_DataMutex);

  fs::path path = sImgPath;
  if (!path.has_filename())
    return;

  auto const id = IsWallhavenFile(path.filename().string());
  if (id.empty())
    return;

  YJson const u8Id = StringAsUtf8(id);

  m_Data.m_Blacklist.remove(u8Id);
  m_Data.m_Used.push_back(u8Id);
  m_Data.SaveData();
}

void Wallhaven::SetJson(const YJson& json)
{
  WallBase::SetJson(json);

  Locker locker(m_DataMutex);

  m_Data.m_ApiUrl.clear();
  m_Data.SaveData();
}
