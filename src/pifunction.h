#ifndef PIFUNCTION_H_INCLUDE_GUARD_
#define PIFUNCTION_H_INCLUDE_GUARD_

template<typename T_>
inline static T_ Pi_Func()
{
#pragma HLS INLINE

  return T_ {3.141592653589793238462f};
  //return T_ {CV_PI};
};

#endif
