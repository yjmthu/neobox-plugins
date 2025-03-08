#include <scriptoutput.h>

#include <wallpaper.h>
#include <wallbase.h>
#include <neobox/systemapi.h>
#include <neobox/process.h>

#include <utility>
#include <numeric>
#include <filesystem>

// export module wallpaper5;

namespace fs = std::filesystem;
using namespace std::literals;
using namespace Wall;

ScriptOutput::ScriptOutput(YJson& setting):
  WallBase(InitSetting(setting))
{
}

ScriptOutput::~ScriptOutput()
{
  //
}

YJson& ScriptOutput::InitSetting(YJson& setting)
{
  if (setting.isObject())
    return setting;
  setting = YJson::O {
    {u8"curcmd", u8"默认脚本"},
    {u8"cmds", YJson::O {
      {u8"默认脚本", YJson::O{
        {u8"command", u8"python.exe \"scripts/getpic.py\""},
        {u8"directory", GetStantardDir(m_Name)}
      }}
    }}
  };
  return setting;
}

ImageInfoX ScriptOutput::GetNext()
{
  ImageInfo ptr {};
  m_DataMutex.lock();
  auto const & u8cmd = GetCurInfo()[u8"command"].getValueString();
  m_DataMutex.unlock();

  if (u8cmd.empty()) {
    ptr.ErrorMsg = u8"Invalid command to get wallpaper path."s;
    ptr.ErrorCode = ImageInfo::CfgErr;
    co_return ptr;
  }
#ifdef _WIN32
  // std::vector<std::wstring> result;
  // const auto wcmd = Utf82WideString(u8cmd);
  NeoProcess process(u8cmd);
  auto res = co_await process.Run();
  if (!res || *res != 0) {
    ptr.ErrorMsg = u8"Run command with error."s;
    ptr.ErrorCode = ImageInfo::RunErr;
    co_return ptr;
  }
  
  auto output = process.GetStdOut();
  auto pos = output.find_first_of(L'\r');
  if (pos == output.npos) {
    pos = output.size();
  }
  auto firstLine = std::string_view(output.data(), pos);

  if (firstLine.empty()) {
    ptr.ErrorMsg = u8"Invalid command to get wallpaper path."s;
    ptr.ErrorCode = ImageInfo::CfgErr;
    co_return ptr;
  }

  fs::path str = firstLine;
#elif defined (__linux__)
  std::vector<std::u8string> result;
  GetCmdOutput(reinterpret_cast<const char*>(u8cmd.c_str()), result);
  if (result.empty()) {
    ptr.ErrorMsg = u8"Run command with empty output."s;
    ptr.ErrorCode = ImageInfo::RunErr;
    co_return ptr;
  }
  fs::path str = result.front();
#endif
  if (str.empty()) {
    ptr.ErrorMsg = u8"Run command with wrong output."s;
    ptr.ErrorCode = ImageInfo::RunErr;
    co_return ptr;
  }
  if (!fs::exists(str)) {
#ifdef _WIN32
    ptr.ErrorMsg = Ansi2Utf8String(output) + u8"\n程序运行输出不匹配，请确保输出图片路径！\n"s;
#else
    auto wErMsg = std::accumulate(result.begin(), result.end(),
      u8"程序运行输出不匹配，请确保输出图片路径！\n"s,
      [] (const std::u8string& a, const std::u8string& b) {
        return a + u8'\n' + b;
      });
    ptr.ErrorMsg = std::move(wErMsg);
#endif
    ptr.ErrorCode = ImageInfo::RunErr;
    co_return ptr;
  }
  ptr.ImagePath = str.make_preferred().u8string();
  ptr.ErrorCode = ImageInfo::NoErr;
  co_return ptr;
}

YJson& ScriptOutput::GetCurInfo() 
{
  return m_Setting[u8"cmds"][m_Setting[u8"curcmd"].getValueString()];
}
