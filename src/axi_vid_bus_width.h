#ifndef AXI_VID_BUS_WIDTH_H_INCLUDE_GUARD_
#define AXI_VID_BUS_WIDTH_H_INCLUDE_GUARD_

#include "div_ceil.h"

template<int COLOR_CHANNELS_,int DEPTH_,int PPC_>
struct Axi_Vid_Bus_Width
{
  enum {Value=8*Div_Ceil<COLOR_CHANNELS_*DEPTH_*PPC_,8>::Value};
};

#endif
