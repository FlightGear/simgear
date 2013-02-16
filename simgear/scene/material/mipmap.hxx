// Copyright (C) 2010  Frederic Bouvier
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_MIPMAP_HXX
#define SIMGEAR_MIPMAP_HXX 1

#include <boost/tuple/tuple.hpp>

class SGPropertyNode;

namespace osg {
    class Image;
}

namespace simgear
{
class Effect;
class SGReaderWriterOptions;

namespace effect {
enum MipMapFunction {
    AUTOMATIC,
    AVERAGE,
    SUM,
    PRODUCT,
    MIN,
    MAX
};

typedef boost::tuple<MipMapFunction, MipMapFunction, MipMapFunction, MipMapFunction> MipMapTuple;

MipMapTuple makeMipMapTuple(Effect* effect, const SGPropertyNode* props,
                      const SGReaderWriterOptions* options);
osg::Image* computeMipmap( osg::Image* image, MipMapTuple attrs );
} }

#endif
