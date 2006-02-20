// Copyright (C) 2006  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//

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
