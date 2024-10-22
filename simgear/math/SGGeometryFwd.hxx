// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2006 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef SGGeometryFwd_HXX
#define SGGeometryFwd_HXX

template<typename T>
class SGBox;
typedef SGBox<float> SGBoxf;
typedef SGBox<double> SGBoxd;

template<typename T>
class SGSphere;
typedef SGSphere<float> SGSpheref;
typedef SGSphere<double> SGSphered;

template<typename T>
class SGRay;
typedef SGRay<float> SGRayf;
typedef SGRay<double> SGRayd;

template<typename T>
class SGLineSegment;
typedef SGLineSegment<float> SGLineSegmentf;
typedef SGLineSegment<double> SGLineSegmentd;

template<typename T>
class SGPlane;
typedef SGPlane<float> SGPlanef;
typedef SGPlane<double> SGPlaned;

template<typename T>
class SGTriangle;
typedef SGTriangle<float> SGTrianglef;
typedef SGTriangle<double> SGTriangled;

#endif
