#include <neobox/neomenu.hpp>
#include <neobox/neoconfig.h>
#include <neobox/menubase.hpp>
// #include <neobox/pluginmgr.h>
#include <tunet.h>
#include <portal.h>

#include <QAction>

#define PluginName Thunet
#include <neobox/pluginexport.cpp>

class TuNetCfg: public NeoConfig {
  ConfigConsruct(TuNetCfg)
  CfgString(Username)
  CfgString(Password)
  CfgBool(AutoLogin)
};

PluginName::PluginName(YJson& settings)
  : PluginObject(InitSettings(settings), u8"tunet", u8"清华校园网")
  , m_Settings(new TuNetCfg(settings))
  , m_Portal(new Portal())
  , m_MainMenuAction(nullptr)
{
  // LoadResources();
  InitFunctionMap();

  m_Portal->Init(m_Settings->GetUsername(), m_Settings->GetPassword()).then([this](auto err) {
    if (err == std::nullopt) {
      mgr->ShowMsg("校园网插件初始化失败，请联系插件开发者。");
    } else if (*err != Portal::Error::NoError) {
      std::cerr << "Init error: " << static_cast<int>(*err) << std::endl;
    } else {
      m_Inited = true;
    }
  }).get();
}

PluginName::~PluginName()
{
  delete m_MainMenuAction;
  delete m_Portal;
  delete m_Settings;
}

void PluginName::InitFunctionMap() {
  m_PluginMethod = {
    {u8"login",
      {u8"登录网络", u8"登录清华大学校园网", [this](PluginEvent, void*) {
        if (!m_Inited) {
          mgr->ShowMsg("请等待初始化完成。");
          return;
        }
        m_Portal->Login().then([this](auto err) {
          if (err == std::nullopt) {
            mgr->ShowMsg("登录失败，请联系插件开发者。");
          } else if (*err != Portal::Error::NoError) {
            std::cerr << "Login error: " << static_cast<int>(*err) << std::endl;
            switch (*err) {
            case Portal::Error::AlreadyLogin:
              mgr->ShowMsg("已经登录。");
              break;
            case Portal::Error::NetworkError:
              mgr->ShowMsg("网络错误。");
              break;
            case Portal::Error::UserInfoError:
              mgr->ShowMsg("用户信息错误。");
              break;
            case Portal::Error::ParseError:
              mgr->ShowMsg("解析错误。");
              break;
            case Portal::Error::TokenError:
              mgr->ShowMsg("获取Token失败。");
              break;
            case Portal::Error::AuthError:
              mgr->ShowMsg("认证失败。");
              break;
            default:
              mgr->ShowMsg("其他错误");
            }
          }
        });
      }, PluginEvent::Void},
    },
    {u8"logout",
      {u8"登出网络", u8"登出清华大学校园网", [this](PluginEvent, void*) {
        if (!m_Inited) {
          mgr->ShowMsg("请等待初始化完成。");
          return;
        }
        m_Portal->Logout().then([this](auto err) {
          if (err == std::nullopt) {
            mgr->ShowMsg("注销失败，请联系插件开发者。");
          } else if (*err != Portal::Error::NoError) {
            switch (*err) {
            case Portal::Error::AlreadyLogout:
              mgr->ShowMsg("已经注销。");
              break;
            case Portal::Error::NetworkError:
              mgr->ShowMsg("网络错误。");
              break;
            case Portal::Error::UserInfoError:
              mgr->ShowMsg("用户信息错误。");
              break;
            case Portal::Error::ParseError:
              mgr->ShowMsg("解析错误。");
              break;
            case Portal::Error::TokenError:
              mgr->ShowMsg("获取Token失败。");
              break;
            case Portal::Error::AuthError:
              mgr->ShowMsg("认证失败。");
              break;
            default:
              mgr->ShowMsg("其他错误");
            }
          } else {
            mgr->ShowMsg("登出成功。");
          }
        });
      }, PluginEvent::Void},
    },
    {u8"autoLogin",
      {u8"自动登录", u8"自动登录清华大学校园网", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Settings->SetAutoLogin(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg(u8"设置成功！");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Settings->GetAutoLogin();
        }
      }, PluginEvent::Bool},
    },
    {u8"setUsername",
      {u8"设置用户名", u8"设置清华大学校园网用户名", [this](PluginEvent, void*) {
        auto const username = m_Settings->GetUsername();
        auto result = mgr->m_Menu->GetNewU8String("输入", "输入用户名", username);
        if (!result) return;

        if (*result != username) {
          m_Settings->SetUsername(*result);
        }
        mgr->ShowMsg("保存成功！");
      }, PluginEvent::String},
    },
    {u8"setPassword",
      {u8"设置密码", u8"设置清华大学校园网密码", [this](PluginEvent, void*) {
        auto const password = m_Settings->GetPassword();
        auto result = mgr->m_Menu->GetNewU8String("输入", "输入密码", password);
        if (!result) return;

        if (*result != password) {
          m_Settings->SetPassword(*result);
        }
        mgr->ShowMsg("保存成功！");
      }, PluginEvent::String},
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
  };
}

QAction* PluginName::LoadMainMenuAction()
{
  auto names = {u8"login", u8"logout"};

  m_MainMenuAction = new QAction("清华校园网");
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

