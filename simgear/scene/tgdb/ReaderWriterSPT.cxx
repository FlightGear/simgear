// ReaderWriterSPT.cxx -- Provide a paged database for flightgear scenery.
//
// Copyright (C) 2010 - 2013  Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "ReaderWriterSPT.hxx"

#include <cassert>

#include <osg/Version>
#include <osg/CullFace>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osg/Texture2D>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <simgear/scene/util/OsgMath.hxx>

#include "BucketBox.hxx"

namespace simgear {

// Cull away tiles that we watch from downside
struct ReaderWriterSPT::CullCallback : public osg::NodeCallback {
    virtual ~CullCallback()
    { }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        const osg::BoundingSphere& nodeBound = node->getBound();
        // If the bounding sphere of the node is empty, there is nothing to do
        if (!nodeBound.valid())
            return;

        // Culling away tiles that we look at from the downside.
        // This is done by computing the maximum distance we can
        // see something from the current eyepoint. If the sphere
        // that is defined by this radius does not intersect the
        // node's sphere, then this tile is culled away.
        // Computing this radius happens by two rectangular triangles:
        // Let r be the view point. rmin is the minimum radius we find
        // a ground surface we need to look above. rmax is the
        // maximum object radius we expect any object.
        //
        //    d1   d2
        //  x----x----x
        //  r\  rmin /rmax
        //    \  |  /
        //     \ | /
        //      \|/
        //
        // The distance from the eyepoint to the point
        // where the line of sight is perpandicular to
        // the radius vector with minimal height is
        // d1 = sqrt(r^2 - rmin^2).
        // The distance from the point where the line of sight
        // is perpandicular to the radius vector with minimal height
        // to the highest possible object on earth with radius rmax is 
        // d2 = sqrt(rmax^2 - rmin^2).
        // So the maximum distance we can see something on the earth
        // from a viewpoint r is
        // d = d1 + d2

        // This is the equatorial earth radius minus 450m,
        // little lower than Dead Sea.
        float rmin = 6378137 - 450;
        float rmin2 = rmin*rmin;
        // This is the equatorial earth radius plus 9000m,
        // little higher than Mount Everest.
        float rmax = 6378137 + 9000;
        float rmax2 = rmax*rmax;

        // Check if we are looking from below any ground
        osg::Vec3 viewPoint = nv->getViewPoint();
        // blow the viewpoint up to a spherical earth with equatorial radius:
        osg::Vec3 sphericViewPoint = viewPoint;
        sphericViewPoint[2] *= 1.0033641;
        float r2 = sphericViewPoint.length2();
        if (r2 <= rmin2)
            return;

        // Due to this line of sight computation, the visible tiles
        // are limited to be within a sphere with radius d1 + d2.
        float d1 = sqrtf(r2 - rmin2);
        float d2 = sqrtf(rmax2 - rmin2);
        // Note that we again base the sphere around elliptic view point,
        // but use the radius from the spherical computation.
        if (!nodeBound.intersects(osg::BoundingSphere(viewPoint, d1 + d2)))
            return;

        traverse(node, nv);
    }
};

struct ReaderWriterSPT::LocalOptions {
    LocalOptions(const osgDB::Options* options) : 
        _options(options)
    {
        std::string pageLevelsString;
        if (_options)
            pageLevelsString = _options->getPluginStringData("SimGear::SPT_PAGE_LEVELS");

        // Get the default if nothing given from outside
        if (pageLevelsString.empty()) {
            // We want an other level of indirection for paging
            // Here we get about 12x12 deg tiles
            _pageLevels.push_back(3);
            // We want an other level of indirection for paging
            // Here we get about 2x2 deg tiles
            _pageLevels.push_back(5);
            // We want an other level of indirection for paging
            // Here we get about 0.5x0.5 deg tiles
            _pageLevels.push_back(7);
        } else {
            // If configured from outside
            std::stringstream ss(pageLevelsString);
            while (ss.good()) {
                unsigned level = ~0u;
                ss >> level;
                _pageLevels.push_back(level);
            }
        }
    }

    bool isPageLevel(unsigned level) const
    {
        return std::find(_pageLevels.begin(), _pageLevels.end(), level) != _pageLevels.end();
    }

    std::string getLodPathForBucketBox(const BucketBox& bucketBox) const
    {
        std::stringstream ss;
        ss << "LOD/";
        for (std::vector<unsigned>::const_iterator i = _pageLevels.begin(); i != _pageLevels.end(); ++i) {
            if (bucketBox.getStartLevel() <= *i)
                break;
            ss << bucketBox.getParentBox(*i) << "/";
        }
        ss << bucketBox;
        return ss.str();
    }

    float getRangeMultiplier() const
    {
        float rangeMultiplier = 2;
        if (!_options)
            return rangeMultiplier;
        std::stringstream ss(_options->getPluginStringData("SimGear::SPT_RANGE_MULTIPLIER"));
        ss >> rangeMultiplier;
        return rangeMultiplier;
    }

    const osgDB::Options* _options;
    std::vector<unsigned> _pageLevels;
};

