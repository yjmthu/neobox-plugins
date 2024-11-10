#ifndef NEOSYSTEMPLG_H
#define NEOSYSTEMPLG_H

#include <neobox/pluginobject.h>

class NeoTimerPlg: public PluginObject
{
protected:
  class QAction* InitMenuAction() override;
public:
  explicit NeoTimerPlg(YJson& settings);
  virtual ~NeoTimerPlg();
private:
  void InitFunctionMap() override;
  QAction* LoadMainMenuAction();
  static void LoadResources();
  static void SetDesktopRightMenu(bool on);
  static bool HasDesktopRightMenu();
  YJson& InitSettings(YJson& settings);
  class SystemCfg* m_Settings;
  QAction* m_MainMenuAction;
};

#endif // NEOSYSTEMPLG_H
