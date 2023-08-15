#pragma once

#include <neobox/pluginobject.h>

class Calculator: public PluginObject {
public:
  explicit Calculator(YJson& settings);
  virtual ~Calculator();
private:
  static YJson& InitSettings(YJson& settings);
private:
  void InitFunctionMap() override;
};
