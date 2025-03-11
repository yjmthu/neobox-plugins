#include <QApplication>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QSharedMemory>
#include <QProcess>
#include <QDragEnterEvent>
#include <QLibrary>

#include <neobox/neomenu.hpp>
#include <speedbox.hpp>
#include <yjson/yjson.h>
#include <neobox/pluginmgr.h>
#include <neobox/appcode.hpp>
#include <neobox/pluginobject.h>
#include <neobox/systemapi.h>
#include <neobox/neotimer.h>

#include <neospeedboxplg.h>
#include <skinobject.h>
#include <netspeedhelper.h>
#include <processform.h>
#include <trayframe.h>

#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

extern SkinApi newSkin, newSkin1, newSkin2, newSkin3, newSkin4, newSkin5, newSkin6;

static SkinApi* const g_SkinApis[] = {
  newSkin, newSkin1, newSkin2, newSkin3, newSkin4, newSkin5, newSkin6
};

// static SkinApi* const g_SkinApis[7] = {};

const std::vector<std::u8string> NeoSpeedboxPlg::m_DefaultSkins = {
    u8"简简单单",
    u8"经典火绒",
    u8"电脑管家",
    u8"数字卫士",
    u8"独霸一方",
    u8"开源力量",
    u8"果里果气",
};

SpeedBox::SpeedBox(NeoSpeedboxPlg* plugin, SpeedBoxCfg& settings, MenuBase* netcardMenu)
    : WidgetBase(nullptr)
    , m_PluginObject(plugin)
    , m_Settings(settings)
    , m_NetSpeedHelper(*plugin->m_NetSpeedHelper)
    , m_ProcessForm(m_Settings.GetProgressMonitor() ? new ProcessForm(this) : nullptr)
    , m_NetCardMenu(*netcardMenu)
    , m_SkinDll(new QLibrary(this))
    , m_CentralWidget(nullptr)
    , m_Timer(NeoTimer::New())
    , m_TrayFrame(nullptr)
    , m_AppBarData(nullptr)
    , m_Animation(new QPropertyAnimation(this, "geometry"))
{
  SetWindowMode();
#ifdef _WIN32
  SetHideFullScreen();
#endif
  LoadScreen(m_Settings.GetScreenIndex());
  LoadCurrentSkin();
  InitNetCard();
}

SpeedBox::~SpeedBox() {
  m_Timer->Destroy();

  delete m_Animation;
  delete m_ProcessForm;
  delete m_CentralWidget;
  m_SkinDll->unload();
  delete m_SkinDll;

#ifdef _WIN32
  UnSetHideFullScreen();
#endif
  
  delete m_TrayFrame;
}

void SpeedBox::InitShow(const PluginObject::FollowerFunction& callback) {
  show();

  if (m_Animation) {
    m_Animation->setDuration(100);
    m_Animation->setTargetObject(this);
    connect(m_Animation, &QPropertyAnimation::finished, this, [this]() {
      m_Settings.SetPosition(YJson::A{x(), y()});
    });
  }

  UpdateNetCardMenu();

  bool buffer = m_Settings.GetColorEffect();
  callback(PluginEvent::Bool, &buffer);

  if (m_Settings.GetTaskbarMode()) {
    SetTrayMode(true);
  }
}

void SpeedBox::SetWindowMode() {
  setWindowTitle("Neobox");
  setWindowIcon(QIcon(QStringLiteral(":/icons/neobox.ico")));
  setAttribute(Qt::WA_TransparentForMouseEvents, m_Settings.GetMousePenetrate());
  setAttribute(Qt::WA_TranslucentBackground, true);
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
  setStyleSheet(
    "QWidget {"
      "background-color: transparent;"
    "}"
    "QToolTip {"
      "border: 1px solid rgb(118, 118, 118);"
      "background-color: white;"
      "color: black;"
    "}");
  setAcceptDrops(true);

  auto p = m_Settings.GetPosition();
  move(p.front().getValueInt(), p.back().getValueInt());
}

void SpeedBox::UpdateSkin()
{
  delete m_CentralWidget;
  if (m_SkinDll->isLoaded()) {
    m_SkinDll->unload();
  }
  m_CentralWidget = nullptr;
  LoadCurrentSkin();
}

bool SpeedBox::LoadDll(fs::path dllPath)
{
  if (!fs::exists(dllPath)) return false;

  SkinObject* (*newSkin)(QWidget*, const TrafficInfo&);
  // bool (*skinVersion)(const std::string&);
  auto const libPath = dllPath.make_preferred().u16string();
  m_SkinDll->setFileName(QString::fromStdU16String(libPath));

  if (!m_SkinDll->load()) return false;

  newSkin = reinterpret_cast<decltype(newSkin)>(m_SkinDll->resolve("newSkin"));
  if (!newSkin) {
    m_SkinDll->unload();
    return false;
  }

  m_CentralWidget = newSkin(this, m_NetSpeedHelper.m_TrafficInfo);

  return true;
}

