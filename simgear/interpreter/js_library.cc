// ----------------------------------------------------------------------------
//  Description      : Javascript interpreter library
// ----------------------------------------------------------------------------
//  (c) Copyright 2000 by iXiONmedia, all rights reserved.
// ----------------------------------------------------------------------------



#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <ixlib_js_internals.hh>
#include <ixlib_numconv.hh>
#include <ixlib_random.hh>




using namespace ixion;
using namespace javascript;




namespace {
  class eval : public value {
    protected:
      interpreter	&Interpreter;
    
    public:
      value_type getType() const { 
        return VT_FUNCTION; 
        } 
      eval(interpreter &interpreter)
        : Interpreter(interpreter) {
	}
      ref<value> call(parameter_list const &parameters);
    };
    
  class Math : public value_with_methods {
    private:
      typedef value_with_methods super;

    protected:
      float_random	RNG;
      
    public:
      value_type getType() const {
        return VT_BUILTIN;
	}
      
      ref<value> duplicate() const;

      ref<value> lookup(string const &identifier);
      ref<value> callMethod(string const &identifier,parameter_list const &parameters);
    };
  }
  



// eval -----------------------------------------------------------------------
ref<value> 
eval::
call(parameter_list const &parameters) {
  if (parameters.size() != 1) {
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"eval")
    }
  if (parameters[0]->getType() != VT_STRING) return parameters[0];
  return Interpreter.execute(parameters[0]->toString());
  }




// parseInt -------------------------------------------------------------------
IXLIB_JS_DECLARE_FUNCTION(parseInt) {
  if (parameters.size() != 1 && parameters.size() != 2) {
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"parseInt")
    }
  unsigned radix = 10;
  if (parameters.size() == 2)
    radix = parameters[1]->toInt();
  return makeConstant(evalSigned(parameters[0]->toString(),radix));
  }




// parseFloat -----------------------------------------------------------------
IXLIB_JS_DECLARE_FUNCTION(parseFloat) {
  if (parameters.size() != 1) {
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"parseFloat")
    }
  return makeConstant(evalFloat(parameters[0]->toString()));
  }




// isNaN ----------------------------------------------------------------------
#ifdef ADVANCED_MATH_AVAILABLE
IXLIB_JS_DECLARE_FUNCTION(isNaN) {
  if (parameters.size() != 1) {
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"isNaN")
    }
  int classification = fpclassify(parameters[0]->toFloat());
  return makeConstant(classification == FP_NAN);
  }
#endif




// isFinite -------------------------------------------------------------------
#ifdef ADVANCED_MATH_AVAILABLE
IXLIB_JS_DECLARE_FUNCTION(isFinite) {
  if (parameters.size() != 1) {
    EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"isFinite")
    }
  int classification = fpclassify(parameters[0]->toFloat());
  return makeConstant(classification != FP_NAN && classification != FP_INFINITE);
  }
#endif




// Math -----------------------------------------------------------------------
ref<value> Math::duplicate() const {
  // Math is not mutable
  return const_cast<Math *>(this);
  }




ref<value> Math::lookup(string const &identifier) {
  #define MATH_CONSTANT(NAME,VALUE) \
    if (identifier == NAME) return makeConstant(VALUE);
  
  MATH_CONSTANT("E",2.7182818284590452354)
  MATH_CONSTANT("LN10",2.30258509299404568402)
  MATH_CONSTANT("LN2",0.69314718055994530942)
  MATH_CONSTANT("LOG2E",1.4426950408889634074)  
  MATH_CONSTANT("LOG10E,",0.43429448190325182765)
  MATH_CONSTANT("PI",3.14159265358979323846)
  MATH_CONSTANT("SQRT1_2",0.70710678118654752440)
  MATH_CONSTANT("SQRT2",1.41421356237309504880)

  return super::lookup(identifier);
  }




ref<value> Math::callMethod(string const &identifier,parameter_list const &parameters) {
  #define MATH_FUNCTION(NAME,C_NAME) \
    if (identifier == NAME) { \
      if (parameters.size() != 1) { \
        EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"Math." NAME) \
        } \
      return makeConstant(C_NAME(parameters[0]->toFloat())); \
      }
  
  MATH_FUNCTION("abs",NUM_ABS)
  MATH_FUNCTION("acos",acos)
  MATH_FUNCTION("asin",asin)
  MATH_FUNCTION("atan",atan)
  MATH_FUNCTION("ceil",ceil)
  MATH_FUNCTION("cos",cos)
  MATH_FUNCTION("exp",exp)
  MATH_FUNCTION("floor",floor)
  MATH_FUNCTION("log",log)
  #ifdef ADVANCED_MATH_AVAILABLE
  MATH_FUNCTION("round",round)
  #endif
  MATH_FUNCTION("sin",sin)
  MATH_FUNCTION("sqrt",sqrt)
  MATH_FUNCTION("tan",tan)
  if (identifier == "atan2") {
    if (parameters.size() != 2) {
      EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"Math.atan2")
      }
    return makeConstant(atan2(parameters[0]->toFloat(),parameters[1]->toFloat()));
    }
  if (identifier == "pow") {
    if (parameters.size() != 2) {
      EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"Math.pow")
      }
    return makeConstant(pow(parameters[0]->toFloat(),parameters[1]->toFloat()));
    }
  if (identifier == "random") {
    if (parameters.size() != 0) {
      EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"Math.random")
      }
    return makeConstant(RNG());
    }
  // *** FIXME this is non-compliant, but there is no equivalent standard function
  if (identifier == "initRandom") {
    if (parameters.size() >= 2) {
      EXJS_THROWINFO(ECJS_INVALID_NUMBER_OF_ARGUMENTS,"Math.initRandom")
      }
    if (parameters.size() == 0)
      RNG.init();
    else if (parameters.size() == 1)
      RNG.init(parameters[0]->toFloat());
    return makeNull();
    }
  
  // *** FIXME: implement max, min
  EXJS_THROWINFO(ECJS_UNKNOWN_IDENTIFIER,("Math." + identifier).c_str())
  }




// external interface functions -----------------------------------------------
#define ADD_GLOBAL_OBJECT(NAME,TYPE) \
  { ref<value> x = new TYPE(); \
    ip.RootScope->addMember(NAME,x); \
    }
  



void javascript::addGlobal(interpreter &ip) {
  ref<value> ev = new eval(ip);
  ip.RootScope->addMember("eval",ev);

  ADD_GLOBAL_OBJECT("parseInt",parseInt)
  ADD_GLOBAL_OBJECT("parseFloat",parseFloat)
  #ifdef ADVANCED_MATH_AVAILABLE
  ADD_GLOBAL_OBJECT("isNaN",isNaN)
  ADD_GLOBAL_OBJECT("isFinite",isFinite)
  #endif
  
  // *** FIXME hope this is portable
  float zero = 0;
  ip.RootScope->addMember("NaN",makeConstant(0.0/zero));
  ip.RootScope->addMember("Infinity",makeConstant(1.0/zero));
  ip.RootScope->addMember("undefined",makeUndefined());
  }




void javascript::addMath(interpreter &ip) {
  ADD_GLOBAL_OBJECT("Math",Math)
  }




void javascript::addStandardLibrary(interpreter &ip) {
  addGlobal(ip);
  addMath(ip);
  }
