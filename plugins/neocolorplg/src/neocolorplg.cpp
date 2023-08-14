#include <neocolorplg.h>
#include <yjson.h>
#include <colordlg.hpp>
#include <smallform.hpp>
#include <menubase.hpp>

#include <QAction>
#include <QMenu>

#define PluginName NeoColorPlg

#include <pluginexport.cpp>

PluginName::PluginName(YJson& settings)
  : PluginObject(InitSettings(settings)
  , u8"neocolorplg", u8"颜色拾取")
  , m_Settings(settings)
{
  InitFunctionMap();
}

PluginName::~PluginName()
{
}

QAction* PluginName::InitMenuAction()
{
  return PluginObject::InitMenuAction();
}

YJson& PluginName::InitSettings(YJson& settings)
{
  if (!settings.isObject()) {
    settings = YJson::O {
      { u8"StayTop", false},
      { u8"HistoryMaxCount", 100 },
      { u8"History", YJson::A {}},
    };
  }

  return settings;
}

void PluginName::InitFunctionMap()
{
  m_PluginMethod = {
    {u8"pickColor",
      {u8"拾取颜色", u8"拾取屏幕颜色，拾取成功后打开编辑器。", [this](PluginEvent, void*)
        {
          SmallForm::PickColor(m_Settings);
        }
      , PluginEvent::Void}
    },
    {u8"showEditor",
      {u8"颜色编辑", u8"打开颜色编辑对话框。", [this](PluginEvent, void*)
        {
          if (ColorDlg::m_Instance) {
            ColorDlg::m_Instance->show();
            return;
          }
          auto colDlg = new ColorDlg(m_Settings);
          colDlg->show();
        }
      , PluginEvent::Void}
    },
    {u8"setHistoryMaxCount",
      {u8"历史数量", u8"设置颜色历史记录条数最大值。", [this](PluginEvent, void*)
        {
          auto count = m_Settings.GetHistoryMaxCount();
          auto result = m_MainMenu->GetNewInt("输入数字", "颜色历史的最大数量", 10, 300, count);
          if (!result) {
            return;
          }
          m_Settings.SetHistoryMaxCount(*result);
          mgr->ShowMsg("保存成功");
        }
      , PluginEvent::Void}
    }
  };
}
