#include <math.h>
#include <string.h>

#include "nasal.h"

static naRef f_sin(naContext c, naRef args)
{
    naRef a = naNumValue(naVec_get(args, 0));
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to sin()");
    a.num = sin(a.num);
    return a;
}

static naRef f_cos(naContext c, naRef args)
{
    naRef a = naNumValue(naVec_get(args, 0));
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to cos()");
    a.num = cos(a.num);
    return a;
}

static naRef f_exp(naContext c, naRef args)
{
    naRef a = naNumValue(naVec_get(args, 0));
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to exp()");
    a.num = exp(a.num);
    return a;
}

static naRef f_ln(naContext c, naRef args)
{
    naRef a = naNumValue(naVec_get(args, 0));
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to ln()");
    a.num = log(a.num);
    return a;
}

static naRef f_sqrt(naContext c, naRef args)
{
    naRef a = naNumValue(naVec_get(args, 0));
    if(naIsNil(a))
        naRuntimeError(c, "non numeric argument to sqrt()");
    a.num = sqrt(a.num);
    return a;
}

static naRef f_atan2(naContext c, naRef args)
{
    naRef a = naNumValue(naVec_get(args, 0));
    naRef b = naNumValue(naVec_get(args, 1));
    if(naIsNil(a) || naIsNil(b))
        naRuntimeError(c, "non numeric argument to atan2()");
    a.num = atan2(a.num, b.num);
    return a;
}

static struct func { char* name; naCFunction func; } funcs[] = {
    { "sin", f_sin },
    { "cos", f_cos },
    { "exp", f_exp },
    { "ln", f_ln },
    { "sqrt", f_sqrt },
    { "atan2", f_atan2 },
};

naRef naMathLib(naContext c)
{
    naRef name, namespace = naNewHash(c);
    int i, n = sizeof(funcs)/sizeof(struct func);
    for(i=0; i<n; i++) {
        naRef code = naNewCCode(c, funcs[i].func);
        naRef name = naStr_fromdata(naNewString(c),
                                    funcs[i].name, strlen(funcs[i].name));
        naHash_set(namespace, name, naNewFunc(c, code));
    }

    // Set up constants for math.pi and math.e
    name = naStr_fromdata(naNewString(c), "pi", 2);
    naHash_set(namespace, name, naNum(M_PI));

    name = naStr_fromdata(naNewString(c), "e", 1);
    naHash_set(namespace, name, naNum(M_E));

    return namespace;
}
