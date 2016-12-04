/*
 * easing_functions_test.cxx
 *
 * Output values of all easing functions for plotting and some simple tests.
 *
 *  Created on: 15.03.2013
 *      Author: tom
 */

#include "easing_functions.hxx"
#include <cmath>
#include <iostream>

#include <simgear/misc/test_macros.hxx>

#define VERIFY_CLOSE(a, b) SG_CHECK_EQUAL_EP2((a), (b), 1e-5)


int main(int argc, char* argv[])
{
  using simgear::easing_functions;

  for( double t = 0; t <= 1; t += 1/32. )
  {
    if( t == 0 )
    {
      for( size_t i = 0; easing_functions[i].name; ++i )
        std::cout << easing_functions[i].name << " ";
      std::cout << '\n';
    }

    for( size_t i = 0; easing_functions[i].name; ++i )
    {
      double val = (*easing_functions[i].func)(t);
      std::cout << val << " ";

      if( t == 0 )
      {
        VERIFY_CLOSE(val, 0)
      }
      else if( t == 1 )
      {
        VERIFY_CLOSE(val, 1)
      }
    }
    std::cout << '\n';
  }

  return 0;
}