ReaderWriterSPT::ReaderWriterSPT()
{
    supportsExtension("spt", "SimGear paged terrain meta database.");
}

ReaderWriterSPT::~ReaderWriterSPT()
{
}

const char*
ReaderWriterSPT::className() const
{
    return "simgear::ReaderWriterSPT";
}

osgDB::ReaderWriter::ReadResult
ReaderWriterSPT::readObject(const std::string& fileName, const osgDB::Options* options) const
{
    // We get called with different extensions. To make sure search continues,
    // we need to return FILE_NOT_HANDLED in this case.
    if (osgDB::getLowerCaseFileExtension(fileName) != "spt")
        return ReadResult(ReadResult::FILE_NOT_HANDLED);
    if (fileName != "state.spt")
        return ReadResult(ReadResult::FILE_NOT_FOUND);

    osg::StateSet* stateSet = new osg::StateSet;
    stateSet->setAttributeAndModes(new osg::CullFace);

    std::string imageFileName = options->getPluginStringData("SimGear::FG_WORLD_TEXTURE");
    if (imageFileName.empty()) {
        imageFileName = options->getPluginStringData("SimGear::FG_ROOT");
        imageFileName = osgDB::concatPaths(imageFileName, "Textures");
        imageFileName = osgDB::concatPaths(imageFileName, "Globe");
        imageFileName = osgDB::concatPaths(imageFileName, "world.topo.bathy.200407.3x4096x2048.png");
    }
    if (osg::Image* image = osgDB::readRefImageFile(imageFileName, options)) {
        osg::Texture2D* texture = new osg::Texture2D;
        texture->setImage(image);
        texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
        texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP);
        stateSet->setTextureAttributeAndModes(0, texture);        
    }

    return stateSet;
}

osgDB::ReaderWriter::ReadResult
ReaderWriterSPT::readNode(const std::string& fileName, const osgDB::Options* options) const
{
    LocalOptions localOptions(options);

    // The file name without path and without the spt extension
    std::string strippedFileName = osgDB::getStrippedName(fileName);
    if (strippedFileName == "earth")
        return ReadResult(createTree(BucketBox(-180, -90, 360, 180), localOptions, true));

    std::stringstream ss(strippedFileName);
    BucketBox bucketBox;
    ss >> bucketBox;
    if (ss.fail())
        return ReadResult::FILE_NOT_FOUND;
    
    BucketBox bucketBoxList[2];
    unsigned bucketBoxListSize = bucketBox.periodicSplit(bucketBoxList);
    if (bucketBoxListSize == 0)
        return ReadResult::FILE_NOT_FOUND;

    if (bucketBoxListSize == 1)
        return ReadResult(createTree(bucketBoxList[0], localOptions, true));

    assert(bucketBoxListSize == 2);
    osg::ref_ptr<osg::Group> group = new osg::Group;
    group->addChild(createTree(bucketBoxList[0], localOptions, true));
    group->addChild(createTree(bucketBoxList[1], localOptions, true));
    return ReadResult(group);
}

osg::ref_ptr<osg::Node>
ReaderWriterSPT::createTree(const BucketBox& bucketBox, const LocalOptions& options, bool topLevel) const
{
    if (bucketBox.getIsBucketSize()) {
        std::string fileName;
        fileName = bucketBox.getBucket().gen_index_str() + std::string(".stg");
        return osgDB::readRefNodeFile(fileName, options._options);
    } else if (!topLevel && options.isPageLevel(bucketBox.getStartLevel())) {
        return createPagedLOD(bucketBox, options);
    } else {
        BucketBox bucketBoxList[100];
        unsigned numTiles = bucketBox.getSubDivision(bucketBoxList, 100);
        if (numTiles == 0)
            return 0;

        if (numTiles == 1) 
            return createTree(bucketBoxList[0], options, false);

        osg::ref_ptr<osg::Group> group = new osg::Group;
        for (unsigned i = 0; i < numTiles; ++i) {
            osg::ref_ptr<osg::Node> node = createTree(bucketBoxList[i], options, false);
            if (!node.valid())
                continue;
            group->addChild(node.get());
        }
        if (!group->getNumChildren())
            return 0;
        
        return group;
    }
}

