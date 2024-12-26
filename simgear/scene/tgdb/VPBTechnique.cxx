// VPBTechnique.cxx -- VirtualPlanetBuilder Effects technique
//
// Copyright (C) 2020 Stuart Buchanan
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

#include <cmath>
#include <tuple>

#include <osgTerrain/TerrainTile>
#include <osgTerrain/Terrain>

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/Tessellator>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture1D>
#include <osg/PatchParameter>
#include <osg/Program>
#include <osg/Math>
#include <osg/Timer>

#include <simgear/bvh/BVHSubTreeCollector.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/tgdb/VPBElevationSlice.hxx>
#include <simgear/scene/tgdb/VPBTileBounds.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include "VPBTechnique.hxx"
#include "VPBBufferData.hxx"
#include "VPBMaterialHandler.hxx"
#include "VPBRasterRenderer.hxx"
#include "VPBLineFeatureRenderer.hxx"

using namespace osgTerrain;
using namespace simgear;
using std::max;
using std::min;

VPBTechnique::VPBTechnique()
{
    setFilterBias(0);
    setFilterWidth(0.1);
    setFilterMatrixAs(GAUSSIAN);
    _randomObjectsConstraintGroup = new osg::Group();
    setOptions(SGReaderWriterOptions::copyOrCreate(NULL));
}

VPBTechnique::VPBTechnique(const SGReaderWriterOptions* options, const std::string fileName) : _fileName(fileName)
{
    setFilterBias(0);
    setFilterWidth(0.1);
    setFilterMatrixAs(GAUSSIAN);
    setOptions(options);
    _randomObjectsConstraintGroup = new osg::Group();
}

VPBTechnique::VPBTechnique(const VPBTechnique& gt,const osg::CopyOp& copyop):
    TerrainTechnique(gt,copyop),
    _fileName(gt._fileName)
{
    setFilterBias(gt._filterBias);
    setFilterWidth(gt._filterWidth);
    setFilterMatrix(gt._filterMatrix);
    setOptions(gt._options);
    _randomObjectsConstraintGroup = new osg::Group();
}

VPBTechnique::~VPBTechnique()
{
}

void VPBTechnique::setFilterBias(float filterBias)
{
    _filterBias = filterBias;
    if (!_filterBiasUniform) _filterBiasUniform = new osg::Uniform("filterBias",_filterBias);
    else _filterBiasUniform->set(filterBias);
}

void VPBTechnique::setFilterWidth(float filterWidth)
{
    _filterWidth = filterWidth;
    if (!_filterWidthUniform) _filterWidthUniform = new osg::Uniform("filterWidth",_filterWidth);
    else _filterWidthUniform->set(filterWidth);
}

void VPBTechnique::setFilterMatrix(const osg::Matrix3& matrix)
{
    _filterMatrix = matrix;
    if (!_filterMatrixUniform) _filterMatrixUniform = new osg::Uniform("filterMatrix",_filterMatrix);
    else _filterMatrixUniform->set(_filterMatrix);
}

void VPBTechnique::setOptions(const SGReaderWriterOptions* options)
{
    _options = simgear::SGReaderWriterOptions::copyOrCreate(options);
    _options->setLoadOriginHint(simgear::SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS);
    _options->setInstantiateMaterialEffects(true);

    if (! _statsPropertyNode) {
        const std::lock_guard<std::mutex> lock(VPBTechnique::_stats_mutex); // Lock the _stats_mutex for this scope
        _statsPropertyNode = _options->getPropertyNode()->getNode("/sim/rendering/statistics/ws30/loading", true);
        _useTessellationPropNode = _options->getPropertyNode()->getNode("/sim/rendering/shaders/tessellation", true);
    }    
}

void VPBTechnique::setFilterMatrixAs(FilterType filterType)
{
    switch(filterType)
    {
        case(SMOOTH):
            setFilterMatrix(osg::Matrix3(0.0, 0.5/2.5, 0.0,
                                         0.5/2.5, 0.5/2.5, 0.5/2.5,
                                         0.0, 0.5/2.5, 0.0));
            break;
        case(GAUSSIAN):
            setFilterMatrix(osg::Matrix3(0.0, 1.0/8.0, 0.0,
                                         1.0/8.0, 4.0/8.0, 1.0/8.0,
                                         0.0, 1.0/8.0, 0.0));
            break;
        case(SHARPEN):
            setFilterMatrix(osg::Matrix3(0.0, -1.0, 0.0,
                                         -1.0, 5.0, -1.0,
                                         0.0, -1.0, 0.0));
            break;

    };
}

void VPBTechnique::init(int dirtyMask, bool assumeMultiThreaded)
{
    if (!_terrainTile) return;
    
    // Don't regenerate if the tile is not dirty AND we haven't switched between tessellation
    // and non-tessellation mode.  A cleaned way to do this would be to have a listener on the
    // property that dirties all tiles.
    bool b = _useTessellationPropNode->getBoolValue();
    if ((dirtyMask == 0) && (_useTessellation == b)) return;

    // Indicate whether to use tessellation for this tile
    _useTessellation = b;

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_writeBufferMutex);

    auto start = std::chrono::system_clock::now();
    osg::ref_ptr<TerrainTile> tile = _terrainTile;

    osgTerrain::TileID tileID = tile->getTileID();
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Init of tile " << tileID.x << "," << tileID.y << " level " << tileID.level << " " << dirtyMask << " _useTessellation " << _useTessellation << " _currentBufferData? " << (_currentBufferData != 0));

    osg::ref_ptr<BufferData> buffer = new BufferData;

    buffer->_masterLocator = computeMasterLocator();

    osg::Vec3d centerModel = computeCenterModel(*buffer);

    // Generate a set of material definitions for this location.
    SGMaterialLibPtr matlib  = _options->getMaterialLib();
    const SGGeod loc = computeCenterGeod(*buffer);
    osg::ref_ptr<SGMaterialCache> matcache;
    if (matlib) {
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Applying VPB material " << loc);
        matcache = _options->getMaterialLib()->generateMatCache(loc, _options, true);
        if (!matcache) SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to create materials cache for  " << loc);
    } else {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to create materials lib for  " << loc);
    }

    if ((dirtyMask & TerrainTile::IMAGERY_DIRTY)==0)
    {
        generateGeometry(*buffer, centerModel, matcache);

        osg::ref_ptr<BufferData> read_buffer = _currentBufferData;

        osg::StateSet* landStateset = read_buffer->_landGeode->getStateSet();
        if (landStateset)
        {
            buffer->_landGeode->setStateSet(landStateset);
            osg::StateSet* seaStateset = read_buffer->_seaGeode->getStateSet();
            buffer->_seaGeode->setStateSet(seaStateset);
        }
        else
        {
            applyColorLayers(*buffer, matcache);
            VPBLineFeatureRenderer lineFeatureRenderer = VPBLineFeatureRenderer(_terrainTile);
            lineFeatureRenderer.applyLineFeatures(*buffer, _options, matcache);
            applyMaterials(*buffer, matcache, loc);
        }
    }
    else
    {
        generateGeometry(*buffer, centerModel, matcache);
        
        applyColorLayers(*buffer, matcache);
        VPBLineFeatureRenderer lineFeatureRenderer = VPBLineFeatureRenderer(_terrainTile);
        lineFeatureRenderer.applyLineFeatures(*buffer, _options, matcache);
        applyMaterials(*buffer, matcache, loc);
    }

    if (buffer->_transform.valid()) buffer->_transform->setThreadSafeRefUnref(true);

    if (!_currentBufferData || !assumeMultiThreaded)
    {
        // no currentBufferData so we must be the first init to be applied
        _currentBufferData = buffer;
    }
    else
    {
        // there is already an active _currentBufferData so we'll request that this gets swapped on next frame.
        _newBufferData = buffer;
        if (_terrainTile->getTerrain()) _terrainTile->getTerrain()->updateTerrainTileOnNextFrame(_terrainTile);
    }

    _terrainTile->setDirtyMask(0);

    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
    VPBTechnique::updateStats(tileID.level, elapsed_seconds.count());
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Init complete of tile " << tileID.x << "," << tileID.y << " level " << tileID.level << " " << elapsed_seconds.count() << " seconds.");
}

Locator* VPBTechnique::computeMasterLocator()
{
    osgTerrain::Layer* elevationLayer = _terrainTile->getElevationLayer();
    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);

    Locator* elevationLocator = elevationLayer ? elevationLayer->getLocator() : 0;
    Locator* colorLocator = colorLayer ? colorLayer->getLocator() : 0;

    Locator* masterLocator = elevationLocator ? elevationLocator : colorLocator;
    if (!masterLocator)
    {
        OSG_NOTICE<<"Problem, no locator found in any of the terrain layers"<<std::endl;
        return 0;
    }

    return masterLocator;
}

osg::Vec3d VPBTechnique::computeCenter(BufferData& buffer)
{
    if (!buffer._masterLocator) return osg::Vec3d(0.0,0.0,0.0);

    osgTerrain::Layer* elevationLayer = _terrainTile->getElevationLayer();
    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);

    Locator* elevationLocator = elevationLayer ? elevationLayer->getLocator() : 0;
    Locator* colorLocator = colorLayer ? colorLayer->getLocator() : 0;

    if (!elevationLocator) elevationLocator = buffer._masterLocator;
    if (!colorLocator) colorLocator = buffer._masterLocator;

    osg::Vec3d bottomLeftNDC(DBL_MAX, DBL_MAX, 0.0);
    osg::Vec3d topRightNDC(-DBL_MAX, -DBL_MAX, 0.0);

    if (elevationLayer)
    {
        if (elevationLocator!= buffer._masterLocator)
        {
            buffer._masterLocator->computeLocalBounds(*elevationLocator, bottomLeftNDC, topRightNDC);
        }
        else
        {
            bottomLeftNDC.x() = osg::minimum(bottomLeftNDC.x(), 0.0);
            bottomLeftNDC.y() = osg::minimum(bottomLeftNDC.y(), 0.0);
            topRightNDC.x() = osg::maximum(topRightNDC.x(), 1.0);
            topRightNDC.y() = osg::maximum(topRightNDC.y(), 1.0);
        }
    }

    if (colorLayer)
    {
        if (colorLocator!= buffer._masterLocator)
        {
            buffer._masterLocator->computeLocalBounds(*colorLocator, bottomLeftNDC, topRightNDC);
        }
        else
        {
            bottomLeftNDC.x() = osg::minimum(bottomLeftNDC.x(), 0.0);
            bottomLeftNDC.y() = osg::minimum(bottomLeftNDC.y(), 0.0);
            topRightNDC.x() = osg::maximum(topRightNDC.x(), 1.0);
            topRightNDC.y() = osg::maximum(topRightNDC.y(), 1.0);
        }
    }

    OSG_INFO<<"bottomLeftNDC = "<<bottomLeftNDC<<std::endl;
    OSG_INFO<<"topRightNDC = "<<topRightNDC<<std::endl;

    osg::Vec3d centerNDC = (bottomLeftNDC + topRightNDC)*0.5;
    return centerNDC;
}

osg::Vec3d VPBTechnique::computeCenterModel(BufferData& buffer)
{
    osg::Vec3d centerNDC = computeCenter(buffer);
    osg::Vec3d centerModel = centerNDC;
    buffer._masterLocator->convertLocalToModel(centerNDC, centerModel);

    SGGeod c = SGGeod::fromCart(toSG(centerModel));
    //osg::Matrix m = osg::Matrixd::inverse(makeZUpFrameRelative(c));
    buffer._transform = new osg::MatrixTransform;
    //buffer._transform->setMatrix(osg::Matrix::translate(centerModel) * m);
    //buffer._transform->setMatrix(osg::Matrix::translate(centerModel));
    buffer._transform->setMatrix(makeZUpFrame(c));

    return centerModel;
}

