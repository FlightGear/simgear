// ----------------------------------------------------------------------------
//  Description      : Random numbers
// ----------------------------------------------------------------------------
//  (c) Copyright 1996 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------




#ifndef IXLIB_RANDOM
#define IXLIB_RANDOM




#include <cstdlib>
#include <cmath>
#include <ctime>
#include <ixlib_base.hh>
#include <ixlib_numeric.hh>




namespace ixion {
  class float_random {
    double Seed;
  
    public:
    float_random()
      : Seed(1)
      { }
  
    void init() { 
      double current_time = time(NULL);
      Seed = current_time*sin(current_time); 
      }
    void init(double seed)
      { Seed = NUM_ABS(seed); }

    /// Generate one random number in the interval [0,max).
    double operator()(double max = 1) {
      // normalize
      while (Seed > 3) Seed = log(Seed);
      Seed -= floor(Seed);
      Seed = pow(Seed+Pi,8);
      Seed -= floor(Seed);
      return Seed*max;
      }
    };
  
  
  
  
  class int_random {
      float_random	Generator;
  
    public:
      int_random()
        { }
    
      void init()
        { Generator.init(); }
      void init(unsigned seed)
        { Generator.init(seed); }
      
      /// Generate one random number in the interval [0,max).
      unsigned operator()(unsigned max = 32768) {
        unsigned num = rng8() + (rng8() << 7) + (rng8() << 14) + (rng8() << 21) + (rng8() << 28);
        return num % max;
        }
    private:
      TUnsigned8 rng8() {
        return (TUnsigned8) (Generator()*256);
        }
    };
  }




#endif
