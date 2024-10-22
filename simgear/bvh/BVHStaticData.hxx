// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

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
    { _vertices.push_back(vertex); return static_cast<unsigned>(_vertices.size() - 1); }
    const SGVec3f& getVertex(unsigned i) const
    { return _vertices[i]; }
    
    
    unsigned addMaterial(const BVHMaterial* material)
    { _materials.push_back(material); return static_cast<unsigned>(_materials.size() - 1); }
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
