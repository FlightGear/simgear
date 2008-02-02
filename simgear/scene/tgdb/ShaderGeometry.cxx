#include "ShaderGeometry.hxx"

using namespace osg;

namespace simgear
{
void ShaderGeometry::drawImplementation(RenderInfo& renderInfo) const
{
    for(PositionSizeList::const_iterator itr = _trees.begin();
        itr != _trees.end();
        ++itr) {
        glColor4fv(itr->ptr());
        _geometry->draw(renderInfo);
    }
}

BoundingBox ShaderGeometry::computeBound() const
{
    BoundingBox geom_box = _geometry->getBound();
    BoundingBox bb;
    for(PositionSizeList::const_iterator itr = _trees.begin();
        itr != _trees.end();
        ++itr) {
        bb.expandBy(geom_box.corner(0)*(*itr)[3] +
                    Vec3((*itr)[0], (*itr)[1], (*itr)[2]));
        bb.expandBy(geom_box.corner(7)*(*itr)[3] +
                    Vec3((*itr)[0], (*itr)[1], (*itr)[2]));
    }
    return bb;
}

}
