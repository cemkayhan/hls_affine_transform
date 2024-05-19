#ifndef PIFUNCTION_H_INCLUDE_GUARD_
#define PIFUNCTION_H_INCLUDE_GUARD_

//#include "hls_math.h"

inline static float piFunction(float Param)
{
#pragma HLS INLINE
  //return hls::sinf(Param);
  return sin(Param);
}

inline static half sinFunction(half Param)
{
#pragma HLS INLINE
  //return hls::half_sin(Param);
  return sin(Param);
}

#endif
