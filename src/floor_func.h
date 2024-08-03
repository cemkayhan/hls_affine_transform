#ifndef FLOOR_FUNC_H_INCLUDE_GUARD_
#define FLOOR_FUNC_H_INCLUDE_GUARD_

#include "hls_math.h"

inline static float Floor_Func(
  float Param
){
#pragma HLS INLINE

  return hls::floorf(Param);
}

inline static half Fpt_Func(
  half Param
){
#pragma HLS INLINE
  
  return hls::half_floor(Param);
}

#endif
