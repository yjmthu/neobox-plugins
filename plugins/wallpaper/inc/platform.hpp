#pragma once

#include <string>
#include <filesystem>
#include <neobox/process.h>

#ifdef _DEBUG
#include <iostream>
#endif

#ifdef _WIN32
#include <thread>
#else
#include <unistd.h>
#include <list>
#include <optional>
#include <format>
#endif

namespace WallpaperPlatform {

namespace fs = std::filesystem;
using namespace std::literals;

enum class Desktop { WIN, KDE, CDE, DDE, GNOME, XFCE, UNKNOWN };

static void GetCmdOutput(std::u8string cmd, std::list<std::string>& result) {
  NeoProcess process(cmd);
  auto res = process.Run().get();
  if (!res || res.value() != 0) {
    return;
  }

  auto out = process.GetStdOut();

  std::string line;
  for (auto c : out) {
    if (c == '\n') {
      result.push_back(std::move(line));
      line.clear();
    } else {
      line.push_back(c);
    }
  }
  if (!line.empty()) {
    result.push_back(std::move(line));
  }
}

inline Desktop GetDesktop() {
#ifdef _WIN32
  return Desktop::WIN;
#elif defined(__linux__)
  auto const ndeEnv = std::getenv("XDG_CURRENT_DESKTOP");
  if (!ndeEnv) {
    return Desktop::UNKNOWN;
  }
  std::string nde = ndeEnv;
  constexpr auto npos = std::string::npos;
  if (nde.find("KDE") != npos) {
    return Desktop::KDE;
  } else if (nde.find("GNOME") != npos) {
    return Desktop::GNOME;
  } else if (nde.find("Deepin") != npos) {
    return Desktop::DDE;
  } else if (nde.find("XFCE") != npos) {
    return Desktop::XFCE;
  } else if (nde.find("Cinnamon") != npos) {
    return Desktop::CDE;
  } else {
    return Desktop::UNKNOWN;
  }
#else
  return Desktop::UNKNOWN;
#endif
}

inline std::optional<fs::path> GetWallpaper() {
#ifdef __linux__
  switch (GetDesktop()) {
  case Desktop::KDE: {
    char8_t argStr[] = u8"qdbus org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript '"
      "var allDesktops = desktops();"
      "print(allDesktops);"
      "for (i=0; i < allDesktops.length; i++){"
        "d = allDesktops[i];"
        "d.wallpaperPlugin = \"org.kde.image\";"
        "d.currentConfigGroup = Array(\"Wallpaper\", \"org.kde.image\", \"General\");"
        "print(d.readConfig(\"Image\"));"
      "}"
    "'";
    std::list<std::string> result;
    GetCmdOutput(argStr, result);
    while (!result.empty() && result.front().empty()) {
      result.pop_front();
    }
    if (!result.empty()) {
      std::string_view data = result.front();
      auto pos = data.find("file://");
      if (pos != data.npos) {
        return data.substr(pos + 7);
      }
    }
    break;
  }
  case Desktop::GNOME:{
    std::u8string_view argStr = u8"gsettings get org.gnome.desktop.background picture-uri"sv;
    std::list<std::string> result;
    GetCmdOutput(argStr.data(), result);
    while (!result.empty() && result.front().size() <= 7) {
      result.pop_front();
    }
    if (result.empty()) break;
    std::string_view uri = result.front();
    if (uri.size() <= 7) break;
    return uri.substr(6, uri.size() - 7);
  }
  case Desktop::CDE:{
    std::u8string_view argStr = u8"gsettings get org.cinnamon.desktop.background picture-uri"sv;
    std::list<std::string> result;
    GetCmdOutput(argStr.data(), result);
    while (!result.empty() && result.front().size() <= 7) {
      result.pop_front();
    }
    if (result.empty()) break;
    std::string_view uri = result.front();
    return uri.substr(6, uri.size() - 7);
  }
  case Desktop::DDE: {
    // DDE无此功能
    break;
  }
  case Desktop::XFCE: {
    std::u8string_view argStr = u8"xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/workspace0/last-image"sv;
    std::list<std::string> result;
    GetCmdOutput(argStr.data(), result);
    break;
  }
  default:
    break;
  }
  return std::nullopt;
#elif defined(_WIN32)
  auto curWallpaper = RegReadString(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"WallPaper");
  if (curWallpaper.empty() || !fs::exists(curWallpaper)) {
    return std::nullopt;
  }
  return curWallpaper;
#else
  return std::nullopt;
#endif
}

inline bool SetWallpaper(fs::path imagePath) {
#ifdef __linux__
  static auto const m_DesktopType = GetDesktop();
#endif
  if (!fs::exists(imagePath) || !fs::is_regular_file(imagePath)) {
    return false;
  }
#if defined(_WIN32)
  std::thread([imagePath]() mutable {
    // use preferred separator to prevent win32 api crash.
    imagePath.make_preferred();
    std::wstring str = imagePath.wstring();
    ::SystemParametersInfoW(
      SPI_SETDESKWALLPAPER, UINT(0),
      const_cast<WCHAR*>(str.c_str()),
      SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
  }).detach();
  return true;
#elif defined(__linux__)
  std::string argStr;
  switch (m_DesktopType) {
    // https://github.com/bharadwaj-raju/libdesktop/issues/1
    case Desktop::KDE:
      argStr = std::format(
        "var allDesktops = desktops();"
        "print(allDesktops);"
        "for (i=0; i < allDesktops.length; i++){{"
          "d = allDesktops[i];"
          "d.wallpaperPlugin = \"org.kde.image\";"
          "d.currentConfigGroup = Array(\"Wallpaper\", \"org.kde.image\", \"General\");"
          "d.writeConfig(\"Image\", \"file://{}\")"
        "}}", imagePath.string());
      if (fork() == 0) {
        execlp(
          "qdbus", "qdbus",
          "org.kde.plasmashell", "/PlasmaShell",
          "org.kde.PlasmaShell.evaluateScript",
          argStr.c_str(), nullptr
        );
      }
      break;
    case Desktop::GNOME:
      argStr = std::format("\"file:{}\"", imagePath.string());
      if (fork() == 0) {
        execlp(
          "gsettings", "gsettings",
          "set", "org.gnome.desktop.background", "picture-uri",
          argStr.c_str(), nullptr
        );
      }
      break;
      // cmdStr = "gsettings set org.gnome.desktop.background picture-uri \"file:" + imagePath.string();
    case Desktop::CDE:
      argStr = std::format("\"file:{}\"", imagePath.string());
      if (fork() == 0) {
        execlp(
          "gsettings", "gsettings",
          "set", "org.cinnamon.desktop.background", "picture-uri",
          argStr.c_str(), nullptr
        );
      }
      break;
    case Desktop::DDE:
    /*
      Old deepin:
      std::string m_sCmd ("gsettings set
      com.deepin.wrap.gnome.desktop.background picture-uri \"");
    */
      // xrandr|grep 'connected primary'|awk '{print $1}' ======> eDP
      argStr = std::format("string:\"file://{}\"", imagePath.string());
      if (fork() == 0) {
        execlp(
          "dbus-send", "dbus-send",
          "--dest=com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance",
          "--print-reply", "com.deepin.daemon.Appearance.SetMonitorBackground",
          "string:\"eDP\"", argStr.c_str(), nullptr
        );
      }
      break;
    case Desktop::XFCE:
      argStr = std::format("\"{}\"", imagePath.string());
      if (fork() == 0) {
        execlp(
          "xfconf-query",
          "xfconf-query",
          "-c", "xfce4-desktop",
          "-p", "/backdrop/screen0/monitor0/workspace0/last-image",
          "-s", argStr.c_str(), nullptr
        );
      }
      break;
    default:
#ifdef _DEBUG
      std::cerr << "不支持的桌面类型；\n";
#endif
      return false;
  }
  return true;
#endif
}

}
