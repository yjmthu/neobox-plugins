#include <neobox/neoconfig.h>
#include <neobox/menubase.hpp>
#include <tunet.h>
#include <portal.h>

#include <QAction>

#define PluginName Thunet
#include <neobox/pluginexport.cpp>

class TuNetCfg: public NeoConfig {
  ConfigConsruct(TuNetCfg)
  CfgString(Username)
  CfgString(Password)
};

PluginName::PluginName(YJson& settings)
  : PluginObject(InitSettings(settings), u8"tunet", u8"清华校园网")
  , m_Settings(new TuNetCfg(settings))
  , m_Portal(new Portal())
  , m_MainMenuAction(nullptr)
{
  // LoadResources();
  InitFunctionMap();
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
      {u8"登录", u8"登录清华校园网", [](PluginEvent, void*) {
        // login
      }, PluginEvent::Void},
    },
    {u8"logout",
      {u8"注销", u8"注销清华校园网", [](PluginEvent, void*) {
        // logout
      }, PluginEvent::Void},
    },
  };
}

YJson& PluginName::InitSettings(YJson& settings) {
  return settings = YJson::O
  {
    {u8"username", u8"2018000000"},
    {u8"password", u8"123456"},
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

