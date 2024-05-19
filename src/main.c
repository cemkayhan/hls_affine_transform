#include "bit_width.h"
#include <iostream>

int main()
{
  std::cout << "8: " << Bit_Width<8>::Value << '\n';
  std::cout << "4: " << Bit_Width<4>::Value << '\n';
  std::cout << "2: " << Bit_Width<2>::Value << '\n';
  std::cout << "1: " << Bit_Width<1>::Value << '\n';
}
