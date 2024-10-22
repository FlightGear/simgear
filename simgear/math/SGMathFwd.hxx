// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef SGMathFwd_H
#define SGMathFwd_H

// All forward declarations in case they only need to be declared

class SGGeoc;
class SGGeod;

template<typename T>
class SGLocation;
template<typename T>
class SGLimits;
template<typename T>
class SGMatrix;
template<typename T>
class SGMisc;
template<typename T>
class SGQuat;
template<typename T>
class SGVec2;
template<typename T>
class SGVec3;
template<typename T>
class SGVec4;

typedef SGLocation<float> SGLocationf;
typedef SGLocation<double> SGLocationd;
typedef SGLimits<float> SGLimitsf;
typedef SGLimits<double> SGLimitsd;
typedef SGMatrix<float> SGMatrixf;
typedef SGMatrix<double> SGMatrixd;
typedef SGMisc<float> SGMiscf;
typedef SGMisc<double> SGMiscd;
typedef SGQuat<float> SGQuatf;
typedef SGQuat<double> SGQuatd;
typedef SGVec2<float> SGVec2f;
typedef SGVec2<double> SGVec2d;
typedef SGVec2<int> SGVec2i;
typedef SGVec3<float> SGVec3f;
typedef SGVec3<double> SGVec3d;
typedef SGVec3<int> SGVec3i;
typedef SGVec4<float> SGVec4f;
typedef SGVec4<double> SGVec4d;
typedef SGVec4<int> SGVec4i;

#endif
