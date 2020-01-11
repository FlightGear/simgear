#include <math.h>
#include <string.h>

#include "nasal.h"

// Toss a runtime error for any NaN or Inf values produced.  Note that
// this assumes an IEEE 754 format.
#define VALIDATE(r) (valid(r.num) ? (r) : die(c, __FUNCTION__+2))

static int valid(double d)
{
    return isfinite(d);
}

static naRef die(naContext c, const char* fn)
{
    naRuntimeError(c, "floating point error in math.%s()", fn);
    return naNil();
}

static naRef f_sin(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to sin()");
    a.num = sin(a.num);
    return VALIDATE(a);
}

static naRef f_cos(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to cos()");
    a.num = cos(a.num);
    return VALIDATE(a);
}

static naRef f_exp(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to exp()");
    a.num = exp(a.num);
    return VALIDATE(a);
}

static naRef f_ln(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to ln()");
    a.num = log(a.num);
    return VALIDATE(a);
}

static naRef f_sqrt(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to sqrt()");
    a.num = sqrt(a.num);
    return VALIDATE(a);
}

static naRef f_pow(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    naRef b = naNumValue(argc > 1 ? args[1] : naNil());
    if(naIsNil(a) || naIsNil(b))
        naRuntimeError(c, "non numeric argument to pow()");
    a.num = pow(a.num, b.num);
    return VALIDATE(a);
}

static naRef f_atan2(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    naRef b = naNumValue(argc > 1 ? args[1] : naNil());
    if(naIsNil(a) || naIsNil(b))
        naRuntimeError(c, "non numeric argument to atan2()");
    a.num = atan2(a.num, b.num);
    return VALIDATE(a);
}

static naRef f_floor(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to floor()");
    a.num = floor(a.num);
    return VALIDATE(a);
}

static naRef f_ceil(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to ceil()");
    a.num = ceil(a.num);
    return VALIDATE(a);
}

static naRef f_fmod(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    naRef b = naNumValue(argc > 1 ? args[1] : naNil());
    if(naIsNil(a) || naIsNil(b))
        naRuntimeError(c, "non numeric arguments to fmod()");

    a.num = fmod(a.num, b.num);
    return VALIDATE(a);
}

static naRef f_clamp(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    naRef min = naNumValue(argc > 1 ? args[1] : naNil());
    naRef max = naNumValue(argc > 2 ? args[2] : naNil());

    if(naIsNil(a) || naIsNil(min) || naIsNil(max))
        naRuntimeError(c, "non numeric arguments to clamp()");

    a.num = a.num < min.num ? min.num : ( a.num > max.num ? max.num : a.num );
    return VALIDATE(a);
}

static naRef f_periodic(naContext c, naRef me, int argc, naRef* args)
{
    double range;
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    naRef b = naNumValue(argc > 1 ? args[1] : naNil());
    naRef x = naNumValue(argc > 2 ? args[2] : naNil());

    if(naIsNil(a) || naIsNil(b) || naIsNil(x))
        naRuntimeError(c, "non numeric arguments to periodic()");

    range = b.num - a.num;
    x.num = x.num - range*floor((x.num - a.num)/range);
    // two security checks that can only happen due to roundoff
    if (x.num <= a.num)
        x.num = a.num;
    if (b.num <= x.num)
        x.num = b.num;
    return VALIDATE(x);

//    x.num = SGMiscd::normalizePeriodic(a, b, x);
    return VALIDATE(x);
}

static naRef f_round(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    naRef b = naNumValue(argc > 1 ? args[1] : naNil());
#ifdef _MSC_VER
    double x,y;
#endif
    if(naIsNil(a))
        naRuntimeError(c, "non numeric arguments to round()");
    if (naIsNil(b))
        b.num = 1.0;

#ifdef _MSC_VER // MSVC is not C99-compatible, no round() in math.h
    y = a.num / b.num;
    x = floor(y + 0.5);
#else
    double x = round(a.num / b.num);
#endif
    a.num = x * b.num;

    return VALIDATE(a);
}


static naRef f_tan(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric arguments to tan()");

   a.num = tan(a.num);
   return VALIDATE(a);
}

static naRef f_atan(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric arguments to tan()");

   a.num = atan(a.num);
   return VALIDATE(a);
}

static naRef f_asin(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to asin()");
    a.num = asin(a.num);
    return VALIDATE(a);
}

static naRef f_acos(naContext c, naRef me, int argc, naRef* args)
{
    naRef a = naNumValue(argc > 0 ? args[0] : naNil());
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to acos()");
    a.num = acos(a.num);
    return VALIDATE(a);
}

static naCFuncItem funcs[] = {
    { "sin", f_sin },
    { "cos", f_cos },
    { "exp", f_exp },
    { "ln", f_ln },
    { "pow", f_pow },
    { "sqrt", f_sqrt },
    { "atan2", f_atan2 },
    { "atan", f_atan },
    { "floor", f_floor },
    { "ceil", f_ceil },
    { "fmod", f_fmod },
    { "clamp", f_clamp },
    { "periodic", f_periodic },
    { "round", f_round },
    { "tan", f_tan },
    { "acos", f_acos },
    { "asin", f_asin },
    { 0 }
};



naRef naInit_math(naContext c)
{
    naRef ns = naGenLib(c, funcs);
    naAddSym(c, ns, "pi", naNum(3.14159265358979323846));
    naAddSym(c, ns, "e", naNum(2.7182818284590452354));
    return ns;
}