osg::ref_ptr<osg::Node>
ReaderWriterSPT::createPagedLOD(const BucketBox& bucketBox, const LocalOptions& options) const
{
    osg::ref_ptr<osg::PagedLOD> pagedLOD = new osg::PagedLOD;

    pagedLOD->setCenterMode(osg::PagedLOD::USER_DEFINED_CENTER);
    SGSpheref sphere = bucketBox.getBoundingSphere();
    pagedLOD->setCenter(toOsg(sphere.getCenter()));
    pagedLOD->setRadius(sphere.getRadius());

    pagedLOD->setCullCallback(new CullCallback);

    osg::ref_ptr<osgDB::Options> localOptions;
    localOptions = static_cast<osgDB::Options*>(options._options->clone(osg::CopyOp()));
    // FIXME:
    // The particle systems have nodes with culling disabled.
    // PagedLOD nodes with childnodes like this will never expire.
    // So, for now switch them off.
    localOptions->setPluginStringData("SimGear::PARTICLESYSTEM", "OFF");
    pagedLOD->setDatabaseOptions(localOptions.get());

    // The break point for the low level of detail to the high level of detail
    float rangeMultiplier = options.getRangeMultiplier();
    float range = rangeMultiplier*sphere.getRadius();

    // Look for a low level of detail tile
    std::string lodPath = options.getLodPathForBucketBox(bucketBox);
    const char* extensions[] = { ".btg.gz", ".flt" };
    for (unsigned i = 0; i < sizeof(extensions)/sizeof(extensions[0]); ++i) {
        std::string fileName = osgDB::findDataFile(lodPath + extensions[i], options._options);
        if (fileName.empty())
            continue;

        osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(fileName, options._options);
        if (!node.valid())
            continue;
        pagedLOD->addChild(node.get(), range, std::numeric_limits<float>::max());
        break;
    }
    // Add the static sea level textured shell if there is nothing found
    if (pagedLOD->getNumChildren() == 0) {
        osg::ref_ptr<osg::Node> node = createSeaLevelTile(bucketBox, options._options);
        if (node.valid())
            pagedLOD->addChild(node.get(), range, std::numeric_limits<float>::max());
    }

    // Add the paged file name that creates the subtrees on demand
    std::stringstream ss;
    ss << bucketBox << ".spt";
    pagedLOD->setFileName(pagedLOD->getNumChildren(), ss.str());
    pagedLOD->setRange(pagedLOD->getNumChildren(), 0.0, range);
    
    return pagedLOD;
}

osg::ref_ptr<osg::Node>
ReaderWriterSPT::createSeaLevelTile(const BucketBox& bucketBox, const LocalOptions& options) const
{
    if (options._options->getPluginStringData("SimGear::FG_EARTH") != "ON")
        return 0;

    SGSpheref sphere = bucketBox.getBoundingSphere();
    osg::Matrixd transform;
    transform.makeTranslate(toOsg(-sphere.getCenter()));

    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec3Array* normals = new osg::Vec3Array;
    osg::Vec2Array* texCoords = new osg::Vec2Array;

    unsigned widthLevel = bucketBox.getWidthLevel();
    unsigned heightLevel = bucketBox.getHeightLevel();

    unsigned incx = bucketBox.getWidthIncrement(widthLevel + 2);
    incx = std::min(incx, bucketBox.getSize(0));
    for (unsigned i = 0; incx != 0;) {
        unsigned incy = bucketBox.getHeightIncrement(heightLevel + 2);
        incy = std::min(incy, bucketBox.getSize(1));
        for (unsigned j = 0; incy != 0;) {
            SGVec3f v[6], n[6];
            SGVec2f t[6];
            unsigned num = bucketBox.getTileTriangles(i, j, incx, incy, v, n, t);
            for (unsigned k = 0; k < num; ++k) {
                vertices->push_back(transform.preMult(toOsg(v[k])));
                normals->push_back(toOsg(n[k]));
                texCoords->push_back(toOsg(t[k]));
            }
            j += incy;
            incy = std::min(incy, bucketBox.getSize(1) - j);
        }
        i += incx;
        incx = std::min(incx, bucketBox.getSize(0) - i);
    }

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1, 1, 1, 1));

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setDataVariance(osg::Object::STATIC);
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setTexCoordArray(0, texCoords);

    osg::DrawArrays* drawArrays = new osg::DrawArrays(osg::DrawArrays::TRIANGLES, 0, vertices->size());
    drawArrays->setDataVariance(osg::Object::STATIC);
    geometry->addPrimitiveSet(drawArrays);

    osg::Geode* geode = new osg::Geode;
    geode->setDataVariance(osg::Object::STATIC);
    geode->addDrawable(geometry);
    osg::ref_ptr<osg::StateSet> stateSet = getLowLODStateSet(options);
    geode->setStateSet(stateSet.get());

    transform.makeTranslate(toOsg(sphere.getCenter()));
    osg::MatrixTransform* matrixTransform = new osg::MatrixTransform(transform);
    matrixTransform->setDataVariance(osg::Object::STATIC);
    matrixTransform->addChild(geode);

    return matrixTransform;
}

osg::ref_ptr<osg::StateSet>
ReaderWriterSPT::getLowLODStateSet(const LocalOptions& options) const
{
    osg::ref_ptr<osgDB::Options> localOptions;
    localOptions = static_cast<osgDB::Options*>(options._options->clone(osg::CopyOp()));
    localOptions->setObjectCacheHint(osgDB::Options::CACHE_ALL);

    osg::ref_ptr<osg::Object> object = osgDB::readRefObjectFile("state.spt", localOptions.get());
    if (!dynamic_cast<osg::StateSet*>(object.get()))
        return 0;

    return static_cast<osg::StateSet*>(object.get());
}

} // namespace simgear