const SGGeod VPBTechnique::computeCenterGeod(BufferData& buffer)
{
    const osg::Vec3d world = buffer._transform->getMatrix().getTrans();
    return SGGeod::fromCart(toSG(world));
}


VPBTechnique::VertexNormalGenerator::VertexNormalGenerator(Locator* masterLocator, const osg::Vec3d& centerModel, int numRows, int numColumns, float scaleHeight, float vtx_gap, bool createSkirt, bool useTessellation):
    _masterLocator(masterLocator),
    _centerModel(centerModel),
    _numRows(numRows),
    _numColumns(numColumns),
    _scaleHeight(scaleHeight),
    _constraint_vtx_gap(vtx_gap),
    _useTessellation(useTessellation)
{
    _ZUpRotationMatrix = makeZUpFrameRelative(SGGeod::fromCart(toSG(_centerModel)));

    int numVerticesInBody = numColumns*numRows;
    int numVertices;

    if (_useTessellation) {
        // If we're using tessellation then we have the main body plus a boundary around the edge
        int numVerticesInBoundary = _numRows*2 + _numColumns*2 + 4;
        numVertices = numVerticesInBody + numVerticesInBoundary;
    } else {
        // If we're not using tessellation then we may instead have a skirt.
        int numVerticesInSkirt = createSkirt ? numColumns*2 + numRows*2 - 4 : 0;
        numVertices = numVerticesInBody + numVerticesInSkirt;
    }

    _indices.resize((_numRows+2)*(_numColumns+2), 0);

    _vertices = new osg::Vec3Array;
    _vertices->reserve(numVertices);

    if (! _useTessellation) {
        // If we're not using Tessellation then we will have both a sea-level mesh and also generate normals ourselves.
        _sea_vertices = new osg::Vec3Array;
        _sea_vertices->reserve(numVertices);

        _normals = new osg::Vec3Array;
        _normals->reserve(numVertices);        
        _sea_normals = new osg::Vec3Array;
        _sea_normals->reserve(numVertices);        

        _boundaryVertices = new osg::Vec3Array;
        _boundaryVertices->reserve(_numRows*2 + _numColumns*2 + 4);        
    }

    // Initialize the elevation constraints to a suitably high number such
    // that any vertex or valid constraint will always fall below it
    _elevationConstraints.assign(numVertices, 9999.0f);
}

void VPBTechnique::VertexNormalGenerator::populateCenter(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas, osgTerrain::TerrainTile* tile, osg::Vec2Array* texcoords0, osg::Vec2Array* texcoords1)
{
    // OSG_NOTICE<<std::endl<<"VertexNormalGenerator::populateCenter("<<elevationLayer<<")"<<std::endl;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

    osg::Image* landclassImage = colorLayer->getImage();

    // For textcoords1 we want a set of uv coordinates such that for each 1x1 degree block they range from (1,1) at each corner to (0,0)
    // in the center.
    //
    //E.g.
    //
    //     (1,1)          (0,1)             (1,1)
    //       +--------------+--------------+
    //       |                             |
    //       |                             |
    //       |            (0,0)            |
    // (1,0) |              +              | (1,0)
    //       |                             |
    //       |                             |
    //       |                             |
    //       +--------------+--------------+
    //  (1,1)          (0,1)             (1,1)
    //
    // Due to the way that the 1x1 blocks tile, they end up being continuous.
    //
    // They are intended to be used as input into noise systems that require
    // a continuous set of UV coordinates.
    osgTerrain::TileID tileID = tile->getTileID();
    double dim = (double) std::pow(2, (tileID.level - 1));

    // We do two passes to calculate the model coordinates.
    // In the first pass we calculate the x/y location and if there are any elevation constraints at that point.
    // In the second pass we determine the elevation of the mesh as the lowest of
    //   - the elevation of the location base on the elevation layer
    //   - -10.0 (in the case of sea level)
    //   - any constraints for this point and the surrounding 8 points

    for(int j=0; j<_numRows; ++j) {
        for(int i=0; i<_numColumns; ++i) {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), (double) 10000.0);
            double elev = VPBTechnique::getConstrainedElevation(ndc, _masterLocator, _constraint_vtx_gap);
            if (elev < 10000.0) {
                _elevationConstraints[j * _numColumns + i] =  elev;
            }
        }
    }

    for(int j=0; j<_numRows; ++j) {
        for(int i=0; i<_numColumns; ++i) {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);

            bool validValue = true;

            if (elevationLayer) {
                float value = 0.0f;
                if (sampled) validValue = elevationLayer->getInterpolatedValidValue(ndc.x(), ndc.y(), value);
                else validValue = elevationLayer->getValidValue(i,j,value);

                if (validValue) {
                    ndc.z() = value*_scaleHeight;
                } else {
                    SG_LOG(SG_TERRAIN, SG_ALERT, "Invalid elevation value found " << elevationLayer->getName());
                }
            }

            // Check against the sea.
            if (landclassImage) {
                osg::Vec4d c = landclassImage->getColor(osg::Vec2d(ndc.x(), ndc.y()));
                unsigned int lc = (unsigned int) std::abs(std::round(c.x() * 255.0));
                if (atlas->isSea(lc)) {
                    ndc.z() = _useTessellation ? 0.0 : -10.0;
                }
            }

            // Check against the constraints of this and surrounding points.  This avoids problems where
            // there is a big elevation difference between two adjacent points, only one of which is covered
            // by the AirportKeep
            for (int jj = -1; jj < 2; ++jj) {
                for (int ii = -1; ii < 2; ++ii) {
                    int row = j + jj;
                    int col = i + ii;
                    if ((row > -1) && (row < _numRows) &&
                        (col > -1) && (col < _numColumns) &&
                        (ndc.z() > _elevationConstraints[row * _numColumns + col])) {
                            ndc.z() = _elevationConstraints[row * _numColumns + col];
                    }
                }
            }
            
            if (_useTessellation) {
                // compute the model coordinates            
                setVertex(i, j, convertLocalToModel(ndc));
                texcoords0->push_back(osg::Vec2(ndc.x(), ndc.y()));
                texcoords1->push_back(osg::Vec2(2.0 * std::fabs((ndc.x() + (double) tileID.x) / dim - 0.5), 2.0 * std::fabs((ndc.y() + (double) tileID.y) / dim - 0.5)));

            } else {
                // compute the model coordinates and the local normal
                osg::Vec3d ndc_up = ndc; ndc_up.z() += 1.0;
                osg::Vec3d model = convertLocalToModel(ndc);
                osg::Vec3d model_up = convertLocalToModel(ndc_up);
                model_up = model_up - model;
                model_up.normalize();

                setVertex(i, j, model, model_up);
                texcoords0->push_back(osg::Vec2(ndc.x(), ndc.y()));
                texcoords1->push_back(osg::Vec2(2.0 * std::abs((ndc.x() + tileID.x) / dim - 0.5), 2.0 * std::abs((ndc.y() + tileID.y) / dim - 0.5)));
            }
        }
    }
}

// Generate a set of vertices at sea level - only valid for non-tesselated terrain
void VPBTechnique::VertexNormalGenerator::populateSeaLevel()
{
    assert(! _useTessellation);
    // OSG_NOTICE<<std::endl<<"VertexNormalGenerator::populateCenter("<<elevationLayer<<")"<<std::endl;

    for(int j=0; j<_numRows; ++j) {
        for(int i=0; i<_numColumns; ++i) {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);

            // compute the model coordinates and the local normal
            osg::Vec3d ndc_up = ndc; ndc_up.z() += 1.0;
            osg::Vec3d model = convertLocalToModel(ndc);
            osg::Vec3d model_up = convertLocalToModel(ndc_up);
            model_up = model_up - model;
            model_up.normalize();
            _sea_vertices->push_back(model);
            _sea_normals->push_back(model_up);
        }
    }
}

