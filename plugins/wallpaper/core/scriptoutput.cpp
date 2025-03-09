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
    ptr.ErrorMsg = "Invalid command to get wallpaper path."s;
    ptr.ErrorCode = ImageInfo::CfgErr;
    co_return ptr;
  }

  NeoProcess process(u8cmd);
  auto res = co_await process.Run().awaiter();
  if (!res || *res != 0) {
    ptr.ErrorMsg = "Run command with error."s;
    ptr.ErrorCode = ImageInfo::RunErr;
    co_return ptr;
  }
  
  auto output = process.GetStdOut();

#ifdef _WIN32
  auto pos = output.find_first_of('\r');
#else
  auto pos = output.find('\n');
#endif
  if (pos == output.npos) {
    pos = output.size();
  }
  auto firstLine = std::string_view(output.data(), pos);

  if (firstLine.empty()) {
    ptr.ErrorMsg = "Invalid command to get wallpaper path."s;
    ptr.ErrorCode = ImageInfo::CfgErr;
    co_return ptr;
  }

  fs::path str = firstLine;
  if (!fs::exists(str)) {
    ptr.ErrorMsg.assign(output.begin(), output.end());
    ptr.ErrorMsg += "\n程序运行输出不匹配，请确保输出图片路径！\n"s;
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
