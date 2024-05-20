#ifndef PIFUNCTION_H_INCLUDE_GUARD_
#define PIFUNCTION_H_INCLUDE_GUARD_

template<typename T_>
inline static T_ Pi_Func()
{
#pragma HLS INLINE

#if 1==D_ENABLE_C_SIMULATION_&&1==D_ENABLE_C_SIMULATION_NONSYNTHESIS_CONSTRUCTS_
  return T_ {CV_PI};
#else
  return T_ {3.141592653589793238462f};
#endif
};

#endif