void SpeedBox::LoadCurrentSkin() {

  auto skinName = m_Settings.GetCurSkin();
  auto allSkins = YJson(m_Settings.GetUserSkins());
  auto iter = allSkins.find(skinName);
  int id = m_PluginObject->IsDefaultSkin(skinName);
  if (iter == allSkins.endO() && id == -1) {
    mgr->ShowMsg("无效的皮肤名称！");
    id = 0;
  }

  if (id == -1) {
    const auto skinFileName = iter->second.getValueString();
#ifdef _WIN32
  auto skinPath = u8"skins/" + skinFileName + u8".dll";
#else
  auto skinPath = u8"skins/lib" + skinFileName + u8".so";
#endif
  auto qSkinPath = QString::fromUtf8(skinPath.data(), skinPath.size());
    if (QFile::exists(qSkinPath)) {
      if (LoadDll(skinPath)) return;
      mgr->ShowMsg("皮肤文件无效！");
      id = 0;
    } else {
      mgr->ShowMsg("皮肤文件找不到！");
    }
  }
  if (g_SkinApis[id]) {
    m_CentralWidget = g_SkinApis[id](this, m_NetSpeedHelper.m_TrafficInfo);
  }
}

#ifdef _WIN32
void SpeedBox::SetHideFullScreen() {
  m_AppBarData = APPBARDATA {
    sizeof(APPBARDATA),
    reinterpret_cast<HWND>(winId()), 
    static_cast<UINT>(MsgCode::MSG_APPBAR_MSGID),
    0, 0, 0
  };
  SHAppBarMessage(ABM_NEW, &std::any_cast<APPBARDATA&>(m_AppBarData));
}

void SpeedBox::UnSetHideFullScreen() {
  SHAppBarMessage(ABM_REMOVE, &std::any_cast<APPBARDATA&>(m_AppBarData));
  m_AppBarData.reset();
}
#else
bool SpeedBox::IsCurreenWindowFullScreen() {
  // https://blog.csdn.net/qq_41264992/article/details/126976154

  auto GetCmdOutput = [](const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return std::string { };
    char buffer[128];
    std::string data;
    while (!feof(pipe)) {
      if (fgets(buffer, 128, pipe) != nullptr) {
        data += buffer;
      }
    }
    pclose(pipe);

    while (data.ends_with('\n')) {
      data.pop_back();
    }

    auto pos = data.find_last_of('\n');
    if (pos != std::string::npos) {
      data = data.substr(pos + 1);
    }
    return data;
  };

  // 获取窗口的ID
  auto activeWinCmd =
      "xprop -root | grep _NET_ACTIVE_WINDOW | head -1 | awk '{print $5}' | sed 's/,//' | sed 's/^0x/0x0/'";
  auto result = GetCmdOutput(activeWinCmd);
  if (result.empty()) return false;
  auto activeWinId = result;

  // 异常ID过滤
  if (activeWinId == "0x00") {
    return false;
  }

  // 获取屏幕尺寸
  auto screenGeoInfoCmd = "xwininfo -root | awk -F'[ +]' '$3 ~ /-geometry/ {print $4}'";
  result = GetCmdOutput(screenGeoInfoCmd);
  if (result.empty()) return false;
  auto screenSize = result;

  // 根据窗口ID获取窗口信息
  auto searchWindowCmd = "xwininfo -id  " + activeWinId + " | awk -F'[ +]' '$3 ~ /-geometry/ {print $4}'";

  result = GetCmdOutput(searchWindowCmd.c_str());
  if (result.empty()) return false;
  auto activeWindowSize = result;
  // std::cout << "<" << active_window_info << ":" << screen_info << ">" << std::endl;

  // 窗口状态判断
  return (activeWindowSize == screenSize);
}
#endif

void SpeedBox::SetProgressMonitor(bool on)
{
  if (on) {
    if (!m_ProcessForm)
      m_ProcessForm = new ProcessForm(this);
  } else {
    delete m_ProcessForm;
    m_ProcessForm = nullptr;
  }
}

void SpeedBox::SetTrayMode(bool on)
{
  if (on) {
    if (m_TrayFrame) return;
    m_TrayFrame = new TrayFrame(*this);
  } else {
    delete m_TrayFrame;
    m_TrayFrame = nullptr;
  }
}

