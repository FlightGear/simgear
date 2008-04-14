#ifndef SIMGEAR_MATH_MATH_HXX
#define SIMGEAR_MATH_MATH_HXX 1
namespace simgear
{
namespace math
{
/** Linear interpolation between two values.
 */
template<typename T>
inline T lerp(const T& x, const T& y, double alpha)
{
    return x * (1.0 - alpha) + y * alpha;
}

template<typename T>
inline T lerp(const T& x, const T& y, float alpha)
{
    return x * (1.0f - alpha) + y * alpha;
}

}
}
#endif
