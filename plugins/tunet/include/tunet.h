#ifndef THUNET_H
#define THUNET_H

#include <neobox/pluginobject.h>
#include <atomic>

#include <portal.h>

class Thunet: public PluginObject
{
public:
  Thunet(YJson& settings);
  virtual ~Thunet();
private:
  void InitFunctionMap() override;
  class QAction* InitMenuAction() override;
  QAction* LoadMainMenuAction();
  void ShowMsg(Portal::Error err, const char* showSucc);
  HttpAction<void> LogInOut(bool login, bool silent);
  // static void SetDesktopRightMenu(bool on);
  // static bool HasDesktopRightMenu();
  // std::atomic_bool m_Inited = false;
  YJson& InitSettings(YJson& settings);
  class TuNetCfg* const m_Settings;
  Portal* const m_Portal;
  QAction* m_MainMenuAction;
};

#endif // THUNET_H