void VPBTechnique::VertexNormalGenerator::populateLeftBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateLeftBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

    osg::Image* landclassImage = colorLayer->getImage();
    
    for(int j=0; j<_numRows; ++j)
    {
        for(int i=-1; i<=0; ++i)
        {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);
            osg::Vec3d left_ndc( 1.0+ndc.x(), ndc.y(), 0.0);

            bool validValue = true;
            if (elevationLayer)
            {
                float value = 0.0f;
                if (sampled) validValue = elevationLayer->getInterpolatedValidValue(left_ndc.x(), left_ndc.y(), value);
                else validValue = elevationLayer->getValidValue((_numColumns-1)+i,j,value);
                ndc.z() = value*_scaleHeight;
            }

            if (landclassImage)
            {
                osg::Vec4d c = landclassImage->getColor(osg::Vec2d(ndc.x(), ndc.y()));
                unsigned int lc = (unsigned int) std::abs(std::round(c.x() * 255.0));
                if (atlas->isSea(lc)) {
                    ndc.set(ndc.x(), ndc.y(), 0.0f);
                }
            }

            if (validValue)
            {
                osg::Vec3d model = convertLocalToModel(ndc);

                if (_useTessellation) {
                    setVertex(i, j, model);
                } else {
                    // compute the local normal
                    osg::Vec3d ndc_up = ndc; ndc_up.z() += 1.0;
                    osg::Vec3d model_up = convertLocalToModel(ndc_up);
                    model_up = model_up - model;
                    model_up.normalize();

                    setVertex(i, j, model, model_up);
                }
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

void VPBTechnique::VertexNormalGenerator::populateRightBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateRightBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

    osg::Image* landclassImage = colorLayer->getImage();
    
    for(int j=0; j<_numRows; ++j)
    {
        for(int i=_numColumns-1; i<_numColumns+1; ++i)
        {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);
            osg::Vec3d right_ndc(ndc.x()-1.0, ndc.y(), 0.0);

            bool validValue = true;
            if (elevationLayer)
            {
                float value = 0.0f;
                if (sampled) validValue = elevationLayer->getInterpolatedValidValue(right_ndc.x(), right_ndc.y(), value);
                else validValue = elevationLayer->getValidValue(i-(_numColumns-1),j,value);
                ndc.z() = value*_scaleHeight;
            }

            if (landclassImage)
            {
                osg::Vec4d c = landclassImage->getColor(osg::Vec2d(ndc.x(), ndc.y()));
                unsigned int lc = (unsigned int) std::abs(std::round(c.x() * 255.0));
                if (atlas->isSea(lc)) {
                    ndc.set(ndc.x(), ndc.y(), 0.0f);
                }
            }

            if (validValue)
            {
                osg::Vec3d model = convertLocalToModel(ndc);

                if (_useTessellation) {
                    setVertex(i, j, model);
                } else {
                    // compute the local normal
                    osg::Vec3d ndc_up = ndc; ndc_up.z() += 1.0;
                    osg::Vec3d model_up = convertLocalToModel(ndc_up);
                    model_up = model_up - model;
                    model_up.normalize();

                    setVertex(i, j, model, model_up);
                }
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

void VPBTechnique::VertexNormalGenerator::populateAboveBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateAboveBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

    osg::Image* landclassImage = colorLayer->getImage();
    
    for(int j=_numRows-1; j<_numRows+1; ++j)
    {
        for(int i=0; i<_numColumns; ++i)
        {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);
            osg::Vec3d above_ndc( ndc.x(), ndc.y()-1.0, 0.0);

            bool validValue = true;
            if (elevationLayer)
            {
                float value = 0.0f;
                if (sampled) validValue = elevationLayer->getInterpolatedValidValue(above_ndc.x(), above_ndc.y(), value);
                else validValue = elevationLayer->getValidValue(i,j-(_numRows-1),value);
                ndc.z() = value*_scaleHeight;
            }

            if (landclassImage)
            {
                osg::Vec4d c = landclassImage->getColor(osg::Vec2d(ndc.x(), ndc.y()));
                unsigned int lc = (unsigned int) std::abs(std::round(c.x() * 255.0));
                if (atlas->isSea(lc)) {
                    ndc.set(ndc.x(), ndc.y(), 0.0f);
                }
            }

            if (validValue)
            {
                osg::Vec3d model = convertLocalToModel(ndc);

                if (_useTessellation) {
                    setVertex(i, j, model);
                } else {
                    // compute the local normal
                    osg::Vec3d ndc_up = ndc; ndc_up.z() += 1.0;
                    osg::Vec3d model_up = convertLocalToModel(ndc_up);
                    model_up = model_up - model;
                    model_up.normalize();

                    setVertex(i, j, model, model_up);
                }
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

void VPBTechnique::VertexNormalGenerator::populateBelowBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateBelowBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

    osg::Image* landclassImage = colorLayer->getImage();
    
    for(int j=-1; j<=0; ++j)
    {
        for(int i=0; i<_numColumns; ++i)
        {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);
            osg::Vec3d below_ndc( ndc.x(), 1.0+ndc.y(), 0.0);

            bool validValue = true;
            if (elevationLayer)
            {
                float value = 0.0f;
                if (sampled) validValue = elevationLayer->getInterpolatedValidValue(below_ndc.x(), below_ndc.y(), value);
                else validValue = elevationLayer->getValidValue(i,(_numRows-1)+j,value);
                ndc.z() = value*_scaleHeight;
            }

            if (landclassImage)
            {
                osg::Vec4d c = landclassImage->getColor(osg::Vec2d(ndc.x(), ndc.y()));
                unsigned int lc = (unsigned int) std::abs(std::round(c.x() * 255.0));
                if (atlas->isSea(lc)) {
                    ndc.set(ndc.x(), ndc.y(), 0.0f);
                }
            }

            if (validValue)
            {
                osg::Vec3d model = convertLocalToModel(ndc);

                if (_useTessellation) {
                    setVertex(i, j, model);
                } else {
                    // compute the local normal
                    osg::Vec3d ndc_up = ndc; ndc_up.z() += 1.0;
                    osg::Vec3d model_up = convertLocalToModel(ndc_up);
                    model_up = model_up - model;
                    model_up.normalize();

                    setVertex(i, j, model, model_up);
                }
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

// Only valid with tessellation
void VPBTechnique::VertexNormalGenerator::populateCorner(
    osgTerrain::Layer* elevationLayer,
    osgTerrain::Layer* colorLayer,
    osg::ref_ptr<Atlas> atlas,
    Corner corner)
{
    assert(_useTessellation);
    if (!elevationLayer)
        return;

    bool sampled =
        (elevationLayer->getNumRows() != static_cast<unsigned int>(_numRows)) ||
        (elevationLayer->getNumColumns() != static_cast<unsigned int>(_numColumns));

    osg::Image* landclassImage = colorLayer->getImage();

    int i, j;
    switch (corner) {
    case Corner::BOTTOM_LEFT:  i = -1;          j =       -1; break;
    case Corner::BOTTOM_RIGHT: i = _numColumns; j =       -1; break;
    case Corner::TOP_LEFT:     i = -1;          j = _numRows; break;
    case Corner::TOP_RIGHT:    i = _numColumns; j = _numRows; break;
    }

    osg::Vec3d ndc(double(i) / double(_numColumns-1), double(j) / double(_numRows-1), 0.0);

    bool validValue = true;
    float value = 0.0f;

    if (sampled) {
        osg::Vec2d ndcOffset;
        switch (corner) {
        case Corner::BOTTOM_LEFT:  ndcOffset.set( 1.0,  1.0); break;
        case Corner::BOTTOM_RIGHT: ndcOffset.set(-1.0,  1.0); break;
        case Corner::TOP_LEFT:     ndcOffset.set( 1.0, -1.0); break;
        case Corner::TOP_RIGHT:    ndcOffset.set(-1.0, -1.0); break;
        }
        validValue = elevationLayer->getInterpolatedValidValue(
            ndc.x() + ndcOffset.x(), ndc.y() + ndcOffset.y(), value);
    } else {
        int layer_i, layer_j;
        switch (corner) {
        case Corner::BOTTOM_LEFT:  layer_i = _numColumns-2; layer_j = _numRows-2; break;
        case Corner::BOTTOM_RIGHT: layer_i =             1; layer_j = _numRows-2; break;
        case Corner::TOP_LEFT:     layer_i = _numColumns-2; layer_j =          1; break;
        case Corner::TOP_RIGHT:    layer_i =             1; layer_j =          1; break;
        }
        validValue = elevationLayer->getValidValue(layer_i, layer_j, value);
    }

    ndc.z() = value * _scaleHeight;

    if (landclassImage) {
        osg::Vec4d c = landclassImage->getColor(osg::Vec2d(ndc.x(), ndc.y()));
        unsigned int lc = (unsigned int) std::abs(std::round(c.x() * 255.0));
        if (atlas->isSea(lc)) {
            ndc.set(ndc.x(), ndc.y(), 0.0f);
        }
    }

    if (validValue) {
        osg::Vec3d model = convertLocalToModel(ndc);
        setVertex(i, j, model);
        // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
    }
}

void VPBTechnique::VertexNormalGenerator::computeNormals()
{
    assert(! _useTessellation);
    // compute normals for the center section
    for(int j=0; j<_numRows; ++j)
    {
        for(int i=0; i<_numColumns; ++i)
        {
            int vi = vertex_index(i, j);
            if (vi>=0) computeNormal(i, j, (*_normals)[vi]);
            else OSG_NOTICE<<"Not computing normal, vi="<<vi<<std::endl;
        }
    }
}

void VPBTechnique::generateGeometry(BufferData& buffer, const osg::Vec3d& centerModel, osg::ref_ptr<SGMaterialCache> matcache)
{
    osg::ref_ptr<Atlas> atlas;

    Terrain* terrain = _terrainTile->getTerrain();
    osgTerrain::Layer* elevationLayer = _terrainTile->getElevationLayer();
    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);

    // Determine the correct Effect for this, based on a material lookup taking into account
    // the lat/lon of the center.
    SGPropertyNode_ptr landEffectProp = new SGPropertyNode();

    if (matcache) {
      atlas = matcache->getAtlas();
      SGMaterial* landmat = matcache->find("ws30land");

      if (landmat) {
        makeChild(landEffectProp.ptr(), "inherits-from")->setStringValue(landmat->get_effect_name());
      } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get ws30land Material for VPB - no matching material in library");
        makeChild(landEffectProp.ptr(), "inherits-from")->setStringValue("Effects/model-default");
      }

    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get ws30land/ws30sea effect for VPB - no material library available");
        makeChild(landEffectProp.ptr(), "inherits-from")->setStringValue("Effects/model-default");
    }

    buffer._landGeode = new EffectGeode();
    buffer._seaGeode = new EffectGeode();

    if (buffer._transform.valid()) buffer._transform->addChild(buffer._landGeode.get());

    buffer._landGeometry = new osg::Geometry;
    buffer._landGeode->addDrawable(buffer._landGeometry.get());
  
    osg::ref_ptr<Effect> landEffect = makeEffect(landEffectProp, true, _options);
    buffer._landGeode->setEffect(landEffect.get());
    buffer._landGeode->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );

    if (! _useTessellation) {
        // Generate a sea-level mesh if we're not using tessellation.
        SGPropertyNode_ptr seaEffectProp = new SGPropertyNode();

        if (matcache) {
            SGMaterial* seamat = matcache->find("ws30sea");
            if (seamat) {
                makeChild(seaEffectProp.ptr(), "inherits-from")->setStringValue(seamat->get_effect_name());
            } else {
                SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get ws30sea Material for VPB - no matching material in library");
                makeChild(seaEffectProp.ptr(), "inherits-from")->setStringValue("Effects/model-default");
            }
        } else {
            SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get ws30land/ws30sea effect for VPB - no material library available");
            makeChild(seaEffectProp.ptr(), "inherits-from")->setStringValue("Effects/model-default");
        }

        if (buffer._transform.valid()) buffer._transform->addChild(buffer._seaGeode.get());

        buffer._seaGeometry = new osg::Geometry;
        buffer._seaGeode->addDrawable(buffer._seaGeometry.get());
    
        osg::ref_ptr<Effect> seaEffect = makeEffect(seaEffectProp, true, _options);
        buffer._seaGeode->setEffect(seaEffect.get());
        buffer._seaGeode->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );
    }

    unsigned int numRows = 20;
    unsigned int numColumns = 20;

    if (elevationLayer)
    {
        numColumns = elevationLayer->getNumColumns();
        numRows = elevationLayer->getNumRows();
    }

    double scaleHeight = SGSceneFeatures::instance()->getVPBVerticalScale();
    double sampleRatio = SGSceneFeatures::instance()->getVPBSampleRatio();
    double constraint_gap = SGSceneFeatures::instance()->getVPBConstraintGap();

    unsigned int minimumNumColumns = 16u;
    unsigned int minimumNumRows = 16u;

    if ((sampleRatio!=1.0f) && (numColumns>minimumNumColumns) && (numRows>minimumNumRows))
    {
        unsigned int originalNumColumns = numColumns;
        unsigned int originalNumRows = numRows;

        numColumns = std::max((unsigned int) (float(originalNumColumns)*sqrtf(sampleRatio)), minimumNumColumns);
        numRows = std::max((unsigned int) (float(originalNumRows)*sqrtf(sampleRatio)),minimumNumRows);
    }

    bool treatBoundariesToValidDataAsDefaultValue = _terrainTile->getTreatBoundariesToValidDataAsDefaultValue();
    OSG_INFO<<"TreatBoundariesToValidDataAsDefaultValue="<<treatBoundariesToValidDataAsDefaultValue<<std::endl;

    float skirtHeight = 0.0f;
    HeightFieldLayer* hfl = dynamic_cast<HeightFieldLayer*>(elevationLayer);
    if (hfl && hfl->getHeightField())
    {
        skirtHeight = hfl->getHeightField()->getSkirtHeight();
    }

    bool createSkirt = skirtHeight != 0.0f;

    // construct the VertexNormalGenerator which will manage the generation and the vertices and normals
    VertexNormalGenerator VNG(buffer._masterLocator, centerModel, numRows, numColumns, scaleHeight, constraint_gap, createSkirt, _useTessellation);

    unsigned int numVertices = VNG.capacity();

    // allocate and assign vertices
    buffer._landGeometry->setVertexArray(VNG._vertices.get());

    // allocate and assign texture coordinates
    auto texcoords0 = new osg::Vec2Array;
    auto texcoords1 = new osg::Vec2Array;
    VNG.populateCenter(elevationLayer, colorLayer, atlas, _terrainTile, texcoords0, texcoords1);
    buffer._landGeometry->setTexCoordArray(0, texcoords0);
    buffer._landGeometry->setTexCoordArray(1, texcoords1);

    if (! _useTessellation) {
        // allocate and assign normals and the sea level mesh
        buffer._landGeometry->setNormalArray(VNG._normals.get(), osg::Array::BIND_PER_VERTEX);
        
        buffer._seaGeometry->setVertexArray(VNG._sea_vertices.get());
        buffer._seaGeometry->setNormalArray(VNG._sea_normals.get(), osg::Array::BIND_PER_VERTEX);

        // The Sea level mesh is identical to the main center mesh, except that it is at sea level
        // Therefore we can use the same texture coordinates calculated above.
        VNG.populateSeaLevel();
        buffer._seaGeometry->setTexCoordArray(0, texcoords0);
        buffer._seaGeometry->setTexCoordArray(1, texcoords1);
    }

    if (terrain)
    {
        TileID tileID = _terrainTile->getTileID();

        osg::ref_ptr<TerrainTile> left_tile  = terrain->getTile(TileID(tileID.level, tileID.x-1, tileID.y));
        osg::ref_ptr<TerrainTile> right_tile = terrain->getTile(TileID(tileID.level, tileID.x+1, tileID.y));
        osg::ref_ptr<TerrainTile> top_tile = terrain->getTile(TileID(tileID.level, tileID.x, tileID.y+1));
        osg::ref_ptr<TerrainTile> bottom_tile = terrain->getTile(TileID(tileID.level, tileID.x, tileID.y-1));

        VNG.populateLeftBoundary(left_tile.valid() ? left_tile->getElevationLayer() : 0, colorLayer, atlas);
        VNG.populateRightBoundary(right_tile.valid() ? right_tile->getElevationLayer() : 0, colorLayer, atlas);
        VNG.populateAboveBoundary(top_tile.valid() ? top_tile->getElevationLayer() : 0, colorLayer, atlas);
        VNG.populateBelowBoundary(bottom_tile.valid() ? bottom_tile->getElevationLayer() : 0, colorLayer, atlas);

        if (_useTessellation) {
            // If we're using tessellation then we also need corner data
            osg::ref_ptr<TerrainTile> bottom_left_tile = terrain->getTile(TileID(tileID.level, tileID.x-1, tileID.y-1));
            osg::ref_ptr<TerrainTile> bottom_right_tile = terrain->getTile(TileID(tileID.level, tileID.x+1, tileID.y-1));
            osg::ref_ptr<TerrainTile> top_left_tile = terrain->getTile(TileID(tileID.level, tileID.x-1, tileID.y+1));
            osg::ref_ptr<TerrainTile> top_right_tile = terrain->getTile(TileID(tileID.level, tileID.x+1, tileID.y+1));

            VNG.populateCorner(bottom_left_tile.valid() ? bottom_left_tile->getElevationLayer() : 0, colorLayer, atlas, VertexNormalGenerator::Corner::BOTTOM_LEFT);
            VNG.populateCorner(bottom_right_tile.valid() ? bottom_right_tile->getElevationLayer() : 0, colorLayer, atlas, VertexNormalGenerator::Corner::BOTTOM_RIGHT);
            VNG.populateCorner(top_left_tile.valid() ? top_left_tile->getElevationLayer() : 0, colorLayer, atlas, VertexNormalGenerator::Corner::TOP_LEFT);
            VNG.populateCorner(top_right_tile.valid() ? top_right_tile->getElevationLayer() : 0, colorLayer, atlas, VertexNormalGenerator::Corner::TOP_RIGHT);
        }

        _neighbours.clear();

        if (left_tile.valid())   addNeighbour(left_tile.get());
        if (right_tile.valid())  addNeighbour(right_tile.get());
        if (top_tile.valid())    addNeighbour(top_tile.get());
        if (bottom_tile.valid()) addNeighbour(bottom_tile.get());

        if (left_tile.valid())
        {
            if (left_tile->getTerrainTechnique()==0 || !(left_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = left_tile->getDirtyMask() | TerrainTile::LEFT_EDGE_DIRTY;
                left_tile->setDirtyMask(dirtyMask);
            }
        }
        if (right_tile.valid())
        {
            if (right_tile->getTerrainTechnique()==0 || !(right_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = right_tile->getDirtyMask() | TerrainTile::RIGHT_EDGE_DIRTY;
                right_tile->setDirtyMask(dirtyMask);
            }
        }
        if (top_tile.valid())
        {
            if (top_tile->getTerrainTechnique()==0 || !(top_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = top_tile->getDirtyMask() | TerrainTile::TOP_EDGE_DIRTY;
                top_tile->setDirtyMask(dirtyMask);
            }
        }
        if (bottom_tile.valid())
        {
            if (bottom_tile->getTerrainTechnique()==0|| !(bottom_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = bottom_tile->getDirtyMask() | TerrainTile::BOTTOM_EDGE_DIRTY;
                bottom_tile->setDirtyMask(dirtyMask);
            }
        }
    }

    if (_useTessellation) {

        //
        // populate the primitive data
        //
        bool smallTile = numVertices < 65536;

        osg::ref_ptr<osg::DrawElements> landElements = smallTile ?
            static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_PATCHES)) :
            static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_PATCHES));
        landElements->reserveElements((numRows-1) * (numColumns-1) * 16);
        buffer._landGeometry->addPrimitiveSet(landElements.get());

        int i, j;
        for (j = 0; j < (int) numRows-1; ++j) {
            for (i = 0; i < (int) numColumns-1; ++i) {
                std::vector<int> vertex_indices;
                vertex_indices.reserve(16);

                // Backup vertex index so we can handle edges with something reasonable.
                int last_vertex_index = VNG.vertex_index(i, j);

                for (int y = -1; y < 3; ++y) {
                    for (int x = -1; x < 3; ++x) {
                        int vertex_index = VNG.vertex_index(i+x, j+y);
                        if (vertex_index >= 0) {
                            vertex_indices.push_back(vertex_index);
                        } else {
                            vertex_indices.push_back(last_vertex_index);
                        }
                    }
                }

                if (vertex_indices.size() == 16) {
                    for (auto index : vertex_indices) {
                        landElements->addElement(index);
                    }
                }
            }
        }
    } else {
        // Non-tessellation case
        
        // Compute normals - though not sure why we would need to do that again?
        osg::ref_ptr<osg::Vec3Array> skirtVectors = new osg::Vec3Array((*VNG._normals));
        VNG.computeNormals();

        //
        // populate the primitive data
        //
        bool swapOrientation = !(buffer._masterLocator->orientationOpenGL());
        bool smallTile = numVertices < 65536;

        // OSG_NOTICE<<"smallTile = "<<smallTile<<std::endl;

        osg::ref_ptr<osg::DrawElements> landElements = smallTile ?
            static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
            static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
        landElements->reserveElements((numRows-1) * (numColumns-1) * 6);
        buffer._landGeometry->addPrimitiveSet(landElements.get());

        unsigned int i, j;
        for(j=0; j<numRows-1; ++j)
        {
            for(i=0; i<numColumns-1; ++i)
            {
                // remap indices to final vertex positions
                int i00 = VNG.vertex_index(i,   j);
                int i01 = VNG.vertex_index(i,   j+1);
                int i10 = VNG.vertex_index(i+1, j);
                int i11 = VNG.vertex_index(i+1, j+1);

                if (swapOrientation)
                {
                    std::swap(i00,i01);
                    std::swap(i10,i11);
                }

                unsigned int numValid = 0;
                if (i00>=0) ++numValid;
                if (i01>=0) ++numValid;
                if (i10>=0) ++numValid;
                if (i11>=0) ++numValid;

                if (numValid==4)
                {
                    // optimize which way to put the diagonal by choosing to
                    // place it between the two corners that have the least curvature
                    // relative to each other.
                    float dot_00_11 = (*VNG._normals)[i00] * (*VNG._normals)[i11];
                    float dot_01_10 = (*VNG._normals)[i01] * (*VNG._normals)[i10];

                    if (dot_00_11 > dot_01_10)
                    {
                        landElements->addElement(i01);
                        landElements->addElement(i00);
                        landElements->addElement(i11);

                        landElements->addElement(i00);
                        landElements->addElement(i10);
                        landElements->addElement(i11);
                    }
                    else
                    {
                        landElements->addElement(i01);
                        landElements->addElement(i00);
                        landElements->addElement(i10);

                        landElements->addElement(i01);
                        landElements->addElement(i10);
                        landElements->addElement(i11);
                    }
                }
                else if (numValid==3)
                {
                    if (i00>=0) landElements->addElement(i00);
                    if (i01>=0) landElements->addElement(i01);
                    if (i11>=0) landElements->addElement(i11);
                    if (i10>=0) landElements->addElement(i10);
                }
            }

            landElements->resizeElements(landElements->getNumIndices());
        }       

        osg::ref_ptr<osg::DrawElements> seaElements = smallTile ?
            static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
            static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
        seaElements->reserveElements((numRows-1) * (numColumns-1) * 6);
        buffer._seaGeometry->addPrimitiveSet(seaElements.get());

        for(j=0; j<numRows-1; ++j)
        {
            for(i=0; i<numColumns-1; ++i)
            {
                // remap sea indices to final vertex positions.  We're relying on
                // the indices for both the land and sea geometry to be identical.
                // That should be the case as long as the number of rows and columns
                // stays identical.
                int i00 = VNG.vertex_index(i,   j);
                int i01 = VNG.vertex_index(i,   j+1);
                int i10 = VNG.vertex_index(i+1, j);
                int i11 = VNG.vertex_index(i+1, j+1);

                if (swapOrientation)
                {
                    std::swap(i00,i01);
                    std::swap(i10,i11);
                }

                unsigned int numValid = 0;
                if (i00 >= 0) ++numValid;
                if (i01 >= 0) ++numValid;
                if (i10 >= 0) ++numValid;
                if (i11 >= 0) ++numValid;

                if (numValid==4)
                {
                    // optimize which way to put the diagonal by choosing to
                    // place it between the two corners that have the least curvature
                    // relative to each other.
                    float dot_00_11 = (*VNG._sea_normals)[i00] * (*VNG._sea_normals)[i11];
                    float dot_01_10 = (*VNG._sea_normals)[i01] * (*VNG._sea_normals)[i10];

                    if (dot_00_11 > dot_01_10)
                    {
                        seaElements->addElement(i01);
                        seaElements->addElement(i00);
                        seaElements->addElement(i11);

                        seaElements->addElement(i00);
                        seaElements->addElement(i10);
                        seaElements->addElement(i11);
                    }
                    else
                    {
                        seaElements->addElement(i01);
                        seaElements->addElement(i00);
                        seaElements->addElement(i10);

                        seaElements->addElement(i01);
                        seaElements->addElement(i10);
                        seaElements->addElement(i11);
                    }
                }
                else if (numValid==3)
                {
                    if (i00>=0) seaElements->addElement(i00);
                    if (i01>=0) seaElements->addElement(i01);
                    if (i11>=0) seaElements->addElement(i11);
                    if (i10>=0) seaElements->addElement(i10);
                }
            }
        }
        if (createSkirt)
        {
            osg::ref_ptr<osg::Vec3Array> vertices = VNG._vertices.get();
            osg::ref_ptr<osg::Vec3Array> normals = VNG._normals.get();

            osg::ref_ptr<osg::DrawElements> skirtDrawElements = smallTile ?
                static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
                static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));

            // create bottom skirt vertices
            int r,c;
            r=0;
            for(c=0;c<static_cast<int>(numColumns -1);++c)
            {
                // remap indices to final vertex positions
                int i00 = VNG.vertex_index(c,   r);
                int i01 = VNG.vertex_index(c+1, r);

                // Generate two additional skirt points below the edge
                int i10 = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[i00] - ((*skirtVectors)[i00])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i00]);
                texcoords0->push_back((*texcoords0)[i00]);
                texcoords1->push_back((*texcoords1)[i00]);

                int i11 = vertices->size(); // index of new index of added skirt point
                new_v = (*vertices)[i01] - ((*skirtVectors)[i01])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i01]);
                texcoords0->push_back((*texcoords0)[i01]);
                texcoords1->push_back((*texcoords1)[i01]);

                skirtDrawElements->addElement(i01);
                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i11);

                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i10);
                skirtDrawElements->addElement(i11);
            }

            if (skirtDrawElements->getNumIndices()!=0)
            {
                buffer._landGeometry->addPrimitiveSet(skirtDrawElements.get());
                skirtDrawElements = smallTile ?
                    static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
                    static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
            }

            // create right skirt vertices
            c=numColumns-1;
            for(r=0;r<static_cast<int>(numRows-1);++r)
            {
                // remap indices to final vertex positions
                int i00 = VNG.vertex_index(c,   r);
                int i01 = VNG.vertex_index(c, r+1);

                // Generate two additional skirt points below the edge
                int i10 = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[i00] - ((*skirtVectors)[i00])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i00]);
                texcoords0->push_back((*texcoords0)[i00]);
                texcoords1->push_back((*texcoords1)[i00]);

                int i11 = vertices->size(); // index of new index of added skirt point
                new_v = (*vertices)[i01] - ((*skirtVectors)[i01])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i01]);
                texcoords1->push_back((*texcoords1)[i01]);
                texcoords1->push_back((*texcoords1)[i01]);

                skirtDrawElements->addElement(i01);
                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i11);

                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i10);
                skirtDrawElements->addElement(i11);
            }

            if (skirtDrawElements->getNumIndices()!=0)
            {
                buffer._landGeometry->addPrimitiveSet(skirtDrawElements.get());
                skirtDrawElements = smallTile ?
                    static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
                    static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
            }

            // create top skirt vertices
            r=numRows-1;
            for(c=numColumns-2;c>=0;--c)
            {
                // remap indices to final vertex positions
                int i00 = VNG.vertex_index(c,   r);
                int i01 = VNG.vertex_index(c+1, r);

                // Generate two additional skirt points below the edge
                int i10 = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[i00] - ((*skirtVectors)[i00])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i00]);
                texcoords0->push_back((*texcoords0)[i00]);
                texcoords1->push_back((*texcoords1)[i00]);

                int i11 = vertices->size(); // index of new index of added skirt point
                new_v = (*vertices)[i01] - ((*skirtVectors)[i01])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i01]);
                texcoords0->push_back((*texcoords0)[i01]);
                texcoords1->push_back((*texcoords1)[i01]);

                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i01);
                skirtDrawElements->addElement(i11);

                skirtDrawElements->addElement(i10);
                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i11);
            }

            if (skirtDrawElements->getNumIndices()!=0)
            {
                buffer._landGeometry->addPrimitiveSet(skirtDrawElements.get());
                skirtDrawElements = smallTile ?
                    static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
                    static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));
            }

            // create left skirt vertices
            c=0;
            for(r=numRows-2;r>=0;--r)
            {
                // remap indices to final vertex positions
                int i00 = VNG.vertex_index(c,   r);
                int i01 = VNG.vertex_index(c, r+1);

                // Generate two additional skirt points below the edge
                int i10 = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[i00] - ((*skirtVectors)[i00])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i00]);
                texcoords0->push_back((*texcoords0)[i00]);
                texcoords1->push_back((*texcoords1)[i00]);

                int i11 = vertices->size(); // index of new index of added skirt point
                new_v = (*vertices)[i01] - ((*skirtVectors)[i01])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[i01]);
                texcoords0->push_back((*texcoords0)[i01]);
                texcoords1->push_back((*texcoords1)[i01]);

                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i01);
                skirtDrawElements->addElement(i11);

                skirtDrawElements->addElement(i10);
                skirtDrawElements->addElement(i00);
                skirtDrawElements->addElement(i11);
            }

            if (skirtDrawElements->getNumIndices()!=0)
            {
                buffer._landGeometry->addPrimitiveSet(skirtDrawElements.get());
            }
        }

        landElements->resizeElements(landElements->getNumIndices());
    }

    

    buffer._landGeometry->setUseDisplayList(false);
    buffer._landGeometry->setUseVertexBufferObjects(true);
    buffer._landGeometry->computeBoundingBox();

    if (! _useTessellation) 
    {
        buffer._landGeode->runGenerators(buffer._landGeometry);

        buffer._seaGeometry->setUseDisplayList(false);
        buffer._seaGeometry->setUseVertexBufferObjects(true);
        buffer._seaGeometry->computeBoundingBox();
        buffer._seaGeode->runGenerators(buffer._seaGeometry);
    }

    // Tile-specific information for the shaders
    osg::StateSet *landStateSet = buffer._landGeode->getOrCreateStateSet();
    osg::StateSet *seaStateSet;
    osg::ref_ptr<osg::Uniform> level = new osg::Uniform("tile_level", _terrainTile->getTileID().level);
    landStateSet->addUniform(level);
    if (_useTessellation) {
        landStateSet->setAttribute(new osg::PatchParameter(16));
    } else {
        seaStateSet = buffer._seaGeode->getOrCreateStateSet();
        seaStateSet->addUniform(level);
    }

    // Determine the x and y texture scaling.  Has to be performed after we've generated all the vertices.
    // Because the earth is round, each tile is not a rectangle.  Apart from edge cases like the poles, the
    // difference in axis length is < 1%, so we will just take the average.
    // Note that we can ignore the actual texture coordinates as we know from above that they are always
    // [0..1.0] [0..1.0] across the entire tile.
    osg::Vec3f bottom_left, bottom_right, top_left, top_right;
    bool got_bl = VNG.vertex(0, 0, bottom_left);
    bool got_br = VNG.vertex(0, VNG._numColumns - 1, bottom_right);
    bool got_tl = VNG.vertex(VNG._numColumns - 1, 0, top_left);
    bool got_tr = VNG.vertex(VNG._numColumns - 1, VNG._numRows -1, top_right);

    if (got_bl && got_br && got_tl && got_tr) {
        osg::Vec3f s = bottom_right - bottom_left;
        osg::Vec3f t = top_left - bottom_left;
        osg::Vec3f u = top_right - top_left;
        osg::Vec3f v = top_right - bottom_right;
        buffer._width = 0.5 * (s.length() + u.length());
        buffer._height = 0.5 * (t.length() + v.length());
    }

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Tile Level " << _terrainTile->getTileID().level << " width " << buffer._width << " height " << buffer._height);

    osg::ref_ptr<osg::Uniform> twu = new osg::Uniform("fg_tileWidth", buffer._width);
    landStateSet->addUniform(twu);
    if (! _useTessellation) seaStateSet->addUniform(twu);

    osg::ref_ptr<osg::Uniform> thu = new osg::Uniform("fg_tileHeight", buffer._height);
    landStateSet->addUniform(thu);
    if (! _useTessellation) seaStateSet->addUniform(thu);

    // Force build of KD trees?
    if (osgDB::Registry::instance()->getBuildKdTreesHint()==osgDB::ReaderWriter::Options::BUILD_KDTREES &&
        osgDB::Registry::instance()->getKdTreeBuilder())
    {

        //osg::Timer_t before = osg::Timer::instance()->tick();
        //OSG_NOTICE<<"osgTerrain::VPBTechnique::build kd tree"<<std::endl;
        osg::ref_ptr<osg::KdTreeBuilder> builder = osgDB::Registry::instance()->getKdTreeBuilder()->clone();
        buffer._landGeode->accept(*builder);
        if (! _useTessellation) buffer._seaGeode->accept(*builder);
        //osg::Timer_t after = osg::Timer::instance()->tick();
        //OSG_NOTICE<<"KdTree build time "<<osg::Timer::instance()->delta_m(before, after)<<std::endl;
    }
}

