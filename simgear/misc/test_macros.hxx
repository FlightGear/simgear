
#ifndef SG_MISC_TEST_MACROS_HXX
#define SG_MISC_TEST_MACROS_HXX

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        std::cerr << "failed:" << #a << " != " << #b << std::endl; \
        std::cerr << "\tgot:'" << a << "'" << std::endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        std::cerr << "failed:" << #a << std::endl; \
        exit(1); \
    }
    
    
#endif // of SG_MISC_TEST_MACROS_HXX
