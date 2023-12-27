#pragma once

#include <neobox/pluginobject.h>
#include <yjson/yjson.h>

class WordFish: public PluginObject
{
public:
  explicit WordFish(YJson& settings);
  virtual ~WordFish();
private:
  class QAction* InitMenuAction() override;
  void InitFunctionMap() override;
  YJson& InitSettings(YJson& settings);
private:
  class QAction* m_MainMenuAction;
  class WordFishDlg* m_WordFishDlg;
};