void VPBTechnique::applyColorLayers(BufferData& buffer, osg::ref_ptr<SGMaterialCache> matcache)
{   
    const SGPropertyNode* propertyNode = _options->getPropertyNode().get();
    Atlas* atlas = matcache->getAtlas();
    buffer._BVHMaterialMap = atlas->getBVHMaterialMap();

    auto tileID = _terrainTile->getTileID();
    const osg::Vec3d world = buffer._transform->getMatrix().getTrans();
    const SGGeod loc = SGGeod::fromCart(toSG(world));
    const SGBucket bucket = SGBucket(loc);

    bool photoScenery = false;

    if (propertyNode) {
        photoScenery = _options->getPropertyNode()->getBoolValue("/sim/rendering/photoscenery/enabled");
    }

    if (photoScenery) {
        // Photoscenery is enabled, so we need to find and assign the orthophoto texture

        // Firstly, we need to work out the texture file we want to load.  Fortunately this follows the same
        // naming convention as the VPB scenery itself.
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Using Photoscenery for " << _fileName << " " << tileID.level << " X" << tileID.x << " Y" << tileID.y);

        SGPath orthotexture;

        osgDB::FilePathList& pathList = _options->getDatabasePathList();
        bool found = false;

        for (auto iter = pathList.begin(); !found && iter != pathList.end(); ++iter) {
            orthotexture = SGPath(*iter);
            orthotexture.append("Orthophotos");
            orthotexture.append(bucket.gen_vpb_subtile(tileID.level, tileID.x, tileID.y) + ".dds");
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Looking for phototexture " << orthotexture);

            if (orthotexture.exists()) {
                found = true;
                SG_LOG(SG_TERRAIN, SG_DEBUG, "Found phototexture " << orthotexture);
            }
        }

        if (found) {

            osg::StateSet* landStateset = buffer._landGeode->getOrCreateStateSet();

            // Set up the texture with wrapping of UV to reduce black edges at tile boundaries.
            osg::Texture2D* texture = SGLoadTexture2D(SGPath(orthotexture), _options, true, true);
            texture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
            texture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
            landStateset->setTextureAttributeAndModes(0, texture);
            landStateset->setTextureAttributeAndModes(1, atlas->getImage(), osg::StateAttribute::ON);

            // Generate a water texture so we can use the water shader
            osg::ref_ptr<osg::Texture2D> waterTexture  = new osg::Texture2D;
            waterTexture->setImage(generateWaterTexture(atlas));
            waterTexture->setMaxAnisotropy(16.0f);
            waterTexture->setResizeNonPowerOfTwoHint(false);
            waterTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
            waterTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            waterTexture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
            waterTexture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
            // Overload of the coast texture
            landStateset->setTextureAttributeAndModes(7, waterTexture);
            
            
            landStateset->addUniform(new osg::Uniform(VPBTechnique::PHOTO_SCENERY, true));
            landStateset->addUniform(new osg::Uniform(VPBTechnique::MODEL_OFFSET, (osg::Vec3f) buffer._transform->getMatrix().getTrans()));
            atlas->addUniforms(landStateset);

            osg::StateSet* seaStateset = buffer._seaGeode->getOrCreateStateSet();
            seaStateset->setTextureAttributeAndModes(0, texture);
            seaStateset->setTextureAttributeAndModes(1, atlas->getImage(), osg::StateAttribute::ON);
            seaStateset->setTextureAttributeAndModes(7, waterTexture);
            seaStateset->addUniform(new osg::Uniform(VPBTechnique::PHOTO_SCENERY, true));
            seaStateset->addUniform(new osg::Uniform(VPBTechnique::MODEL_OFFSET, (osg::Vec3f) buffer._transform->getMatrix().getTrans()));
            atlas->addUniforms(seaStateset);
        } else {
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Unable to find " << orthotexture);
            photoScenery = false;
        }
    }

    if (!photoScenery) {
        // Either photoscenery is turned off, or we failed to find a suitable texture.

        osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);
        if (!colorLayer) return;

        osg::Image* image = colorLayer->getImage();
        if (!image || ! image->valid()) return;

        int raster_count[256] = {0};

        // Set the "g" color channel to an index into the atlas for the landclass.
        for (unsigned int s = 0; s < (unsigned int) image->s(); s++) {
            for (unsigned int t = 0; t < (unsigned int) image->t(); t++) {
                osg::Vec4d c = image->getColor(s, t);
                unsigned int i = (unsigned int) std::abs(std::round(c.x() * 255.0));
                c.set(c.x(), (double) (atlas->getIndex(i) / 255.0), atlas->isWater(i) ? 1.0 : 0.0, c.z());
                if (i < 256) {
                    raster_count[i]++;
                } else {
                    SG_LOG(SG_TERRAIN, SG_ALERT, "Raster value out of range: " << c.x() << " " << i);
                }
                image->setColor(c, s, t);
            }
        }

        // Simple statistics on the raster
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Landclass Raster " << _fileName << " Level " << tileID.level << " X" << tileID.x << " Y" << tileID.y);
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Raster Information:" << image->s() << "x" << image->t() << " (" << (image->s() * image->t()) << " pixels)" << " mipmaps:" << image->getNumMipmapLevels() << " format:" << image->getInternalTextureFormat());
        for (unsigned int i = 0; i < 256; ++i) { 
            if (raster_count[i] > 0) {
                SGMaterial* mat = matcache->find(i);
                if (mat) {
                    SG_LOG(SG_TERRAIN, SG_DEBUG, "  Landclass: " << i << " Material " <<  mat->get_names()[0] << " " << mat->get_one_texture(0,0) << " count: " << raster_count[i]);
                } else {
                    SG_LOG(SG_TERRAIN, SG_DEBUG, "  Landclass: " << i << " NO MATERIAL FOUND count : " << raster_count[i]);
                }
            }
        }

        osg::ref_ptr<osg::Texture2D> texture2D  = new osg::Texture2D;
        texture2D->setImage(image);
        texture2D->setMaxAnisotropy(16.0f);
        texture2D->setResizeNonPowerOfTwoHint(false);

        // Use mipmaps only in the minimization case because on magnification this results
        // in bad interpolation of boundaries between landclasses
        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
        texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

        texture2D->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
        texture2D->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);

        // Look for a pre-generated coastline texture.  There are two possible locations
        //  - Inside the vpb directory adjacent to this tile file.
        //  - Inside a 1x1 degree zipped file, which we can access using OSGs archive loader.
        osg::StateSet* landStateset = buffer._landGeode->getOrCreateStateSet();
        std::string filePath = "vpb/" + bucket.gen_vpb_filename(tileID.level, tileID.x, tileID.y, "coastline") + ".png";
        std::string archiveFilePath = "vpb/" + bucket.gen_vpb_archive_filename(tileID.level, tileID.x, tileID.y, "coastline") + ".png";
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Looking for coastline texture in " << filePath << " and " << archiveFilePath);

        // Check for the normal file first.  We go straight to the implementation here because we're already deep within
        // the registry code stack.
        osgDB::Registry* registry = osgDB::Registry::instance();
        osgDB::ReaderWriter::ReadResult result = registry->readImageImplementation(filePath, _options);
        if (result.notFound()) {
            // Check for the archive file next.  Note we only go down this path on a notFound() to avoid
            // masking errors.
            result = registry->readImageImplementation(archiveFilePath, _options);
        }

        if (result.success()) {
            buffer._waterRasterTexture = new osg::Texture2D(result.getImage());
            buffer._waterRasterTexture->getImage()->flipVertical();
            buffer._waterRasterTexture->setMaxAnisotropy(16.0f);
            buffer._waterRasterTexture->setResizeNonPowerOfTwoHint(false);
            buffer._waterRasterTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
            buffer._waterRasterTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
            buffer._waterRasterTexture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
            buffer._waterRasterTexture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Loaded coastline texture from " << filePath << " or " << archiveFilePath << " " << result.statusMessage());
        } else  {
            VPBRasterRenderer renderer = VPBRasterRenderer(propertyNode, _terrainTile, world, buffer._width, buffer._height);
            buffer._waterRasterTexture = renderer.generateCoastTexture();
        }

        landStateset->setTextureAttributeAndModes(0, texture2D, osg::StateAttribute::ON);
        landStateset->setTextureAttributeAndModes(1, atlas->getImage(), osg::StateAttribute::ON);
        landStateset->setTextureAttributeAndModes(7, buffer._waterRasterTexture, osg::StateAttribute::ON);
        landStateset->addUniform(new osg::Uniform(VPBTechnique::PHOTO_SCENERY, false));
        landStateset->addUniform(new osg::Uniform(VPBTechnique::MODEL_OFFSET, (osg::Vec3f) buffer._transform->getMatrix().getTrans()));
        atlas->addUniforms(landStateset);
        //SG_LOG(SG_TERRAIN, SG_ALERT, "modeOffset:" << buffer._transform->getMatrix().getTrans().length() << " " << buffer._transform->getMatrix().getTrans());

        osg::StateSet* seaStateset = buffer._seaGeode->getOrCreateStateSet();
        seaStateset->setTextureAttributeAndModes(0, texture2D, osg::StateAttribute::ON);
        seaStateset->setTextureAttributeAndModes(1, atlas->getImage(), osg::StateAttribute::ON);
        seaStateset->setTextureAttributeAndModes(7, buffer._waterRasterTexture, osg::StateAttribute::ON);
        seaStateset->addUniform(new osg::Uniform(VPBTechnique::PHOTO_SCENERY, false));
        seaStateset->addUniform(new osg::Uniform(VPBTechnique::MODEL_OFFSET, (osg::Vec3f) buffer._transform->getMatrix().getTrans()));
        atlas->addUniforms(seaStateset);
    }
}

