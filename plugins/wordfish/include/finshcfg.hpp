#pragma once

#include <neobox/neoconfig.h>

class WordFishCfg: public NeoConfig {
  ConfigConsruct(WordFishCfg)
  CfgString(WordFish)
  CfgBool(Prompt)
  CfgBool(IsPaidUser)
  CfgYJson(CityList)
  CfgString(City)
  CfgString(UpdateCycle)
  CfgArray(WindowPosition)
};
