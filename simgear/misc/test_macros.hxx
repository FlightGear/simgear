
#ifndef SG_MISC_TEST_MACROS_HXX
#define SG_MISC_TEST_MACROS_HXX

#include <cmath> // for fabs()

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        std::cerr << "failed:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        std::cerr << "failed:" << #a << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define COMPARE_EP(a, b) \
    if (fabs(a - b) > SG_EPSILON)  { \
        std::cerr << "failed with epsilon:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define COMPARE_EP2(a, b, ep) \
    if (fabs(a - b) > ep)  { \
        std::cerr << "failed with epsilon:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        std::cerr << "\tat:" << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#endif // of SG_MISC_TEST_MACROS_HXX