double VPBTechnique::det2(const osg::Vec2d a, const osg::Vec2d b)
{
    return a.x() * b.y() - b.x() * a.y();
}

void VPBTechnique::applyMaterials(BufferData& buffer, osg::ref_ptr<SGMaterialCache> matcache, const SGGeod loc)
{
    if (_useTessellation)
        applyMaterialsTesselated(buffer, matcache, loc);
    else 
        applyMaterialsTriangles(buffer, matcache, loc);
}

osg::Vec4d VPBTechnique::catmull_rom_interp_basis(const float t)
{
    // Catmull-Rom basis matrix for tau=0.5.  See also fgdata/Shaders/HDR/ws30.tese
    // Note that GLSL is column-major, while OSG is row-major.
    osg::Matrixd catmull_rom_basis_M = osg::Matrixd(0.0,  1.0,  0.0,  0.0,
                                                   -0.5,  0.0,  0.5,  0.0,
                                                    1.0, -2.5,  2.0, -0.5,
                                                   -0.5,  1.5, -1.5,  0.5);

    return osg::Vec4d(1.0, t, t*t, t*t*t) * catmull_rom_basis_M;
}

void VPBTechnique::applyMaterialsTesselated(BufferData& buffer, osg::ref_ptr<SGMaterialCache> matcache, const SGGeod loc)
{
    assert(_useTessellation);
    if (!matcache) return;

    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);

    if (!colorLayer) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "No landclass image for " << _terrainTile->getTileID().x << " " << _terrainTile->getTileID().y << " " << _terrainTile->getTileID().level);
        return;
    }

    osg::Image* image = colorLayer->getImage();

    if (!image || ! image->valid()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "No landclass image for " << _terrainTile->getTileID().x << " " << _terrainTile->getTileID().y << " " << _terrainTile->getTileID().level);
        return;
    }

    pc_init((unsigned int) (loc.getLatitudeDeg() * loc.getLongitudeDeg() * 1000.0));

    // Define all possible handlers
    VegetationHandler vegetationHandler;
    RandomLightsHandler lightsHandler;
    std::vector<VPBMaterialHandler *> all_handlers{&vegetationHandler,
                                                   &lightsHandler};

    // Filter out handlers that do not apply to the current tile
    std::vector<VPBMaterialHandler *> handlers;
    for (const auto handler : all_handlers) {
        if (handler->initialize(_options, _terrainTile)) {
            handlers.push_back(handler);
        }
    }

    // If no handlers are relevant to the current tile, return immediately
    if (handlers.size() == 0) {
        return;
    }

    SGMaterial* mat = 0;

    const osg::PrimitiveSet* primSet = buffer._landGeometry->getPrimitiveSet(0);
    const osg::DrawElements* drawElements = primSet->getDrawElements();
    const osg::Array* vertices = buffer._landGeometry->getVertexArray();
    const osg::Array* texture_coords = buffer._landGeometry->getTexCoordArray(0);
    const osg::Vec3* vertexPtr = static_cast<const osg::Vec3*>(vertices->getDataPointer());
    const osg::Vec2* texPtr = static_cast<const osg::Vec2*>(texture_coords->getDataPointer());

    const unsigned int patchCount = drawElements->getNumIndices() / 16u;
    //const unsigned int dim = std::sqrt(patchCount);

    const double patchArea = buffer._width * buffer._height / (float) patchCount;

    //SG_LOG(SG_TERRAIN, SG_ALERT, "Number of Primitives: " << drawElements->getNumIndices() << " number of patches : " << patchCount << " patchArea (sqm): " << patchArea);

    // At the detailed tile level we are handling various materials, and
    // as we walk across the tile, the landclass doesn't change regularly 
    // from point to point within a given triangle.  Cache the required
    // material information for the current landclass to reduce the
    // number of lookups into the material cache.
    int current_land_class = -1;
    osg::Texture2D* object_mask = NULL;
    osg::Image* object_mask_image = NULL;
    float x_scale = 1000.0;
    float y_scale = 1000.0;

    for (unsigned int i = 0; i < patchCount; ++i) {
        // We're going to generate points in each patch in turn, which helps with 
        // temporal locality of materials.  Each patch is defined by 16 points,
        // and bicubic interpolation for any point within.  See ws30.tesce.
        double height[16];
        for (unsigned int j=0; j<16; ++j) {
            const unsigned int idx = drawElements->index(16 * i + j);
            const osg::Vec3 v = vertexPtr[idx];
            height[j] = (double) v[2];
        }

        const unsigned int idx0 = drawElements->index(16 * i + 5);  // Index of inner bottom left element
        const unsigned int idx1 = drawElements->index(16 * i + 6);  // Index of the inner bottom right element
        const unsigned int idx2 = drawElements->index(16 * i + 9);  // Index of the inner top left element

        // Determing both the location of the (0,0) point for this patch,
        // and the unit vectors in u and v for both the point and the texture.
        const osg::Vec3 v0 = vertexPtr[idx0];
        const osg::Vec3 vu = vertexPtr[idx1] - v0;
        const osg::Vec3 vv = vertexPtr[idx2] - v0;
        const osg::Vec2 t0 = texPtr[idx0];
        const osg::Vec2 tu = texPtr[idx1] - t0;
        const osg::Vec2 tv = texPtr[idx2] - t0;

        //SG_LOG(SG_TERRAIN, SG_ALERT, "Patch v0 " << v0.x() << ", " << v0.y() << ", " << v0.z());
        //SG_LOG(SG_TERRAIN, SG_ALERT, "Patch vu " << vu.x() << ", " << vu.y() << ", " << vu.z());
        //SG_LOG(SG_TERRAIN, SG_ALERT, "Patch vv " << vv.x() << ", " << vv.y() << ", " << vv.z());

        //SG_LOG(SG_TERRAIN, SG_ALERT, "Patch t0 " << t0.x() << ", " << t0.y());
        //SG_LOG(SG_TERRAIN, SG_ALERT, "Patch tu " << tu.x() << ", " << tu.y());
        //SG_LOG(SG_TERRAIN, SG_ALERT, "Patch tv " << tv.x() << ", " << tv.y());

        osg::Matrixd H = osg::Matrixd(height);
        osg::Matrixd HT;
        HT.transpose(H);

        for (const auto handler : handlers) {
            handler->setLocation(loc, 0.1, 0.1);

            // Determine the number of points to generate for this patch, using a zombie door method to handle low
            // densities.
            double zombie = pc_rand();
            unsigned int pt_count = static_cast<unsigned int>(std::floor(patchArea / handler->get_max_density_m2() + zombie));

            for (unsigned int k = 0; k < pt_count; ++k) {
                // Create a pseudo-random UV coordinate that is repeatable and relatively unique for this patch.
                double uvx   = pc_rand();
                double uvy   = pc_rand();
                double rand1 = pc_rand();
                double rand2 = pc_rand();
                osg::Vec2d uv = osg::Vec2d(uvx, uvy);

                // Location of this actual point.
                osg::Vec3 p = v0 + vu*uvx + vv*uvy;
                const osg::Vec2 t = t0 + tu*uvx + tv*uvy;

                // Determine the landclass from the texture coordinates
                unsigned int tx = (unsigned int) (image->s() * t.x()) % image->s();
                unsigned int ty = (unsigned int) (image->t() * t.y()) % image->t();
                const osg::Vec4 tc = image->getColor(tx, ty);
                const int land_class = int(std::round(tc.x() * 255.0));

                if (land_class != current_land_class) {
                    // Use temporal locality to reduce material lookup by caching
                    // some elements for future lookups against the same landclass.
                    mat = matcache->find(land_class);
                    if (!mat) {
                        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to find landclass " << land_class << " from point " << tx << ", " << ty);
                        continue;
                    }

                    current_land_class = land_class;

                    // We need to notify all handlers of material change, but
                    // only consider the current handler being processed for
                    // skipping the loop
                    bool current_handler_result = true;
                    for (const auto temp_handler : handlers) {
                        bool result = temp_handler->handleNewMaterial(mat);

                        if (temp_handler == handler) {
                            current_handler_result = result;
                        }
                    }

                    if (!current_handler_result) {
                        continue;
                    }

                    object_mask = mat->get_one_object_mask(0);
                    object_mask_image = NULL;
                    if (object_mask != NULL) {
                        object_mask_image = object_mask->getImage();
                        if (!object_mask_image || ! object_mask_image->valid()) {
                            object_mask_image = NULL;
                            continue;
                        }

                        // Texture coordinates run [0..1][0..1] across the entire tile whereas
                        // the texure itself has defined dimensions in m.
                        // We therefore need to use the tile width and height to determine the correct
                        // texture coordinate transformation.
                        x_scale = buffer._width / 1000.0;
                        y_scale = buffer._height / 1000.0;

                        if (mat->get_xsize() > 0.0) { x_scale = buffer._width / mat->get_xsize(); }
                        if (mat->get_ysize() > 0.0) { y_scale = buffer._height / mat->get_ysize(); }
                    }
                }

                if (!mat) continue;

                // Check against actual material density and objectMask.
                if (handler->handleIterationTessellation(mat, object_mask_image, t, rand1, rand2, x_scale, y_scale)) {

                    // Check against constraints to stop lights and objects on roads or water.
                    const osg::Vec3 upperPoint = p + osg::Vec3d(0.0,0.0, 9000.0);
                    const osg::Vec3 lowerPoint = p + osg::Vec3d(0.0,0.0, -300.0);

                    // Check against water
                    if (checkAgainstWaterConstraints(buffer, t))
                        continue;

                    if (checkAgainstRandomObjectsConstraints(buffer, lowerPoint, upperPoint))
                        continue;

                    const osg::Matrixd localToGeocentricTransform = buffer._transform->getMatrix();
                    if (checkAgainstElevationConstraints(lowerPoint * localToGeocentricTransform, upperPoint * localToGeocentricTransform))
                        continue;

                    // If we have got this far, then determine the points height using bicubic interpolation
                    // We can determine the height using the same calculations that will be used by the tesselation shader.
                    // See fgdata/Shaders/HDR/ws30.tese
                    osg::Vec4d u_basis = VPBTechnique::catmull_rom_interp_basis(uv.x());
                    osg::Vec4d v_basis = VPBTechnique::catmull_rom_interp_basis(uv.y());

                    osg::Vec4d hu = osg::Vec4d(
                        osg::Vec4d(H(0,0), H(1,0), H(2,0), H(3,0)) * u_basis,
                        osg::Vec4d(H(0,1), H(1,1), H(2,1), H(3,1)) * u_basis,
                        osg::Vec4d(H(0,2), H(1,2), H(2,2), H(3,2)) * u_basis,
                        osg::Vec4d(H(0,3), H(1,3), H(2,3), H(3,3)) * u_basis);

                    float h = hu *  v_basis; // Can also be dot(hv, u_basis)
                    p.set(p.x(), p.y(), h);

                    // Finally place the object
                    handler->placeObject(p);
                }
            }
        }
    }

    for (const auto handler : handlers) {
        handler->finish(_options, buffer._transform, loc);
    }
}

