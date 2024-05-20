#ifndef SINFUNCTION_H_INCLUDE_GUARD_
#define SINFUNCTION_H_INCLUDE_GUARD_

#include "hls_math.h"

inline static float Sin_Func(
  float Param
){
#pragma HLS INLINE
#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_NONSYNTHESIS_CONSTRUCTS_
  return sin(Param);
#else
  return hls::sinf(Param);
#endif
}

inline static half Sin_Func(
  half Param
){
#pragma HLS INLINE

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_NONSYNTHESIS_CONSTRUCTS_
  return sin(Param);
#else
  return hls::half_sin(Param);
#endif
}

#endif
