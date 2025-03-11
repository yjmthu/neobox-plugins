#ifndef SPEEDBOXCFG_H
#define SPEEDBOXCFG_H

#include <neobox/neoconfig.h>

class SpeedBoxCfg: public NeoConfig {
  ConfigConsruct(SpeedBoxCfg)
  CfgBool(ShowForm)
  CfgArray(Position)
  CfgString(PositionSide)
  CfgInt(HideAside)
  CfgInt(ScreenIndex)
  CfgBool(ColorEffect)
  CfgArray(BackgroundColorRgba)
  CfgString(CurSkin)
  CfgObject(UserSkins)
  CfgYJson(NetCardDisabled)
  CfgBool(MousePenetrate)
  CfgBool(ProgressMonitor)
  CfgBool(TaskbarMode)
};

#endif // SPEEDBOXCFG_H