void VPBTechnique::applyMaterialsTriangles(BufferData& buffer, osg::ref_ptr<SGMaterialCache> matcache, const SGGeod loc)
{
    // XXX: This currently assumes we use triangles, so doesn't work with tessellation
    assert(! _useTessellation);
    if (!matcache) return;
    
    pc_init(2718281);

    // Define all possible handlers
    VegetationHandler vegetationHandler;
    RandomLightsHandler lightsHandler;
    std::vector<VPBMaterialHandler *> all_handlers{&vegetationHandler,
                                                   &lightsHandler};

    // Filter out handlers that do not apply to the current tile
    std::vector<VPBMaterialHandler *> handlers;
    for (const auto handler : all_handlers) {
        if (handler->initialize(_options, _terrainTile)) {
            handlers.push_back(handler);
        }
    }

    // If no handlers are relevant to the current tile, return immediately
    if (handlers.size() == 0) {
        return;
    }

    SGMaterial* mat = 0;

    osg::Vec3d up = buffer._transform->getMatrix().getTrans();
    up.normalize();

    if (!matcache) return;

    const osg::Array* vertices = buffer._landGeometry->getVertexArray();
    const osg::Array* texture_coords = buffer._landGeometry->getTexCoordArray(0);
    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);

    if (!colorLayer) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "No landclass image for " << _terrainTile->getTileID().x << " " << _terrainTile->getTileID().y << " " << _terrainTile->getTileID().level);
        return;
    }

    osg::Image* image = colorLayer->getImage();

    if (!image || ! image->valid()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "No landclass image for " << _terrainTile->getTileID().x << " " << _terrainTile->getTileID().y << " " << _terrainTile->getTileID().level);
        return;
    }

    const osg::Vec3* vertexPtr = static_cast<const osg::Vec3*>(vertices->getDataPointer());
    const osg::Vec2* texPtr = static_cast<const osg::Vec2*>(texture_coords->getDataPointer());

    const osg::PrimitiveSet* primSet = buffer._landGeometry->getPrimitiveSet(0);
    const osg::DrawElements* drawElements = primSet->getDrawElements();
    const unsigned int triangle_count = drawElements->getNumPrimitives();

    const double lon          = loc.getLongitudeRad();
    const double lat          = loc.getLatitudeRad();
    const double r_E_lat      = /* 6356752.3 */ 6.375993e+06;
    const double r_E_lon      = /* 6378137.0 */ 6.389377e+06;
    const double C            = r_E_lon * cos(lat);
    const double one_over_C   = (fabs(C) > 1.0e-4) ? (1.0 / C) : 0.0;
    const double one_over_r_E = 1.0 / r_E_lat;

    // Compute lat/lon deltas for each handler
    std::vector<std::pair<double, double>> deltas;
    for (const auto handler : handlers) {
        handler->setLocation(loc, r_E_lat, r_E_lon);
        deltas.push_back(
            std::make_pair(handler->get_delta_lat(), handler->get_delta_lon()));
    }

    // At the detailed tile level we are handling various materials, and
    // as we walk across the tile, the landclass doesn't change regularly 
    // from point to point within a given triangle.  Cache the required
    // material information for the current landclass to reduce the
    // number of lookups into the material cache.
    int current_land_class = -1;
    osg::Texture2D* object_mask = NULL;
    osg::Image* object_mask_image = NULL;
    float x_scale = 1000.0;
    float y_scale = 1000.0;

    for (unsigned int i = 0; i < triangle_count; i++)
    {
        const int i0 = drawElements->index(3 * i);
        const int i1 = drawElements->index(3 * i + 1);
        const int i2 = drawElements->index(3 * i + 2);

        const osg::Vec3 v0 = vertexPtr[i0];
        const osg::Vec3 v1 = vertexPtr[i1];
        const osg::Vec3 v2 = vertexPtr[i2];

        const osg::Vec3d v_0 = v0;
        const osg::Vec3d v_x = v1 - v0;
        const osg::Vec3d v_y = v2 - v0;

        osg::Vec3 n = v_x ^ v_y;
        n.normalize();

        const osg::Vec2d ll_0 = osg::Vec2d(v0.y() * one_over_C + lon, -v0.x() * one_over_r_E + lat);
        const osg::Vec2d ll_1 = osg::Vec2d(v1.y() * one_over_C + lon, -v1.x() * one_over_r_E + lat);
        const osg::Vec2d ll_2 = osg::Vec2d(v2.y() * one_over_C + lon, -v2.x() * one_over_r_E + lat);

        const osg::Vec2d ll_O = ll_0;
        const osg::Vec2d ll_x = osg::Vec2d((v1.y() - v0.y()) * one_over_C, -(v1.x() - v0.x()) * one_over_r_E);
        const osg::Vec2d ll_y = osg::Vec2d((v2.y() - v0.y()) * one_over_C, -(v2.x() - v0.x()) * one_over_r_E);

        // Each handler may have a different delta/granularity in the scanline.
        // To take advantage of the material caching, we first collect all the
        // scan points from all the handlers for the current triangle, and then 
        // call the appropriate handler for each point.
        std::vector<std::tuple<double, double, VPBMaterialHandler *>>
            scan_points;

        for(auto iter=0u; iter!=handlers.size(); iter++) {
            const double delta_lat = deltas[iter].first;
            const double delta_lon = deltas[iter].second;
            const int off_x = ll_O.x() / delta_lon;
            const int off_y = ll_O.y() / delta_lat;
            const int min_lon = min(min(ll_0.x(), ll_1.x()), ll_2.x()) / delta_lon;
            const int max_lon = max(max(ll_0.x(), ll_1.x()), ll_2.x()) / delta_lon;
            const int min_lat = min(min(ll_0.y(), ll_1.y()), ll_2.y()) / delta_lat;
            const int max_lat = max(max(ll_0.y(), ll_1.y()), ll_2.y()) / delta_lat;

            for (int lat_int = min_lat - 1; lat_int <= max_lat + 1; lat_int++) {
                const double lat = (lat_int - off_y) * delta_lat;
                for (int lon_int = min_lon - 1; lon_int <= max_lon + 1;
                     lon_int++) {
                    const double lon = (lon_int - off_x) * delta_lon;
                    scan_points.push_back(
                        std::make_tuple(lon, lat, handlers[iter]));
                }
            }
        }

        const osg::Vec2 t0 = texPtr[i0];
        const osg::Vec2 t1 = texPtr[i1];
        const osg::Vec2 t2 = texPtr[i2];

        const osg::Vec2d t_0 = t0;
        const osg::Vec2d t_x = t1 - t0;
        const osg::Vec2d t_y = t2 - t0;

        const double D = det2(ll_x, ll_y);

        for(auto const &point : scan_points) {
            const double lon = std::get<0>(point);
            const double lat = std::get<1>(point);
            VPBMaterialHandler *handler = std::get<2>(point);

            osg::Vec2d p(lon, lat);
            double x = det2(ll_x, p) / D;
            double y = det2(p, ll_y) / D;

            if ((x < 0.0) || (y < 0.0) || (x + y > 1.0)) continue;

            if (!image) {
                SG_LOG(SG_TERRAIN, SG_ALERT, "Image disappeared under my feet.");
                continue;
            }

            osg::Vec2 t = osg::Vec2(t_0 + t_x * x + t_y * y);
            unsigned int tx = (unsigned int) (image->s() * t.x()) % image->s();
            unsigned int ty = (unsigned int) (image->t() * t.y()) % image->t();
            const osg::Vec4 tc = image->getColor(tx, ty);
            const int land_class = int(std::round(tc.x() * 255.0));

            if (land_class != current_land_class) {
                // Use temporal locality to reduce material lookup by caching
                // some elements for future lookups against the same landclass.
                mat = matcache->find(land_class);
                if (!mat) continue;

                current_land_class = land_class;

                // We need to notify all handlers of material change, but
                // only consider the current handler being processed for
                // skipping the loop
                bool current_handler_result = true;
                for (const auto temp_handler : handlers) {
                    bool result = temp_handler->handleNewMaterial(mat);

                    if (temp_handler == handler) {
                        current_handler_result = result;
                    }
                }

                if (!current_handler_result) {
                    continue;
                }

                object_mask = mat->get_one_object_mask(0);
                object_mask_image = NULL;
                if (object_mask != NULL) {
                    object_mask_image = object_mask->getImage();
                    if (!object_mask_image || ! object_mask_image->valid()) {
                        object_mask_image = NULL;
                        continue;
                    }

                    // Texture coordinates run [0..1][0..1] across the entire tile whereas
                    // the texure itself has defined dimensions in m.
                    // We therefore need to use the tile width and height to determine the correct
                    // texture coordinate transformation.
                    x_scale = buffer._width / 1000.0;
                    y_scale = buffer._height / 1000.0;

                    if (mat->get_xsize() > 0.0) { x_scale = buffer._width / mat->get_xsize(); }
                    if (mat->get_ysize() > 0.0) { y_scale = buffer._height / mat->get_ysize(); }
                }
            }

            if (!mat) continue;

            osg::Vec2f pointInTriangle;

            if (handler->handleIteration(mat, object_mask_image,
                                         lon, lat, p,
                                         D, ll_O, ll_x, ll_y, t_0, t_x, t_y, x_scale, y_scale, pointInTriangle)) {

                // Check against constraints to stop lights and objects on roads or water.
                const osg::Vec3 vp = v_x * pointInTriangle.x() + v_y * pointInTriangle.y() + v_0;
                const osg::Vec2 tp = t_x * pointInTriangle.x() + t_y * pointInTriangle.y() + t_0;
                
                const osg::Vec3 upperPoint = vp + up * 100;
                const osg::Vec3 lowerPoint = vp - up * 100;

                // Check against water
                if (checkAgainstWaterConstraints(buffer, tp))
                    continue;

                if (checkAgainstRandomObjectsConstraints(buffer, lowerPoint, upperPoint))
                    continue;

                const osg::Matrixd localToGeocentricTransform = buffer._transform->getMatrix();
                if (checkAgainstElevationConstraints(lowerPoint * localToGeocentricTransform, upperPoint * localToGeocentricTransform))
                    continue;

                handler->placeObject(vp);
            }
        }
    }

    for (const auto handler : handlers) {
        handler->finish(_options, buffer._transform, loc);
    }
}

