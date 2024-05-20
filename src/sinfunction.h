#ifndef SINFUNCTION_H_INCLUDE_GUARD_
#define SINFUNCTION_H_INCLUDE_GUARD_

#include "hls_math.h"

inline static float Sin_Func(
  float Param
){
#pragma HLS INLINE
  return hls::sinf(Param);
  //return sin(Param);
}

inline static half Sin_Func(
  half Param
){
#pragma HLS INLINE
  return hls::half_sin(Param);
  //return sin(Param);
}

#endif
