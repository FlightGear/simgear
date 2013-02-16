// Copyright (C) 2008 - 2009  Mathias Froehlich - Mathias.Froehlich@web.de
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
//

#ifndef BVHStaticData_hxx
#define BVHStaticData_hxx

#include <vector>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGGeometry.hxx>
#include "BVHMaterial.hxx"

namespace simgear {

class BVHStaticData : public SGReferenced {
public:
    virtual ~BVHStaticData() {}
    
    unsigned addVertex(const SGVec3f& vertex)
    { _vertices.push_back(vertex); return _vertices.size() - 1; }
    const SGVec3f& getVertex(unsigned i) const
    { return _vertices[i]; }
    
    
    unsigned addMaterial(const BVHMaterial* material)
    { _materials.push_back(material); return _materials.size() - 1; }
    const BVHMaterial* getMaterial(unsigned i) const
    { if (_materials.size() <= i) return 0; return _materials[i]; }

    void trim()
    {
        std::vector<SGVec3f>(_vertices).swap(_vertices);
        std::vector<SGSharedPtr<const BVHMaterial> >(_materials).swap(_materials);
    }
    
private:
    std::vector<SGVec3f> _vertices;
    std::vector<SGSharedPtr<const BVHMaterial> > _materials;
};

}

#endif
