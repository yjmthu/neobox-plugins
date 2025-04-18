#include <neobox/neomenu.hpp>
#include <neobox/neoconfig.h>
#include <neobox/menubase.hpp>
// #include <neobox/pluginmgr.h>
#include <tunet.hpp>
#include <portal.h>
#include <neobox/neotimer.h>

#include <QAction>
#include <QEventLoop>
#include <QMessageBox>

#define PluginName Thunet
#include <neobox/pluginexport.cpp>

class TuNetCfg: public NeoConfig {
  ConfigConsruct(TuNetCfg)
  CfgString(Username)
  CfgString(Password)
  CfgBool(AutoLogin)
  CfgInt(Delay)
};

PluginName::PluginName(YJson& settings)
  : QObject()
  , PluginObject(InitSettings(settings), u8"tunet", u8"清华校园网")
  , m_Settings(new TuNetCfg(settings))
  , m_Portal(new Portal())
  , m_MainMenuAction(nullptr)
  , m_Timer(NeoTimer::New())
{
  // LoadResources();
  InitFunctionMap();

  if (m_Settings->GetAutoLogin()) {
    auto delay = m_Settings->GetDelay();
    delay = delay < 0 ? 0 : delay;
    auto cb = [this] { LogInOut(true, true).get(); };
    // NeoTimer::SingleShot(std::chrono::seconds(delay), cb);
    m_Timer->StartOnce(std::chrono::seconds(delay), cb);
  }
}

PluginName::~PluginName()
{
  m_Timer->Destroy();
  delete m_MainMenuAction;
  delete m_Portal;
  delete m_Settings;
}

void PluginName::InitFunctionMap() {
  m_PluginMethod = {
    {u8"login",
      {u8"登录网络", u8"登录清华大学校园网", [this](PluginEvent, void*) {
        QEventLoop loop;
        connect(this, &PluginName::LoginFinished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
        auto action = LogInOut(true, false);
        action.then([this] { emit LoginFinished(); });
        loop.exec();
      }, PluginEvent::Void},
    },
    {u8"logout",
      {u8"登出网络", u8"登出清华大学校园网", [this](PluginEvent, void*) {
        QEventLoop loop;
        connect(this, &PluginName::LogoutFinished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
        // avoid using 'auto&& = ' to receive the return value of 'then'
        auto action = LogInOut(false, false);
        action.then([this] { emit LogoutFinished(); });
        loop.exec();
      }, PluginEvent::Void},
    },
    {u8"autoLogin",
      {u8"自动登录", u8"自动登录清华大学校园网", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          auto const on = *reinterpret_cast<bool*>(data);
          if (!on) m_Timer->Expire();
          m_Settings->SetAutoLogin(on);
          mgr->ShowMsg("设置成功！");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Settings->GetAutoLogin();
        }
      }, PluginEvent::Bool},
    },
    {u8"setUsername",
      {u8"设置用户名", u8"设置清华大学校园网用户名", [this](PluginEvent, void*) {
        auto const username = m_Settings->GetUsername();
        auto result = mgr->m_Menu->GetNewU8String("输入", "输入用户名", username);
        if (result == std::nullopt) return;
        m_Settings->SetUsername(*result);
        mgr->ShowMsg("保存成功！");
      }, PluginEvent::Void},
    },
    {u8"setPassword",
      {u8"设置密码", u8"设置清华大学校园网密码", [this](PluginEvent, void*) {
        auto const password = m_Settings->GetPassword();
        auto result = mgr->m_Menu->GetNewU8String("输入", "输入密码", password);
        if (result == std::nullopt) return;
        m_Settings->SetPassword(*result);
        mgr->ShowMsg("保存成功！");
      }, PluginEvent::Void},
    },
    {u8"setDelay",
      {u8"设置延迟", u8"设置启动时登录延迟时间", [this](PluginEvent, void*) {
        auto const delay = m_Settings->GetDelay();
        auto result = mgr->m_Menu->GetNewInt("输入", "输入延迟时间", 0, 90, delay);
        if (result == std::nullopt) return;
        m_Timer->Expire();
        m_Settings->SetDelay(*result);
        mgr->ShowMsg("保存成功！");
      }, PluginEvent::Void},
    },
  };
}

YJson& PluginName::InitSettings(YJson& settings) {
  if (settings.isObject()) {
    return settings;
  }
  return settings = YJson::O {
    { u8"Username", YJson::String },
    { u8"Password", YJson::String },
    { u8"AutoLogin", false },
    { u8"Delay", 3 },
  };
}

QAction* PluginName::LoadMainMenuAction()
{
  auto names = {u8"login", u8"logout"};

  m_MainMenuAction = new QAction("校园网络");
  auto const menu = new MenuBase(m_MainMenu);
  m_MainMenuAction->setMenu(menu);

  for (const auto& name: names) {
    const auto& info = m_PluginMethod[name];
    auto const action = menu->addAction(
          Utf82QString(info.friendlyName));
    action->setToolTip(PluginObject::Utf82QString(info.description));
    QObject::connect(action, &QAction::triggered, menu, std::bind(info.function, PluginEvent::Void, nullptr));
    m_PluginMethod[name] = std::move(info);
  }

  return m_MainMenuAction;
}

QAction* PluginName::InitMenuAction()
{
  this->PluginObject::InitMenuAction();
  return LoadMainMenuAction();
}

void PluginName::ShowMsg(Portal::Error err, const char* showSucc) {
  switch (err) {
  case Portal::Error::NoError:
    if (showSucc) mgr->ShowMsg(showSucc);
    break;
  case Portal::Error::AlreadyLogin:
    if (showSucc) mgr->ShowMsg("已经登录");
    break;
  case Portal::Error::AlreadyLogout:
    if (showSucc) mgr->ShowMsg("已经注销");
    break;
  case Portal::Error::NetworkError:
    mgr->ShowMsg("网络错误");
    break;
  case Portal::Error::UserInfoError:
    mgr->ShowMsg("用户信息错误");
    break;
  case Portal::Error::ParseError:
    mgr->ShowMsg("解析错误");
    break;
  case Portal::Error::TokenError:
    mgr->ShowMsg("获取Token失败");
    break;
  case Portal::Error::AuthError:
    mgr->ShowMsg("认证失败");
    break;
  default:
    mgr->ShowMsg("其他错误");
  }
}

AsyncVoid PluginName::LogInOut(bool login, bool silent) {
  static bool isRunning = false;
  if (isRunning) {
    mgr->ShowMsg("正在运行中，请稍后");
    co_return;
  }
  struct Guard {
    Guard() { isRunning = true; }
    ~Guard() { isRunning = false; }
  } guard;

  std::optional<Portal::Error> res = std::nullopt;

  res = co_await m_Portal->Init(m_Settings->GetUsername(), m_Settings->GetPassword()).awaiter();

  if (res == std::nullopt) {
    mgr->ShowMsg("协程运行失败，请联系开发者");
    co_return;
  }

  ShowMsg(*res, nullptr);
  if (*res != Portal::NoError) {
#ifdef _DEBUG
    std::cout << "State 1 error: " << static_cast<int>(*res) << std::endl;
#endif
    co_return;
  }

  if (login)
    res = co_await m_Portal->Login().awaiter();
  else
    res = co_await m_Portal->Logout().awaiter();
  
  if (res == std::nullopt) {
    mgr->ShowMsg(login ? "登入失败，请联系开发者" : "登出失败，请联系开发者");
    co_return;
  }

  ShowMsg(*res, silent ? nullptr : (login ? "登入成功" : "登出成功"));
}
