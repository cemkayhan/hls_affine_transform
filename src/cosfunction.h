#ifndef COSFUNCTION_H_INCLUDE_GUARD_
#define COSFUNCTION_H_INCLUDE_GUARD_

#include "hls_math.h"

inline static float Cos_Func(
  float Param
){
#pragma HLS INLINE
  return hls::cosf(Param);
  //return cos(Param);
}

inline static half Cos_Func(
  half Param
){
#pragma HLS INLINE
  return hls::half_cos(Param);
  //return cos(Param);
}

#endif
