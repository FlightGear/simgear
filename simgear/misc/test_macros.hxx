// -*- coding: utf-8 -*-

#ifndef SG_MISC_TEST_MACROS_HXX
#define SG_MISC_TEST_MACROS_HXX

#include <iostream>
#include <cmath>                // for std::fabs()


// Assertion-like test
#define SG_VERIFY(a) \
    if (!(a))  { \
        std::cerr << "failed: " << #a << std::endl; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

// Internal macro (implementation detail). 'a' and 'b' don't always support
// operator<<. This is why we use (a0) and (b0) after '<<', otherwise in such
// cases the test couldn't be compiled, even if 'stream' is false.
#define SG_CHECK_EQUAL0(a, b, stream, a0, b0) \
    if ( !((a) == (b)) )  { \
        std::cerr << "failed: " << #a << " == " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

// Test macros for user consumption. The _NOSTREAM variant is for cases where
// 'a' or 'b' doesn't support operator<< (or where using it would have
// undesirable side effects).
#define SG_CHECK_EQUAL(a, b) SG_CHECK_EQUAL0((a), (b), true, (a), (b))
#define SG_CHECK_EQUAL_NOSTREAM(a, b) SG_CHECK_EQUAL0((a), (b), false, "", "")

// “Approximate equality” test (EP stands for “epsilon”)
#define SG_CHECK_EQUAL_EP0(a, b, stream, a0, b0) \
    if (std::fabs((a) - (b)) > SG_EPSILON)  { \
        std::cerr << "failed: " << #a << " ~= " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL_EP(a, b) SG_CHECK_EQUAL_EP0((a), (b), true, (a), (b))
#define SG_CHECK_EQUAL_EP_NOSTREAM(a, b) \
    SG_CHECK_EQUAL_EP0((a), (b), false, "", "")

// Same as SG_CHECK_EQUAL_EP*, except the “epsilon” can be chosen by the caller
#define SG_CHECK_EQUAL_EP2_0(a, b, ep, stream, a0, b0) \
  if (std::fabs((a) - (b)) > (ep))  {                  \
        std::cerr << "failed: " << #a << " ~= " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_EQUAL_EP2(a, b, ep) \
    SG_CHECK_EQUAL_EP2_0((a), (b), ep, true, (a), (b))
#define SG_CHECK_EQUAL_EP2_NOSTREAM(a, b) \
    SG_CHECK_EQUAL_EP2_0((a), (b), ep, false, "", "")

// “Non-equal” test
#define SG_CHECK_NE0(a, b, stream, a0, b0) \
    if ( !((a) != (b)) )  { \
        std::cerr << "failed: " << #a << " != " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_NE(a, b) SG_CHECK_NE0((a), (b), true, (a), (b))
#define SG_CHECK_NE_NOSTREAM(a, b) SG_CHECK_NE0((a), (b), false, "", "")

// “Lower than” test
#define SG_CHECK_LT0(a, b, stream, a0, b0) \
    if ( !((a) < (b)) )  { \
        std::cerr << "failed: " << #a << " < " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_LT(a, b) SG_CHECK_LT0((a), (b), true, (a), (b))
#define SG_CHECK_LT_NOSTREAM(a, b) SG_CHECK_LT0((a), (b), false, "", "")

// “Lower than or equal” test
#define SG_CHECK_LE0(a, b, stream, a0, b0) \
    if ( !((a) <= (b)) )  { \
        std::cerr << "failed: " << #a << " <= " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_LE(a, b) SG_CHECK_LE0((a), (b), true, (a), (b))
#define SG_CHECK_LE_NOSTREAM(a, b) SG_CHECK_LE0((a), (b), false, "", "")

// “Greater than” test
#define SG_CHECK_GT0(a, b, stream, a0, b0) \
    if ( !((a) > (b)) )  { \
        std::cerr << "failed: " << #a << " > " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_GT(a, b) SG_CHECK_GT0((a), (b), true, (a), (b))
#define SG_CHECK_GT_NOSTREAM(a, b) SG_CHECK_GT0((a), (b), false, "", "")

// “Greater than or equal” test
#define SG_CHECK_GE0(a, b, stream, a0, b0) \
    if ( !((a) >= (b)) )  { \
        std::cerr << "failed: " << #a << " >= " << #b << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "' and '" << (b0) << "'" \
                    << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_GE(a, b) SG_CHECK_GE0((a), (b), true, (a), (b))
#define SG_CHECK_GE_NOSTREAM(a, b) SG_CHECK_GE0((a), (b), false, "", "")

// “Is NULL” test
#define SG_CHECK_IS_NULL0(a, stream, a0) \
    if ( !((a) == nullptr) )  { \
        std::cerr << "failed: " << #a << " == nullptr" << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "'" << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_IS_NULL(a) SG_CHECK_IS_NULL0((a), true, (a))
#define SG_CHECK_IS_NULL_NOSTREAM(a) SG_CHECK_IS_NULL0((a), false, "")

// “Is not NULL” test
#define SG_CHECK_IS_NOT_NULL0(a, stream, a0) \
    if ( !((a) != nullptr) )  { \
        std::cerr << "failed: " << #a << " != nullptr" << std::endl; \
        if (stream) { \
          std::cerr << "\tgot '" << (a0) << "'" << std::endl; \
        }; \
        std::cerr << "\tat " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    }

#define SG_CHECK_IS_NOT_NULL(a) SG_CHECK_IS_NOT_NULL0((a), true, (a))
#define SG_CHECK_IS_NOT_NULL_NOSTREAM(a) SG_CHECK_IS_NOT_NULL0((a), false, "")

// “Always failing” test
#define SG_TEST_FAIL(msg) \
  std::cerr << "failure: " << msg; \
  exit(1);


#endif // of SG_MISC_TEST_MACROS_HXX
