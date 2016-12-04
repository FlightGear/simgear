
#ifndef SG_MISC_TEST_MACROS_HXX
#define SG_MISC_TEST_MACROS_HXX

#include <cmath> // for fabs()

#define SG_CHECK_EQUAL(a, b) \
    if ((a) != (b))  { \
        std::cerr << "failed:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_VERIFY(a) \
    if (!(a))  { \
        std::cerr << "failed:" << #a << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL_EP(a, b) \
    if (fabs(a - b) > SG_EPSILON)  { \
        std::cerr << "failed with epsilon:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL_EP2(a, b, ep) \
    if (fabs(a - b) > ep)  { \
        std::cerr << "failed with epsilon:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_TEST_FAIL(msg) \
  std::cerr << "failure:" << msg; \
  exit(1);


#endif // of SG_MISC_TEST_MACROS_HXX
