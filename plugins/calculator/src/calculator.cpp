#include <calculator.h>

#include <yjson/yjson.h>

Calculator::Calculator(YJson& settings)
  : PluginObject(InitSettings(settings), u8"calculator", u8"科学计算")
{
  InitFunctionMap();
}

Calculator::~Calculator() {}

YJson& Calculator::InitSettings(YJson& settings) {
  if (!settings.isObject()) {
    // settings = YJson::O {};
  }
  return settings;
}

void Calculator::InitFunctionMap() {
  // m_PluginMethod = {};
}

#define PluginName Calculator
#include <neobox/pluginexport.cpp>
