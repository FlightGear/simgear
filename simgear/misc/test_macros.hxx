#ifndef SG_MISC_TEST_MACROS_HXX
#define SG_MISC_TEST_MACROS_HXX

#include <iostream>
#include <cmath>                // for std::fabs()


#define SG_VERIFY(a) \
    if (!(a))  { \
        std::cerr << "failed: " << #a << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL(a, b) \
    if ( !((a) == (b)) )  { \
        std::cerr << "failed: " << #a << " == " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL_EP(a, b) \
    if (std::fabs((a) - (b)) > SG_EPSILON)  { \
        std::cerr << "failed: " << #a << " ~= " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL_EP2(a, b, ep) \
    if (std::fabs((a) - (b)) > ep)  { \
        std::cerr << "failed: " << #a << " ~= " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_NE(a, b) \
    if ( !((a) != (b)) )  { \
        std::cerr << "failed: " << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_LT(a, b) \
    if ( !((a) < (b)) )  { \
        std::cerr << "failed: " << #a << " < " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_LE(a, b) \
    if ( !((a) <= (b)) )  { \
        std::cerr << "failed: " << #a << " <= " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_GT(a, b) \
    if ( !((a) > (b)) )  { \
        std::cerr << "failed: " << #a << " > " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_GE(a, b) \
    if ( !((a) >= (b)) )  { \
        std::cerr << "failed: " << #a << " >= " << #b << std::endl; \
        std::cerr << "\tgot '" << a << "' and '" << b << "'" << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_TEST_FAIL(msg) \
  std::cerr << "failure: " << msg; \
  exit(1);


#endif // of SG_MISC_TEST_MACROS_HXX
