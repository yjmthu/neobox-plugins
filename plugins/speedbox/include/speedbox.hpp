#ifndef SPEEDBOX_H
#define SPEEDBOX_H

#include <any>
#include <filesystem>

#include <neobox/widgetbase.hpp>
#include <neobox/pluginobject.h>
#include <speedboxcfg.h>
#include <sidechooser.hpp>

namespace fs = std::filesystem;

class SpeedBox : public WidgetBase {

  Q_OBJECT

private:
  SpeedBoxCfg& m_Settings;
  class NeoSpeedboxPlg* m_PluginObject;
  class QLibrary* const m_SkinDll;
  class SkinObject* m_CentralWidget;
  class NeoTimer* m_Timer;
  class TrayFrame* m_TrayFrame;
  class MenuBase& m_NetCardMenu;
  std::any m_AppBarData;

  friend void SideChooser::showEvent(QShowEvent*);
  QRect m_ScreenGeometry;
  class NetSpeedHelper& m_NetSpeedHelper;
  class ProcessForm* m_ProcessForm;
  std::string m_MemFrameStyle;

  class QPropertyAnimation* m_Animation;

  friend void SideChooser::SetupUi(SpeedBox*);
  using ScreenSide = SideChooser::ScreenSide;
  SideChooser::ScreenSide m_HideSide = ScreenSide::None;
  uint32_t m_ScreenAround = ScreenSide::None;

protected:
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
#ifdef _WIN32
  bool nativeEvent(const QByteArray& eventType,
                   void* message,
                   qintptr* result) override;
#endif
  void enterEvent(QEnterEvent* event) override;
  void leaveEvent(QEvent* event) override;

 public:
  explicit SpeedBox(NeoSpeedboxPlg* plugin, SpeedBoxCfg& settings, MenuBase* netcardMenu);
  ~SpeedBox();
  void InitShow(const PluginObject::FollowerFunction& callback);
  void InitMove();
  void UpdateSkin();
  void SetProgressMonitor(bool on);
  void SetTrayMode(bool on);
  void UpdateScreenIndex(int index);
  void MoveToScreenCenter();

  void LoadScreen(int index);
  QPoint ReadPosition();
  void SavePosition(QPoint pos);

 private:
  void SetWindowMode();
  bool LoadDll(fs::path dllPath);
  void LoadCurrentSkin();
#ifdef _WIN32
  void SetHideFullScreen();
  void UnSetHideFullScreen();
#else
  bool IsCurreenWindowFullScreen();
#endif
  void InitNetCard();
  void UpdateNetCardMenu();
  void UpdateNetCard(QAction* action, bool checked);
signals:
  void TimeOut();
  void NetCardChanged();
  void ScreenChanged(int index);
};

#endif
