#ifndef TRANSLATECFG_H
#define TRANSLATECFG_H

#include <neobox/neoconfig.h>

class TranslateCfg: public NeoConfig
{
  ConfigConsruct(TranslateCfg)
  CfgInt(Mode)
  CfgArray(Size)
  CfgArray(PairBaidu)
  CfgArray(PairYoudao)
  CfgArray(PairGoogle)
  CfgArray(Position)
  CfgYJson(HeightRatio)
  CfgBool(AutoTranslate)
  CfgBool(ReadClipboard)
  CfgBool(AutoMove)
  CfgBool(AutoSize)
  CfgBool(AutoDetect)
};

#endif // TRANSLATECFG_H