osg::Image* VPBTechnique::generateWaterTexture(Atlas* atlas) {  
    osg::Image* waterTexture = new osg::Image();

    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);
    if (!colorLayer) return waterTexture;

    osg::Image* image = colorLayer->getImage();
    if (!image || ! image->valid()) return waterTexture;

    waterTexture->allocateImage(image->s(), image->t(), 1, GL_RGBA, GL_FLOAT);

    // Set the r color channel to indicate if this is water or not
    for (unsigned int s = 0; s < (unsigned int) image->s(); s++) {
        for (unsigned int t = 0; t < (unsigned int) image->t(); t++) {            
            osg::Vec4d c = image->getColor(s, t);
            int i = int(std::round(c.x() * 255.0));
            waterTexture->setColor(osg::Vec4f(atlas->isWater(i) ? 1.0f : 0.0f,0.0f,0.0f,0.0f), s, t);
        }
    }

    return waterTexture;
}

void VPBTechnique::update(osg::NodeVisitor& nv)
{
    if (_terrainTile) _terrainTile->osg::Group::traverse(nv);

    if (_newBufferData.valid())
    {
        _currentBufferData = _newBufferData;
        _newBufferData = 0;
    }
}


void VPBTechnique::cull(osg::NodeVisitor& nv)
{
    if (_currentBufferData.valid())
    {
        if (_currentBufferData->_transform.valid())
        {
            _currentBufferData->_transform->accept(nv);
        }
    }
}