void SpeedBox::wheelEvent(QWheelEvent *event)
{
  auto const numDegrees = event->angleDelta();

  if (!numDegrees.isNull() && numDegrees.y()) {
    if (numDegrees.y() > 0) {
      if (m_ProcessForm && !m_ProcessForm->isVisible()) {
        m_ProcessForm->show();
      }
    } else {
      if (m_ProcessForm && m_ProcessForm->isVisible()) {
        m_ProcessForm->hide();
      }
    }
  }

  event->accept();
}

void SpeedBox::mouseMoveEvent(QMouseEvent* event) {
  if (event->buttons() == Qt::LeftButton) {
    if (m_ProcessForm && m_ProcessForm->isVisible())
      m_ProcessForm->hide();
    m_PluginObject->SendBroadcast(PluginEvent::MouseMove, event);
  }
  this->WidgetBase::mouseMoveEvent(event);
}

void SpeedBox::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::RightButton) {
    mgr->m_Menu->popup(pos() + event->pos());
  } else if (event->button() == Qt::MiddleButton) {
    mgr->Restart();
  }
  this->WidgetBase::mousePressEvent(event);
}

void SpeedBox::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    m_Settings.SetPosition(YJson::A{x(), y()});
    auto screen = QApplication::screenAt(QCursor::pos());
    auto index = QApplication::screens().indexOf(screen);
    if (m_Settings.GetScreenIndex() != index) {
      m_ScreenGeometry = screen->geometry();
      m_Settings.SetScreenIndex(index);
      emit ScreenChanged(index);
    }
  }
  this->WidgetBase::mouseReleaseEvent(event);
}

void SpeedBox::mouseDoubleClickEvent(QMouseEvent* event) {
  m_PluginObject->SendBroadcast(PluginEvent::MouseDoubleClick, event);
  event->accept();
}

void SpeedBox::dragEnterEvent(QDragEnterEvent* event) {
  auto mimeData = event->mimeData();
  if (mimeData->hasUrls() || mimeData->hasText())
    event->acceptProposedAction();
  else
    event->ignore();
}

void SpeedBox::dropEvent(QDropEvent* event) {
  auto mimeData = event->mimeData();

  m_PluginObject->SendBroadcast(PluginEvent::Drop, event);
  event->accept();
}

#ifdef _WIN32
bool SpeedBox::nativeEvent(const QByteArray& eventType,
                           void* message,
                           qintptr* result) {
  MSG* msg = static_cast<MSG*>(message);

  if (MsgCode::MSG_APPBAR_MSGID == static_cast<MsgCode>(msg->message)) {
    switch ((UINT)msg->wParam) {
      case ABN_FULLSCREENAPP:
        if (msg->lParam)
          hide();
        else
          show();
        return true;
      default:
        break;
    }
  }
  return false;
}
#endif

static constexpr int delta = 1;

void SpeedBox::enterEvent(QEnterEvent* event) {
  if (!m_Animation) return;

  auto const& rtScreen = m_ScreenGeometry;
  auto rtForm = this->frameGeometry();
  m_Animation->setStartValue(rtForm);
  switch (m_HideSide) {
    case HideSide::Left:
      rtForm.moveTo(rtScreen.left() - delta, rtForm.y());
      break;
    case HideSide::Right:
      rtForm.moveTo(rtScreen.right() - rtForm.width() + delta, rtForm.y());
      break;
    case HideSide::Top:
      rtForm.moveTo(rtForm.x(), rtScreen.top() - delta);
      break;
    case HideSide::Bottom:
      rtForm.moveTo(rtForm.x(), rtScreen.bottom() - rtForm.height() + delta);
      break;
    default:
      event->ignore();
      return;
  }
  event->accept();
  m_Animation->setEndValue(rtForm);
  m_Animation->start();
}

void SpeedBox::leaveEvent(QEvent* event) {
  if (!m_Animation) return;

  auto const& rtScreen = m_ScreenGeometry;
  auto rtForm = this->frameGeometry();
  m_Animation->setStartValue(rtForm);
  auto const hideBits = m_Settings.GetHideAside();
  if ((HideSide::Right & hideBits) && rtForm.right() + delta >= rtScreen.right()) {
    m_HideSide = HideSide::Right;
    rtForm.moveTo(rtScreen.right() - delta, rtForm.y());
  } else if ((HideSide::Left & hideBits)  && rtForm.left() - delta <= rtScreen.left()) {
    m_HideSide = HideSide::Left;
    rtForm.moveTo(rtScreen.left() + delta - rtForm.width(), rtForm.y());
  } else if ((HideSide::Top & hideBits) && rtForm.top() - delta <= rtScreen.top()) {
    m_HideSide = HideSide::Top;
    rtForm.moveTo(rtForm.x(), rtScreen.top() + delta - rtForm.height());
  } else if ((HideSide::Bottom & hideBits) && rtForm.bottom() + delta >= rtScreen.bottom()) {
    m_HideSide = HideSide::Bottom;
    rtForm.moveTo(rtForm.x(), rtScreen.bottom() - delta);
  } else {
    m_HideSide = HideSide::None;
    event->ignore();
    return;
  }
  m_Animation->setEndValue(rtForm);
  m_Animation->start();
  event->accept();
}

