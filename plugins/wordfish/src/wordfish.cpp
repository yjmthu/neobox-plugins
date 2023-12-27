#include <wordfish.hpp>

#include <yjson/yjson.h>

#include <QApplication>
#include <QDialog>

WordFish::WordFish(YJson& settings)
  : PluginObject(settings, u8"wordfish", u8"单词小鱼")
  , m_MainMenuAction(nullptr)
  , m_WordFishDlg(nullptr)
{
  InitSettings(settings);
  InitFunctionMap();
  m_MainMenuAction = InitMenuAction();
}

WordFish::~WordFish()
{
  if (m_WordFishDlg) {
  }
}

QAction* WordFish::InitMenuAction()
{
  // QAction* action = new QAction(tr("WordFish"), this);
  // connect(action, &QAction::triggered, this, &WordFish::OnMenuAction);
  return nullptr;
}

void WordFish::InitFunctionMap()
{
  // m_FunctionMap["WordFish"] = [this](const YJson& params) {
  //   if (m_WordFishDlg) {
  //     m_WordFishDlg->close();
  //     delete m_WordFishDlg;
  //   }
  //   m_WordFishDlg = new WordFishDlg(params);
  //   m_WordFishDlg->show();
  // };
}

YJson& WordFish::InitSettings(YJson& settings)
{
  if (!settings[u8"WordFish"].isString()) {
    settings[u8"WordFish"] = u8"WordFish";
  }
  if (!settings[u8"Prompt"].isTrue()) {
    settings[u8"Prompt"] = true;
  }
  if (!settings[u8"IsPaidUser"].isTrue()) {
    settings[u8"IsPaidUser"] = false;
  }
  if (!settings[u8"CityList"].isArray()) {
    settings[u8"CityList"] = YJson::Array;
  }
  if (!settings[u8"City"].isString()) {
    settings[u8"City"] = "Shanghai";
  }
  if (!settings[u8"UpdateCycle"].isString()) {
    settings[u8"UpdateCycle"] = "1h";
  }
  if (!settings[u8"WindowPosition"].isArray()) {
    settings[u8"WindowPosition"] = YJson::A {};
  }
  return settings;
}

// void WordFish::OnMenuAction()
// {
//   if (m_WordFishDlg) {
//     m_WordFishDlg->close();
//     delete m_WordFishDlg;
//   }
//   m_WordFishDlg = new WordFishDlg(m_Settings);
//   m_WordFishDlg->show();
// }

// Path: plugins/wordfish/src/wordfishdlg.cpp
