#ifndef BIT_LENGHT_H_INCLUDE_GUARD
#define BIT_LENGHT_H_INCLUDE_GUARD

template<int N_>
struct Bit_Length
{
  enum {Value=1+Bit_Width<N_/2>::Value};
};

template<>
struct Bit_Length<0>
{
  enum {Value=0};
};       

#endif
