#include <neotranslateplg.h>
#include <translatedlg.hpp>
#include <yjson.h>
#include <menubase.hpp>

#include <QPlainTextEdit>
#include <QMimeData>
#include <QDropEvent>

#define PluginName NeoTranslatePlg
#include <pluginexport.cpp>

/*
 * NeoTranslatePlugin
 */

PluginName::PluginName(YJson& settings)
  : PluginObject(InitSettings(settings), u8"neotranslateplg", u8"极简翻译")
  , m_Settings(settings)
  , m_TranslateDlg(new NeoTranslateDlg(m_Settings))
{
  AddMainObject(m_TranslateDlg);
  InitFunctionMap();
  m_Following.push_back({u8"neospeedboxplg", [this](PluginEvent event, void* data){
    if (event == PluginEvent::MouseMove) {
      if (m_TranslateDlg->isVisible())
        m_TranslateDlg->hide();
    } else if (event == PluginEvent::MouseDoubleClick) {
      if (m_TranslateDlg->isVisible())
        m_TranslateDlg->hide();
      else
        m_TranslateDlg->show();
    } else if (event == PluginEvent::Drop) {
      const auto mimeData = reinterpret_cast<QDropEvent*>(data)->mimeData();
      if (mimeData->hasUrls() || !mimeData->hasText()) return;
      const QString text = mimeData->text();
      if (text.isEmpty() || text.isNull()) return;
      auto const txtfrom = m_TranslateDlg->findChild<QPlainTextEdit*>("neoTextFrom");
      txtfrom->setPlainText(text);
      m_TranslateDlg->show();
    }
  }});
  m_Following.push_back({u8"neoocrplg", [this](PluginEvent event, void* data){
    if (event == PluginEvent::U8string) {
      if (!data) return;
      const auto& str = *reinterpret_cast<std::u8string*>(data);
      auto const txtfrom = m_TranslateDlg->findChild<QPlainTextEdit*>("neoTextFrom");
      txtfrom->setPlainText(Utf82QString(str));
      if (m_TranslateDlg->isVisible()) {
        if (m_Settings.GetAutoTranslate()) {
          m_TranslateDlg->GetResultData(str);
        }
      } else {
        m_TranslateDlg->show();
      }
    }
  }});
}

PluginName::~PluginName()
{
  delete m_MainMenuAction;
  delete m_TranslateDlg;  // must delete the dlg when plugin destroying.
}

void PluginName::InitFunctionMap() {
  m_PluginMethod = {
    {u8"toggleVisibility",
      {u8"打开窗口", u8"打开或关闭极简翻译界面", [this](PluginEvent, void* data){
        m_TranslateDlg->ToggleVisibility();
      }, PluginEvent::Void
    }},
    {u8"enableReadClipboard",
      {u8"读剪切板", u8"打开界面自动读取剪切板内容到From区", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Settings.SetReadClipboard(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg("设置成功");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Settings.GetReadClipboard();
        }
      }, PluginEvent::Bool}
    },
    {u8"enableAutoTranslate",
      {u8"自动翻译", u8"打开界面自动翻译From区内容", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Settings.SetAutoTranslate(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg("设置成功");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Settings.GetAutoTranslate();
        }
      }, PluginEvent::Bool}
    },
    {u8"enableAutoMove",
      {u8"吸附窗口", u8"自动吸附窗口贴近网速悬浮窗", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Settings.SetAutoMove(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg("设置成功");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Settings.GetAutoMove();
        }
      }, PluginEvent::Bool}
    },
    {u8"enableAutoSize",
      {u8"默认大小", u8"每次显示界面时调回默认大小", [this](PluginEvent event, void* data) {
        if (event == PluginEvent::Bool) {
          m_Settings.SetAutoSize(*reinterpret_cast<bool*>(data));
          mgr->ShowMsg("设置成功");
        } else if (event == PluginEvent::BoolGet) {
          *reinterpret_cast<bool*>(data) = m_Settings.GetAutoSize();
        }
      }, PluginEvent::Bool}
    },
  };
}

QAction* PluginName::InitMenuAction()
{
  m_MainMenuAction = new QAction("极简翻译");
  PluginObject::InitMenuAction();
  QObject::connect(m_MainMenuAction, &QAction::triggered, m_MainMenu, std::bind(m_PluginMethod[u8"toggleVisibility"].function, PluginEvent::Void, nullptr));
  return m_MainMenuAction;
}

YJson& PluginName::InitSettings(YJson& settings)
{
  if (!settings.isObject()) {
    settings = YJson::O {
      { u8"Version", 0},
      { u8"Mode", 0 },
      { u8"Size", YJson::A { 300, 500 }},
      { u8"PairBaidu", YJson::A {0, 0}},     // 语言组合
      { u8"PairYoudao", YJson::A {0, 0}},    // 语言组合
      { u8"AutoTranslate", false },
      { u8"ReadClipboard", false },
      { u8"AutoMove",      true  },    // 自动移动悬浮窗
      { u8"AutoSize",      true  },    // 自动移动悬浮窗
      { u8"Position", YJson::A { 100, 100 }},
      { u8"HeightRatio", YJson:: A { 180, 180 }},
    };
  }
  auto& version = settings[u8"Version"];
  if (!version.isNumber()) {
    version = 0;
    settings[u8"AutoMove"] = true;
    settings[u8"Position"] = YJson::A { 100, 100 };
    settings[u8"Size"] = YJson::A { 300, 500 };
    settings[u8"AutoSize"] = true;
  }
  if (version.getValueInt() == 0) {
    version = 1;
    settings[u8"PairBaidu"] = YJson::A {0, 0};
    settings[u8"PairYoudao"] = YJson::A {0, 0};
  }
  if (version.getValueInt() == 1) {
    version = 2;
    settings[u8"HeightRatio"] = YJson::A {180, 180};
  }
  if (version.getValueInt() == 2) {
    version = 3;
    settings[u8"HeightRatio"].joinA({180, 180, 180});
  }
  return settings;
  // we may not need to call SaveSettings;
}
