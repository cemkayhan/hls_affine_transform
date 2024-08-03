#ifndef POW2_H_INCLUDE_GUARD_
#define POW2_H_INCLUDE_GUARD_

template<int N_>
struct Pow2
{
  enum {Value=1+Pow2<N_/2>::Value};
};

template<>
struct Pow2<1>
{
  enum {Value=0};
};

#endif
