#ifndef THUNET_H
#define THUNET_H

#include <neobox/pluginobject.h>

class Thunet: public PluginObject
{
public:
  Thunet(YJson& settings);
  virtual ~Thunet();
private:
  void InitFunctionMap() override;
  class QAction* InitMenuAction() override;
  QAction* LoadMainMenuAction();
  // static void LoadResources();
  // static void SetDesktopRightMenu(bool on);
  // static bool HasDesktopRightMenu();
  YJson& InitSettings(YJson& settings);
  class TuNetCfg* m_Settings;
  class Portal* m_Portal;
  QAction* m_MainMenuAction;
};

#endif // THUNET_H
