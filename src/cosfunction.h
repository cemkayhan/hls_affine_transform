#ifndef COSFUNCTION_H_INCLUDE_GUARD_
#define COSFUNCTION_H_INCLUDE_GUARD_

#include "hls_math.h"

inline static float Cos_Func(
  float Param
){
#pragma HLS INLINE

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_NONSYNTHESIS_CONSTRUCTS_
  return cos(Param);
#else
  return hls::cosf(Param);
#endif
}

inline static half Cos_Func(
  half Param
){
#pragma HLS INLINE

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_NONSYNTHESIS_CONSTRUCTS_
  return cos(Param);
#else
  return hls::half_cos(Param);
#endif
}

#endif
