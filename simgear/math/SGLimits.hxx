#ifndef SGLimits_H
#define SGLimits_H

#include <limits>

/// Helper class for epsilon and so on
/// This is the possible place to hook in for machines not
/// providing numeric_limits ...
template<typename T>
class SGLimits : public std::numeric_limits<T> {};

typedef SGLimits<float> SGLimitsf;
typedef SGLimits<double> SGLimitsd;

#endif
