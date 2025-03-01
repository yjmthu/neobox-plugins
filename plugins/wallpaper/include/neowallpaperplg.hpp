#ifndef NEOWALLPAPERPLG_H
#define NEOWALLPAPERPLG_H

#include <neobox/pluginobject.h>
#include <QObject>

class NeoWallpaperPlg: public QObject, public PluginObject
{
  Q_OBJECT

protected:
  class QAction* InitMenuAction() override;
  bool eventFilter(QObject* watched, QEvent* event) override;
public:
  explicit NeoWallpaperPlg(YJson& settings);
  virtual ~NeoWallpaperPlg();
private:
  YJson& InitSettings(YJson& settings);
  void InitFunctionMap() override;
  void LoadWallpaperTypeMenu(class MenuBase* pluginMenu);
  void LoadWallpaperExMenu(class MenuBase* pluginMenu);
  class Wallpaper* const m_Wallpaper;
  class MenuBase* m_WallpaperTypesMenu;
  class QAction* m_MoreSettingsAction;
  class QActionGroup* m_WallpaperTypesGroup;
  class QAction* m_MainMenuAction;
private:
  MenuBase* LoadWallavenMenu(MenuBase* parent);
  MenuBase* LoadBingApiMenu(MenuBase* parent);
  MenuBase* LoadDirectApiMenu(MenuBase* parent);
  MenuBase* LoadNativeMenu(MenuBase* parent);
  MenuBase* LoadScriptMenu(MenuBase* parent);
  MenuBase* LoadFavoriteMenu(MenuBase* parent);
  void LoadDropMenu(QAction* action);
  void LoadMainMenuAction();
signals:
  void NextWallpaperChanged();
  void NextWallpaperDisliked();
  void DropImageDownloaded();
};

#endif // NEOWALLPAPERPLG_H
