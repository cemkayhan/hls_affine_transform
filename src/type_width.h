#ifndef TYPE_WIDTH_H_INCLUDE_GUARD_
#define TYPE_WIDTH_H_INCLUDE_GUARD_

template<typename T_>
struct Type_Width
{
};

template<>
struct Type_Width<half>
{
  enum {Value=16};
};

template<>
struct Type_Width<float>
{
  enum {Value=32};
};

#endif
