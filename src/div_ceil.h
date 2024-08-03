#ifndef DIV_CEIL_H_INCLUDE_GUARD_
#define DIV_CEIL_H_INCLUDE_GUARD_

template<int M_,int N_>
struct Div_Ceil
{
  enum {Value=1+(M_-1)/N_};
};

#endif