void SpeedBox::InitMove()
{
  LoadScreen(m_Settings.GetScreenIndex());

  auto pos = m_ScreenGeometry.topLeft() + QPoint(100, 100);
  move(pos);

  m_HideSide = HideSide::None;
  m_Settings.SetPosition(YJson::A { pos.x(), pos.y() } );
  if (!isVisible())
    show();
  mgr->ShowMsg("移动成功！");
}

void SpeedBox::LoadScreen(int index) {
  auto screens = QGuiApplication::screens();
  if (screens.empty()) {
    mgr->ShowMsg("无法获取屏幕信息！");
    return;
  }
  if (index >= screens.size()) {
    m_ScreenGeometry = QGuiApplication::primaryScreen()->geometry();
  } else {
    m_ScreenGeometry = screens[index]->geometry();
  }
}

void SpeedBox::UpdateScreenIndex(int index)
{
  LoadScreen(index);

  auto pos = m_ScreenGeometry.center() - QPoint(width() / 2, height() / 2);
  move(pos);
  m_Settings.SetPosition(YJson::A { pos.x(), pos.y() }, false);

  m_Settings.SetScreenIndex(index);
  mgr->ShowMsg("切换屏幕成功！");
}

void SpeedBox::InitNetCard()
{
  m_Timer->StartTimer(1s, [this] {
#ifdef _WIN32
    static int count = 10;
    if (--count == 0) {
      m_NetSpeedHelper.UpdateAdaptersAddresses();
      count = 10;
      emit NetCardChanged();
    }
#endif
    emit TimeOut();
  });

#ifdef _WIN32
  connect(this, &SpeedBox::NetCardChanged, this, &SpeedBox::UpdateNetCardMenu);
#endif

  connect(this, &SpeedBox::TimeOut, this, [this] {
#ifndef _WIN32
    if (IsCurreenWindowFullScreen()) {
      if (isVisible()) hide();
    } else {
      if (!isVisible()) show();
    }
#endif
    if (!m_CentralWidget) return;
    m_NetSpeedHelper.GetSysInfo();
    m_CentralWidget->UpdateText();
    if (m_ProcessForm) {
      m_ProcessForm->UpdateList();
    }
  });
}

void SpeedBox::UpdateNetCardMenu()
{
  for (auto i: m_NetCardMenu.actions()) {
    delete i;
  }
  m_NetCardMenu.clear();
  for (const auto& i: m_NetSpeedHelper.m_Adapters) {
#ifdef _WIN32
    const auto fName = QString::fromWCharArray(i.friendlyName.data(), i.friendlyName.size());
    auto const action = m_NetCardMenu.addAction(fName);
#else
    auto const action = m_NetCardMenu.addAction(QString::fromUtf8(i.adapterName.data(), i.adapterName.size()));
#endif
    action->setCheckable(true);
    action->setChecked(i.enabled);
#ifdef _WIN32
    action->setToolTip(QString::fromUtf8(i.adapterName.data(), i.adapterName.size()));
#endif
    connect(action, &QAction::triggered, this, std::bind(
      &SpeedBox::UpdateNetCard, this, action, std::placeholders::_1
    ));
  }
}

void SpeedBox::UpdateNetCard(QAction* action, bool checked)
{
  const auto data = action->toolTip().toUtf8();
  const std::u8string_view guid(reinterpret_cast<const char8_t*>(data.data()), data.size());

  if (checked) {
    // 因为 m_AdapterBalckList 使用的是 u8string_view，所以顺序很重要。
    m_NetSpeedHelper.m_AdapterBalckList.erase(guid);
    auto blklst = m_Settings.GetNetCardDisabled();
    blklst.removeByValA(guid);
    m_Settings.SetNetCardDisabled(std::move(blklst));
    mgr->ShowMsg("添加网卡成功！"); // removed from blacklist
  } else {
    auto blklst = m_Settings.GetNetCardDisabled();
    auto iter = blklst.append(guid);
    m_NetSpeedHelper.m_AdapterBalckList.emplace(iter->getValueString());
    m_Settings.SetNetCardDisabled(std::move(blklst));
    mgr->ShowMsg("删除网卡成功！");
  }
  m_NetSpeedHelper.UpdateAdaptersAddresses();
}
