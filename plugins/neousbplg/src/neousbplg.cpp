#include <neousbplg.h>
#include <usbdlg.hpp>

#define PluginName NeoUsbPlg
#include <pluginexport.cpp>

#include <QApplication>

#ifdef _WIN32
#include <windows.h>
#include <dbt.h>
#endif

PluginName::PluginName(YJson& settings)
  : PluginObject(InitSettings(settings), u8"neousbplg", u8"U盘助手")
  , m_Settings(settings)
  , m_UsbDlg(new UsbDlg(m_Settings))
{
  InitFunctionMap();
#ifdef _WIN32
  qApp->installNativeEventFilter(this);
#endif
}

PluginName::~PluginName()
{
  delete m_UsbDlg;
}

void PluginName::InitFunctionMap()
{
  m_PluginMethod = {
    {u8"openWindow",
      {u8"开关窗口", u8"打开/关闭 U盘助手窗口", [this](PluginEvent, void*){
        if (m_UsbDlg->isVisible()) {
          m_UsbDlg->hide();
        } else {
          m_UsbDlg->show();
        }
      }, PluginEvent::Void}
    },
    {u8"enableHideAside",
      {u8"贴边隐藏", u8"靠是否右贴边隐藏", [this](PluginEvent event, void* data){
        if (event == PluginEvent::Bool) {
          m_Settings.SetHideAside(*reinterpret_cast<bool *>(data));
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool *>(data) = m_Settings.GetHideAside();
        }
      }, PluginEvent::Bool}
    },
    {u8"moveInscreen",
      {u8"还原位置", u8"找不到窗口时可使用还原位置", [this](PluginEvent, void*){
        m_UsbDlg->hide();
        m_Settings.SetPosition(YJson::Null);
        m_UsbDlg->show();
        // QMetaObject::invokeMethod(m_UsbDlg, &QWidget::show);
      }, PluginEvent::Void}
    },
    {u8"enableHideWhenFull",
      {u8"全屏隐藏", u8"有全屏程序运行时是否隐藏", [this](PluginEvent event, void* data){
        if (event == PluginEvent::Bool) {
          m_Settings.SetHideWhenFull(*reinterpret_cast<bool *>(data));
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool *>(data) = m_Settings.GetHideWhenFull();
        }
      }, PluginEvent::Bool}
    },
  };

  m_Following.push_back({u8"neohotkeyplg", [this](PluginEvent event, void* data){
    if (event == PluginEvent::HotKey) {
      // 判断是否为想要的快捷键
      if (*reinterpret_cast<std::u8string*>(data) == u8"neousbplg") {
        if (m_UsbDlg->isVisible()) {
          m_UsbDlg->hide();
          mgr->ShowMsg("隐藏成功！");
        } else {
          m_UsbDlg->show();
          mgr->ShowMsg("显示成功！");
        }
      }
    }
  }});
}

QAction* PluginName::InitMenuAction()
{
  return this->PluginObject::InitMenuAction();
}

YJson& PluginName::InitSettings(YJson& settings)
{
  if (settings.isObject()) {
    return settings;
  }
  return settings = YJson::O {
    {u8"StayOnTop", false},
    {u8"HideWhenFull", false},
    {u8"HideAside", false},     // 0xFF
  };
}

#ifdef _WIN32
bool PluginName::nativeEventFilter(const QByteArray& eventType, void *message, qintptr *result)
{
  if(eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
    return false;
  // https://www.cnblogs.com/swarmbees/p/8145342.html
  auto const msg = static_cast<MSG*>(message);
  if(msg->message != WM_DEVICECHANGE || !msg->lParam) return false;

  // don't use the reinterpret_cast, because lParam isn't a pointer.
  PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
  if (lpdb->dbch_devicetype != DBT_DEVTYP_VOLUME) return false;

  // https://learn.microsoft.com/zh-cn/windows/win32/devio/wm-devicechange?redirectedfrom=MSDN
  switch(msg->wParam)
  {
  case DBT_DEVICEARRIVAL:
    // 已插入设备或介质，现已推出。
    m_UsbDlg->DoDeviceArrival(lpdb);
    break;
  case DBT_DEVICEREMOVECOMPLETE:
    // 已删除设备或媒体片段
    m_UsbDlg->DoDeviceRemoveComplete(lpdb);
    break;
  case DBT_DEVICEQUERYREMOVE:
    // 请求移除设备，可能失败  此时刷新不会让移动设备消失
  case DBT_DEVICEQUERYREMOVEFAILED:
  case DBT_DEVICEREMOVEPENDING:
    // 即将删除设备或媒体片段。 无法拒绝。
  case DBT_CUSTOMEVENT:
  case DBT_DEVNODES_CHANGED:
  default:
    break;
  }
  return false;
}
#endif
