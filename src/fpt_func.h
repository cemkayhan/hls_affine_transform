#ifndef FP_T_FUNC_H_INCLUDE_GUARD_
#define FP_T_FUNC_H_INCLUDE_GUARD_

#include "type_width.h"

inline static float Fpt_Func(
  ap_uint<Type_Width<float>::Value> Param 
){
#pragma HLS INLINE

  const auto Param_ {fp_struct<float> {Param}};
  return Param_.to_float();
}

inline static half Fpt_Func(
  ap_uint<Type_Width<half>::Value> Param 
){
#pragma HLS INLINE
  
  const auto Param_ {fp_struct<half> {Param}};
  return Param_.to_half();
}

#endif