void VPBTechnique::traverse(osg::NodeVisitor& nv)
{
    if (!_terrainTile) return;

    // if app traversal update the frame count.
    if (nv.getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR)
    {
        if (_terrainTile->getDirty()) _terrainTile->init(_terrainTile->getDirtyMask(), false);
        update(nv);
        return;
    }
    else if (nv.getVisitorType()==osg::NodeVisitor::CULL_VISITOR)
    {
        cull(nv);
        return;
    }


    if (_terrainTile->getDirty())
    {
        OSG_INFO<<"******* Doing init ***********"<<std::endl;
        //_terrainTile->init(_terrainTile->getDirtyMask(), false);
    }

    if (_currentBufferData.valid())
    {
        if (_currentBufferData->_transform.valid()) _currentBufferData->_transform->accept(nv);
    }
}

void VPBTechnique::cleanSceneGraph()
{
}

void VPBTechnique::releaseGLObjects(osg::State* state) const
{
    if (_currentBufferData.valid() && _currentBufferData->_transform.valid()) _currentBufferData->_transform->releaseGLObjects(state);
    if (_newBufferData.valid() && _newBufferData->_transform.valid()) _newBufferData->_transform->releaseGLObjects(state);
}

// Add an osg object representing an elevation contraint on the terrain mesh.  The generated terrain mesh will not include any vertices that
// lie above the constraint model.  (Note that geometry may result in edges intersecting the constraint model in cases where there
// are significantly higher vertices that lie just outside the constraint model.
void VPBTechnique::addElevationConstraint(osg::ref_ptr<osg::Node> constraint)
{ 
    const std::lock_guard<std::mutex> lock(VPBTechnique::_elevationConstraintMutex); // Lock the _elevationConstraintGroup for this scope
    _elevationConstraintGroup->addChild(constraint.get()); 
}

// Remove a previously added constraint.  E.g on model unload.
void VPBTechnique::removeElevationConstraint(osg::ref_ptr<osg::Node> constraint)
{ 
    const std::lock_guard<std::mutex> lock(VPBTechnique::_elevationConstraintMutex); // Lock the _elevationConstraintGroup for this scope
    _elevationConstraintGroup->removeChild(constraint.get()); 
}

// Check a given vertex against any elevation constraints  E.g. to ensure the terrain mesh doesn't
// poke through any airport meshes.  If such a constraint exists, the function will return the elevation
// in local coordinates.
double VPBTechnique::getConstrainedElevation(osg::Vec3d ndc, Locator* masterLocator, double vtx_gap)
{
    const std::lock_guard<std::mutex> lock(VPBTechnique::_elevationConstraintMutex); // Lock the _elevationConstraintGroup for this scope

    osg::Vec3d origin, vertex;
    masterLocator->convertLocalToModel(osg::Vec3d(ndc.x(), ndc.y(), -1000), origin);
    masterLocator->convertLocalToModel(ndc, vertex);
    
    double elev = ndc.z();

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
    intersector = new osgUtil::LineSegmentIntersector(origin, vertex);
    osgUtil::IntersectionVisitor visitor(intersector.get());
    _elevationConstraintGroup->accept(visitor);

    if (intersector->containsIntersections()) {
        // We have an intersection with our constraints model, so determine the elevation
        osg::Vec3d intersect;
        masterLocator->convertModelToLocal(intersector->getFirstIntersection().getWorldIntersectPoint(), intersect);
        if (elev > intersect.z()) {
            // intersection is below the terrain mesh, so lower the terrain vertex, with an extra epsilon to avoid
            // z-buffer fighting and handle oddly shaped meshes.
            elev = intersect.z() - vtx_gap;
        }
    }

    return elev;
}

bool VPBTechnique::checkAgainstElevationConstraints(osg::Vec3d origin, osg::Vec3d vertex)
{
    const std::lock_guard<std::mutex> lock(VPBTechnique::_elevationConstraintMutex); // Lock the _elevationConstraintGroup for this scope
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
    intersector = new osgUtil::LineSegmentIntersector(origin, vertex);
    osgUtil::IntersectionVisitor visitor(intersector.get());
    _elevationConstraintGroup->accept(visitor);
    return intersector->containsIntersections();
}

bool VPBTechnique::checkAgainstWaterConstraints(BufferData& buffer, osg::Vec2d point)
{
    osg::Image* waterRaster = buffer._waterRasterTexture->getImage();
    if (waterRaster && waterRaster->getColor(point).b() > 0.05f) {
        // B channel contains water information.
        return true;
    } else {
        return false;
    }
}

bool VPBTechnique::checkAgainstRandomObjectsConstraints(BufferData& buffer, osg::Vec3d origin, osg::Vec3d vertex)
{
    if (buffer._lineFeatures) {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
        intersector = new osgUtil::LineSegmentIntersector(origin, vertex);
        osgUtil::IntersectionVisitor visitor(intersector.get());
        buffer._lineFeatures->accept(visitor);
        return intersector->containsIntersections();
    } else {
        return false;
    }
}

void VPBTechnique::clearConstraints()
{
    const std::lock_guard<std::mutex> elock(VPBTechnique::_elevationConstraintMutex); // Lock the _elevationConstraintGroup for this scope
    _elevationConstraintGroup = new osg::Group();
}

void VPBTechnique::updateStats(int tileLevel, float loadTime) {
    const std::lock_guard<std::mutex> lock(VPBTechnique::_stats_mutex); // Lock the _stats_mutex for this scope
    if (_loadStats.find(tileLevel) == _loadStats.end()) {
        _loadStats[tileLevel] = LoadStat(1, loadTime);
    } else {
        _loadStats[tileLevel].first++;
        _loadStats[tileLevel].second +=loadTime;
    }

    if (_statsPropertyNode) {
        _statsPropertyNode->getNode("level", tileLevel, true)->setIntValue("count", _loadStats[tileLevel].first);
        _statsPropertyNode->getNode("level", tileLevel, true)->setFloatValue("average-load-time-s", _loadStats[tileLevel].second / _loadStats[tileLevel].first);
    }
}

BVHMaterial* VPBTechnique::getMaterial(osg::Vec3d point) {
    osg::Vec3d local;
    _currentBufferData->_masterLocator->convertModelToLocal(point, local);

    auto image = _terrainTile->getColorLayer(0)->getImage();
    // Blue channel indicates if this is water, green channel is an index into the landclass data
    unsigned int tx = (unsigned int) (image->s() * local.x()) % image->s();
    unsigned int ty = (unsigned int) (image->t() * local.y()) % image->t();
    auto c = image->getColor(tx, ty);
    unsigned int lc = (unsigned int) std::abs(std::round(c.g() * 255.0));
    SGSharedPtr<SGMaterial> mat = _currentBufferData->_BVHMaterialMap[lc];
    if (mat) {
        //SG_LOG(SG_TERRAIN, SG_ALERT, "Material: " << mat->get_names()[0]);
        return mat;
    } else {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unexpected Landclass index in landclass texture: " << lc << " original texture value: " << c.g() << " at point " << local);
        return new BVHMaterial();
    }
}

SGSphered VPBTechnique::computeBoundingSphere() const {
    SGSphered bs;
    osg::Vec3d center = _currentBufferData->_transform->getBound().center();
    bs.setCenter(SGVec3d(center.x(), center.y(), center.z()));
    bs.setRadius(_currentBufferData->_transform->getBound().radius());
    return bs;
}
