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


#include <osgTerrain/TerrainTile>
#include <osgTerrain/Terrain>

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/MeshOptimizers>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture1D>
#include <osg/Program>
#include <osg/Math>
#include <osg/Timer>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/tgdb/VPBElevationSlice.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include "VPBTechnique.hxx"
#include "TreeBin.hxx"

using namespace osgTerrain;
using namespace simgear;

VPBTechnique::VPBTechnique()
{
    setFilterBias(0);
    setFilterWidth(0.1);
    setFilterMatrixAs(GAUSSIAN);
}

VPBTechnique::VPBTechnique(const SGReaderWriterOptions* options)
{
    setFilterBias(0);
    setFilterWidth(0.1);
    setFilterMatrixAs(GAUSSIAN);
    setOptions(options);
}

VPBTechnique::VPBTechnique(const VPBTechnique& gt,const osg::CopyOp& copyop):
    TerrainTechnique(gt,copyop)
{
    setFilterBias(gt._filterBias);
    setFilterWidth(gt._filterWidth);
    setFilterMatrix(gt._filterMatrix);
    setOptions(gt._options);
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

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_writeBufferMutex);

    osg::ref_ptr<TerrainTile> tile = _terrainTile;

    if (dirtyMask==0) return;

    osgTerrain::TileID tileID = tile->getTileID();
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Init of tile " << tileID.x << "," << tileID.y << " level " << tileID.level << " " << dirtyMask);

    osg::ref_ptr<BufferData> buffer = new BufferData;

    Locator* masterLocator = computeMasterLocator();

    osg::Vec3d centerModel = computeCenterModel(*buffer, masterLocator);

    if ((dirtyMask & TerrainTile::IMAGERY_DIRTY)==0)
    {
        generateGeometry(*buffer, masterLocator, centerModel);

        osg::ref_ptr<BufferData> read_buffer = _currentBufferData;

        osg::StateSet* stateset = read_buffer->_geode->getStateSet();
        if (stateset)
        {
            buffer->_geode->setStateSet(stateset);
        }
        else
        {
            applyColorLayers(*buffer, masterLocator);
            applyTrees(*buffer, masterLocator);
            applyLineFeatures(*buffer, masterLocator);
        }
    }
    else
    {
        generateGeometry(*buffer, masterLocator, centerModel);
        applyColorLayers(*buffer, masterLocator);
        applyTrees(*buffer, masterLocator);
        applyLineFeatures(*buffer, masterLocator);
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

osg::Vec3d VPBTechnique::computeCenter(BufferData& buffer, Locator* masterLocator)
{
    if (!masterLocator) return osg::Vec3d(0.0,0.0,0.0);

    osgTerrain::Layer* elevationLayer = _terrainTile->getElevationLayer();
    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);

    Locator* elevationLocator = elevationLayer ? elevationLayer->getLocator() : 0;
    Locator* colorLocator = colorLayer ? colorLayer->getLocator() : 0;

    if (!elevationLocator) elevationLocator = masterLocator;
    if (!colorLocator) colorLocator = masterLocator;

    osg::Vec3d bottomLeftNDC(DBL_MAX, DBL_MAX, 0.0);
    osg::Vec3d topRightNDC(-DBL_MAX, -DBL_MAX, 0.0);

    if (elevationLayer)
    {
        if (elevationLocator!= masterLocator)
        {
            masterLocator->computeLocalBounds(*elevationLocator, bottomLeftNDC, topRightNDC);
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
        if (colorLocator!= masterLocator)
        {
            masterLocator->computeLocalBounds(*colorLocator, bottomLeftNDC, topRightNDC);
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

osg::Vec3d VPBTechnique::computeCenterModel(BufferData& buffer, Locator* masterLocator)
{
    osg::Vec3d centerNDC = computeCenter(buffer, masterLocator);
    osg::Vec3d centerModel = centerNDC;
    masterLocator->convertLocalToModel(centerNDC, centerModel);

    buffer._transform = new osg::MatrixTransform;
    buffer._transform->setMatrix(osg::Matrix::translate(centerModel));

    return centerModel;
}

class VertexNormalGenerator
{
    public:

        typedef std::vector<int> Indices;
        typedef std::pair< osg::ref_ptr<osg::Vec2Array>, Locator* > TexCoordLocatorPair;
        typedef std::map< Layer*, TexCoordLocatorPair > LayerToTexCoordMap;

        VertexNormalGenerator(Locator* masterLocator, const osg::Vec3d& centerModel, int numRows, int numColmns, float scaleHeight, float vtx_gap, bool createSkirt);

        void populateCenter(osgTerrain::Layer* elevationLayer, LayerToTexCoordMap& layerToTexCoordMap);
        void populateLeftBoundary(osgTerrain::Layer* elevationLayer);
        void populateRightBoundary(osgTerrain::Layer* elevationLayer);
        void populateAboveBoundary(osgTerrain::Layer* elevationLayer);
        void populateBelowBoundary(osgTerrain::Layer* elevationLayer);

        void computeNormals();

        unsigned int capacity() const { return _vertices->capacity(); }

        inline void setVertex(int c, int r, const osg::Vec3& v, const osg::Vec3& n)
        {
            int& i = index(c,r);
            if (i==0)
            {
                if (r<0 || r>=_numRows || c<0 || c>=_numColumns)
                {
                    i = -(1+static_cast<int>(_boundaryVertices->size()));
                    _boundaryVertices->push_back(v);
                    // OSG_NOTICE<<"setVertex("<<c<<", "<<r<<", ["<<v<<"], ["<<n<<"]), i="<<i<<" _boundaryVertices["<<-i-1<<"]="<<(*_boundaryVertices)[-i-1]<<"]"<<std::endl;
                }
                else
                {
                    i = _vertices->size() + 1;
                    _vertices->push_back(v);
                    _normals->push_back(n);
                    // OSG_NOTICE<<"setVertex("<<c<<", "<<r<<", ["<<v<<"], ["<<n<<"]), i="<<i<<" _vertices["<<i-1<<"]="<<(*_vertices)[i-1]<<"]"<<std::endl;
                }
            }
            else if (i<0)
            {
                (*_boundaryVertices)[-i-1] = v;
                // OSG_NOTICE<<"setVertex("<<c<<", "<<r<<", ["<<v<<"], ["<<n<<"] _boundaryVertices["<<-i-1<<"]="<<(*_boundaryVertices)[-i-1]<<"]"<<std::endl;
            }
            else
            {
                // OSG_NOTICE<<"Overwriting setVertex("<<c<<", "<<r<<", ["<<v<<"], ["<<n<<"]"<<std::endl;
                // OSG_NOTICE<<"     previous values ( vertex ["<<(*_vertices)[i-1]<<"], normal (*_normals)[i-1] ["<<n<<"]"<<std::endl;
                // (*_vertices)[i-1] = v;

                // average the vertex positions
                (*_vertices)[i-1] = ((*_vertices)[i-1] + v)*0.5f;

                (*_normals)[i-1] = n;
            }
        }

        inline int& index(int c, int r) { return _indices[(r+1)*(_numColumns+2)+c+1]; }

        inline int index(int c, int r) const { return _indices[(r+1)*(_numColumns+2)+c+1]; }

        inline int vertex_index(int c, int r) const { int i = _indices[(r+1)*(_numColumns+2)+c+1]; return i-1; }

        inline bool vertex(int c, int r, osg::Vec3& v) const
        {
            int i = index(c,r);
            if (i==0) return false;
            if (i<0) v = (*_boundaryVertices)[-i-1];
            else v = (*_vertices)[i-1];
            return true;
        }

        inline bool computeNormal(int c, int r, osg::Vec3& n) const
        {
#if 1
            return computeNormalWithNoDiagonals(c,r,n);
#else
            return computeNormalWithDiagonals(c,r,n);
#endif
        }

        inline bool computeNormalWithNoDiagonals(int c, int r, osg::Vec3& n) const
        {
            osg::Vec3 center;
            bool center_valid  = vertex(c, r,  center);
            if (!center_valid) return false;

            osg::Vec3 left, right, top,  bottom;
            bool left_valid  = vertex(c-1, r,  left);
            bool right_valid = vertex(c+1, r,   right);
            bool bottom_valid = vertex(c,   r-1, bottom);
            bool top_valid = vertex(c,   r+1, top);

            osg::Vec3 dx(0.0f,0.0f,0.0f);
            osg::Vec3 dy(0.0f,0.0f,0.0f);
            osg::Vec3 zero(0.0f,0.0f,0.0f);
            if (left_valid)
            {
                dx += center-left;
            }
            if (right_valid)
            {
                dx += right-center;
            }
            if (bottom_valid)
            {
                dy += center-bottom;
            }
            if (top_valid)
            {
                dy += top-center;
            }

            if (dx==zero || dy==zero) return false;

            n = dx ^ dy;
            return n.normalize() != 0.0f;
        }

        inline bool computeNormalWithDiagonals(int c, int r, osg::Vec3& n) const
        {
            osg::Vec3 center;
            bool center_valid  = vertex(c, r,  center);
            if (!center_valid) return false;

            osg::Vec3 top_left, top_right, bottom_left, bottom_right;
            bool top_left_valid  = vertex(c-1, r+1,  top_left);
            bool top_right_valid  = vertex(c+1, r+1,  top_right);
            bool bottom_left_valid  = vertex(c-1, r-1,  bottom_left);
            bool bottom_right_valid  = vertex(c+1, r-1,  bottom_right);

            osg::Vec3 left, right, top,  bottom;
            bool left_valid  = vertex(c-1, r,  left);
            bool right_valid = vertex(c+1, r,   right);
            bool bottom_valid = vertex(c,   r-1, bottom);
            bool top_valid = vertex(c,   r+1, top);

            osg::Vec3 dx(0.0f,0.0f,0.0f);
            osg::Vec3 dy(0.0f,0.0f,0.0f);
            osg::Vec3 zero(0.0f,0.0f,0.0f);
            const float ratio = 0.5f;
            if (left_valid)
            {
                dx = center-left;
                if (top_left_valid) dy += (top_left-left)*ratio;
                if (bottom_left_valid) dy += (left-bottom_left)*ratio;
            }
            if (right_valid)
            {
                dx = right-center;
                if (top_right_valid) dy += (top_right-right)*ratio;
                if (bottom_right_valid) dy += (right-bottom_right)*ratio;
            }
            if (bottom_valid)
            {
                dy += center-bottom;
                if (bottom_left_valid) dx += (bottom-bottom_left)*ratio;
                if (bottom_right_valid) dx += (bottom_right-bottom)*ratio;
            }
            if (top_valid)
            {
                dy += top-center;
                if (top_left_valid) dx += (top-top_left)*ratio;
                if (top_right_valid) dx += (top_right-top)*ratio;
            }

            if (dx==zero || dy==zero) return false;

            n = dx ^ dy;
            return n.normalize() != 0.0f;
        }

        Locator*                        _masterLocator;
        const osg::Vec3d                _centerModel;
        int                             _numRows;
        int                             _numColumns;
        float                           _scaleHeight;
        float                           _constraint_vtx_gap;

        Indices                         _indices;

        osg::ref_ptr<osg::Vec3Array>    _vertices;
        osg::ref_ptr<osg::Vec3Array>    _normals;
        osg::ref_ptr<osg::FloatArray>   _elevations;

        osg::ref_ptr<osg::Vec3Array>    _boundaryVertices;

};

VertexNormalGenerator::VertexNormalGenerator(Locator* masterLocator, const osg::Vec3d& centerModel, int numRows, int numColumns, float scaleHeight, float vtx_gap, bool createSkirt):
    _masterLocator(masterLocator),
    _centerModel(centerModel),
    _numRows(numRows),
    _numColumns(numColumns),
    _scaleHeight(scaleHeight),
    _constraint_vtx_gap(vtx_gap)
{
    int numVerticesInBody = numColumns*numRows;
    int numVerticesInSkirt = createSkirt ? numColumns*2 + numRows*2 - 4 : 0;
    int numVertices = numVerticesInBody+numVerticesInSkirt;

    _indices.resize((_numRows+2)*(_numColumns+2),0);

    _vertices = new osg::Vec3Array;
    _vertices->reserve(numVertices);

    _normals = new osg::Vec3Array;
    _normals->reserve(numVertices);

    _elevations = new osg::FloatArray;
    _elevations->reserve(numVertices);

    _boundaryVertices = new osg::Vec3Array;
    _boundaryVertices->reserve(_numRows*2 + _numColumns*2 + 4);
}

void VertexNormalGenerator::populateCenter(osgTerrain::Layer* elevationLayer, LayerToTexCoordMap& layerToTexCoordMap)
{
    // OSG_NOTICE<<std::endl<<"VertexNormalGenerator::populateCenter("<<elevationLayer<<")"<<std::endl;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

    for(int j=0; j<_numRows; ++j)
    {
        for(int i=0; i<_numColumns; ++i)
        {
            osg::Vec3d ndc( ((double)i)/(double)(_numColumns-1), ((double)j)/(double)(_numRows-1), 0.0);

            bool validValue = true;
            if (elevationLayer)
            {
                float value = 0.0f;
                if (sampled) validValue = elevationLayer->getInterpolatedValidValue(ndc.x(), ndc.y(), value);
                else validValue = elevationLayer->getValidValue(i,j,value);
                ndc.z() = value*_scaleHeight;
            }

            if (validValue)
            {
                osg::Vec3d model;
                osg::Vec3d origin;
                _masterLocator->convertLocalToModel(osg::Vec3d(ndc.x(), ndc.y(), -1000), origin);
                _masterLocator->convertLocalToModel(ndc, model);

                model = VPBTechnique::checkAgainstElevationConstraints(origin, model, _constraint_vtx_gap);

                for(VertexNormalGenerator::LayerToTexCoordMap::iterator itr = layerToTexCoordMap.begin();
                    itr != layerToTexCoordMap.end();
                    ++itr)
                {
                    osg::Vec2Array* texcoords = itr->second.first.get();
                    osgTerrain::ImageLayer* imageLayer(dynamic_cast<osgTerrain::ImageLayer*>(itr->first));

                    if (imageLayer != NULL)
                    {
                        Locator* colorLocator = itr->second.second;
                        if (colorLocator != _masterLocator)
                        {
                            osg::Vec3d color_ndc;
                            Locator::convertLocalCoordBetween(*_masterLocator, ndc, *colorLocator, color_ndc);
                            (*texcoords).push_back(osg::Vec2(color_ndc.x(), color_ndc.y()));
                        }
                        else
                        {
                            (*texcoords).push_back(osg::Vec2(ndc.x(), ndc.y()));
                        }
                    }
                    else
                    {
                        osgTerrain::ContourLayer* contourLayer(dynamic_cast<osgTerrain::ContourLayer*>(itr->first));

                        bool texCoordSet = false;
                        if (contourLayer)
                        {
                            osg::TransferFunction1D* transferFunction = contourLayer->getTransferFunction();
                            if (transferFunction)
                            {
                                float difference = transferFunction->getMaximum()-transferFunction->getMinimum();
                                if (difference != 0.0f)
                                {
                                    osg::Vec3d           color_ndc;
                                    osgTerrain::Locator* colorLocator(itr->second.second);

                                    if (colorLocator != _masterLocator)
                                    {
                                        Locator::convertLocalCoordBetween(*_masterLocator,ndc,*colorLocator,color_ndc);
                                    }
                                    else
                                    {
                                        color_ndc = ndc;
                                    }

                                    color_ndc[2] /= _scaleHeight;

                                    (*texcoords).push_back(osg::Vec2((color_ndc[2]-transferFunction->getMinimum())/difference,0.0f));
                                    texCoordSet = true;
                                }
                            }
                        }
                        if (!texCoordSet)
                        {
                            (*texcoords).push_back(osg::Vec2(0.0f,0.0f));
                        }
                    }
                }

                if (_elevations.valid())
                {
                    (*_elevations).push_back(ndc.z());
                }

                // compute the local normal
                osg::Vec3d ndc_one = ndc; ndc_one.z() += 1.0;
                osg::Vec3d model_one;
                _masterLocator->convertLocalToModel(ndc_one, model_one);
                model_one = model_one - model;
                model_one.normalize();

                setVertex(i, j, osg::Vec3(model-_centerModel), model_one);
            }
        }
    }
}

void VertexNormalGenerator::populateLeftBoundary(osgTerrain::Layer* elevationLayer)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateLeftBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

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

                ndc.z() += 0.f;
            }
            if (validValue)
            {
                osg::Vec3d model;
                _masterLocator->convertLocalToModel(ndc, model);

                // compute the local normal
                osg::Vec3d ndc_one = ndc; ndc_one.z() += 1.0;
                osg::Vec3d model_one;
                _masterLocator->convertLocalToModel(ndc_one, model_one);
                model_one = model_one - model;
                model_one.normalize();

                setVertex(i, j, osg::Vec3(model-_centerModel), model_one);
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

void VertexNormalGenerator::populateRightBoundary(osgTerrain::Layer* elevationLayer)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateRightBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

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

            if (validValue)
            {
                osg::Vec3d model;
                _masterLocator->convertLocalToModel(ndc, model);

                // compute the local normal
                osg::Vec3d ndc_one = ndc; ndc_one.z() += 1.0;
                osg::Vec3d model_one;
                _masterLocator->convertLocalToModel(ndc_one, model_one);
                model_one = model_one - model;
                model_one.normalize();

                setVertex(i, j, osg::Vec3(model-_centerModel), model_one);
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

void VertexNormalGenerator::populateAboveBoundary(osgTerrain::Layer* elevationLayer)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateAboveBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

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

            if (validValue)
            {
                osg::Vec3d model;
                _masterLocator->convertLocalToModel(ndc, model);

                // compute the local normal
                osg::Vec3d ndc_one = ndc; ndc_one.z() += 1.0;
                osg::Vec3d model_one;
                _masterLocator->convertLocalToModel(ndc_one, model_one);
                model_one = model_one - model;
                model_one.normalize();

                setVertex(i, j, osg::Vec3(model-_centerModel), model_one);
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}

void VertexNormalGenerator::populateBelowBoundary(osgTerrain::Layer* elevationLayer)
{
    // OSG_NOTICE<<"   VertexNormalGenerator::populateBelowBoundary("<<elevationLayer<<")"<<std::endl;

    if (!elevationLayer) return;

    bool sampled = elevationLayer &&
                   ( (elevationLayer->getNumRows()!=static_cast<unsigned int>(_numRows)) ||
                     (elevationLayer->getNumColumns()!=static_cast<unsigned int>(_numColumns)) );

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

            if (validValue)
            {
                osg::Vec3d model;
                _masterLocator->convertLocalToModel(ndc, model);

                // compute the local normal
                osg::Vec3d ndc_one = ndc; ndc_one.z() += 1.0;
                osg::Vec3d model_one;
                _masterLocator->convertLocalToModel(ndc_one, model_one);
                model_one = model_one - model;
                model_one.normalize();

                setVertex(i, j, osg::Vec3(model-_centerModel), model_one);
                // OSG_NOTICE<<"       setVertex("<<i<<", "<<j<<"..)"<<std::endl;
            }
        }
    }
}


void VertexNormalGenerator::computeNormals()
{
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

void VPBTechnique::generateGeometry(BufferData& buffer, Locator* masterLocator, const osg::Vec3d& centerModel)
{
    Terrain* terrain = _terrainTile->getTerrain();
    osgTerrain::Layer* elevationLayer = _terrainTile->getElevationLayer();

    // Determine the correct Effect for this, based on a material lookup taking into account
    // the lat/lon of the center.
    SGPropertyNode_ptr effectProp;
    SGMaterialLibPtr matlib  = _options->getMaterialLib();

    if (matlib) {
        // Determine the center of the tile, sadly non-trivial.
      osg::Vec3d tileloc = computeCenter(buffer, masterLocator);
      osg::Vec3d world;
      masterLocator->convertLocalToModel(tileloc, world);
      const SGVec3d world2 = SGVec3d(world.x(), world.y(), world.z());
      const SGGeod loc = SGGeod::fromCart(world2);
      SG_LOG(SG_TERRAIN, SG_DEBUG, "Applying VPB material " << loc);
      SGMaterialCache* matcache = _options->getMaterialLib()->generateMatCache(loc, _options);
      SGMaterial* mat = matcache->find("ws30");
      delete matcache;

      if (mat) {
        effectProp = new SGPropertyNode();
        makeChild(effectProp, "inherits-from")->setStringValue(mat->get_effect_name());
      } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get effect for VPB - no matching material in library");
        effectProp = new SGPropertyNode;
        makeChild(effectProp.ptr(), "inherits-from")->setStringValue("Effects/model-default");
      }
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get effect for VPB - no material library available");
        effectProp = new SGPropertyNode;
        makeChild(effectProp.ptr(), "inherits-from")->setStringValue("Effects/model-default");
    }

    buffer._geode = new EffectGeode();
    if(buffer._transform.valid())
        buffer._transform->addChild(buffer._geode.get());

    buffer._geometry = new osg::Geometry;
    buffer._geode->addDrawable(buffer._geometry.get());
  
    osg::ref_ptr<Effect> effect = makeEffect(effectProp, true, _options);
    buffer._geode->setEffect(effect.get());

    osg::Geometry* geometry = buffer._geometry.get();

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
    VertexNormalGenerator VNG(masterLocator, centerModel, numRows, numColumns, scaleHeight, constraint_gap, createSkirt);

    unsigned int numVertices = VNG.capacity();

    // allocate and assign vertices
    geometry->setVertexArray(VNG._vertices.get());

    // allocate and assign normals
    geometry->setNormalArray(VNG._normals.get(), osg::Array::BIND_PER_VERTEX);


    // allocate and assign tex coords
    // typedef std::pair< osg::ref_ptr<osg::Vec2Array>, Locator* > TexCoordLocatorPair;
    // typedef std::map< Layer*, TexCoordLocatorPair > LayerToTexCoordMap;

    VertexNormalGenerator::LayerToTexCoordMap layerToTexCoordMap;
    for(unsigned int layerNum=0; layerNum<_terrainTile->getNumColorLayers(); ++layerNum)
    {
        osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(layerNum);
        if (colorLayer)
        {
            VertexNormalGenerator::LayerToTexCoordMap::iterator itr = layerToTexCoordMap.find(colorLayer);
            if (itr!=layerToTexCoordMap.end())
            {
                geometry->setTexCoordArray(layerNum, itr->second.first.get());
            }
            else
            {

                Locator* locator = colorLayer->getLocator();
                if (!locator)
                {
                    osgTerrain::SwitchLayer* switchLayer = dynamic_cast<osgTerrain::SwitchLayer*>(colorLayer);
                    if (switchLayer)
                    {
                        if (switchLayer->getActiveLayer()>=0 &&
                            static_cast<unsigned int>(switchLayer->getActiveLayer())<switchLayer->getNumLayers() &&
                            switchLayer->getLayer(switchLayer->getActiveLayer()))
                        {
                            locator = switchLayer->getLayer(switchLayer->getActiveLayer())->getLocator();
                        }
                    }
                }

                VertexNormalGenerator::TexCoordLocatorPair& tclp = layerToTexCoordMap[colorLayer];
                tclp.first = new osg::Vec2Array;
                tclp.first->reserve(numVertices);
                tclp.second = locator ? locator : masterLocator;
                geometry->setTexCoordArray(layerNum, tclp.first.get());
            }
        }
    }

    // allocate and assign color
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
    (*colors)[0].set(1.0f,1.0f,1.0f,1.0f);

    geometry->setColorArray(colors.get(), osg::Array::BIND_OVERALL);

    //
    // populate vertex and tex coord arrays
    //
    VNG.populateCenter(elevationLayer, layerToTexCoordMap);

    if (terrain && terrain->getEqualizeBoundaries())
    {
        TileID tileID = _terrainTile->getTileID();

        osg::ref_ptr<TerrainTile> left_tile  = terrain->getTile(TileID(tileID.level, tileID.x-1, tileID.y));
        osg::ref_ptr<TerrainTile> right_tile = terrain->getTile(TileID(tileID.level, tileID.x+1, tileID.y));
        osg::ref_ptr<TerrainTile> top_tile = terrain->getTile(TileID(tileID.level, tileID.x, tileID.y+1));
        osg::ref_ptr<TerrainTile> bottom_tile = terrain->getTile(TileID(tileID.level, tileID.x, tileID.y-1));

#if 0
        osg::ref_ptr<TerrainTile> top_left_tile  = terrain->getTile(TileID(tileID.level, tileID.x-1, tileID.y+1));
        osg::ref_ptr<TerrainTile> top_right_tile = terrain->getTile(TileID(tileID.level, tileID.x+1, tileID.y+1));
        osg::ref_ptr<TerrainTile> bottom_left_tile = terrain->getTile(TileID(tileID.level, tileID.x-1, tileID.y-1));
        osg::ref_ptr<TerrainTile> bottom_right_tile = terrain->getTile(TileID(tileID.level, tileID.x+1, tileID.y-1));
#endif
        VNG.populateLeftBoundary(left_tile.valid() ? left_tile->getElevationLayer() : 0);
        VNG.populateRightBoundary(right_tile.valid() ? right_tile->getElevationLayer() : 0);
        VNG.populateAboveBoundary(top_tile.valid() ? top_tile->getElevationLayer() : 0);
        VNG.populateBelowBoundary(bottom_tile.valid() ? bottom_tile->getElevationLayer() : 0);

        _neighbours.clear();

        bool updateNeighboursImmediately = false;

        if (left_tile.valid())   addNeighbour(left_tile.get());
        if (right_tile.valid())  addNeighbour(right_tile.get());
        if (top_tile.valid())    addNeighbour(top_tile.get());
        if (bottom_tile.valid()) addNeighbour(bottom_tile.get());

#if 0
        if (bottom_left_tile.valid()) addNeighbour(bottom_left_tile.get());
        if (bottom_right_tile.valid()) addNeighbour(bottom_right_tile.get());
        if (top_left_tile.valid()) addNeighbour(top_left_tile.get());
        if (top_right_tile.valid()) addNeighbour(top_right_tile.get());
#endif

        if (left_tile.valid())
        {
            if (left_tile->getTerrainTechnique()==0 || !(left_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = left_tile->getDirtyMask() | TerrainTile::LEFT_EDGE_DIRTY;
                if (updateNeighboursImmediately) left_tile->init(dirtyMask, true);
                else left_tile->setDirtyMask(dirtyMask);
            }
        }
        if (right_tile.valid())
        {
            if (right_tile->getTerrainTechnique()==0 || !(right_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = right_tile->getDirtyMask() | TerrainTile::RIGHT_EDGE_DIRTY;
                if (updateNeighboursImmediately) right_tile->init(dirtyMask, true);
                else right_tile->setDirtyMask(dirtyMask);
            }
        }
        if (top_tile.valid())
        {
            if (top_tile->getTerrainTechnique()==0 || !(top_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = top_tile->getDirtyMask() | TerrainTile::TOP_EDGE_DIRTY;
                if (updateNeighboursImmediately) top_tile->init(dirtyMask, true);
                else top_tile->setDirtyMask(dirtyMask);
            }
        }

        if (bottom_tile.valid())
        {
            if (bottom_tile->getTerrainTechnique()==0 || !(bottom_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = bottom_tile->getDirtyMask() | TerrainTile::BOTTOM_EDGE_DIRTY;
                if (updateNeighboursImmediately) bottom_tile->init(dirtyMask, true);
                else bottom_tile->setDirtyMask(dirtyMask);
            }
        }

#if 0
        if (bottom_left_tile.valid())
        {
            if (!(bottom_left_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = bottom_left_tile->getDirtyMask() | TerrainTile::BOTTOM_LEFT_CORNER_DIRTY;
                if (updateNeighboursImmediately) bottom_left_tile->init(dirtyMask, true);
                else bottom_left_tile->setDirtyMask(dirtyMask);
            }
        }

        if (bottom_right_tile.valid())
        {
            if (!(bottom_right_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = bottom_right_tile->getDirtyMask() | TerrainTile::BOTTOM_RIGHT_CORNER_DIRTY;
                if (updateNeighboursImmediately) bottom_right_tile->init(dirtyMask, true);
                else bottom_right_tile->setDirtyMask(dirtyMask);
            }
        }

        if (top_right_tile.valid())
        {
            if (!(top_right_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = top_right_tile->getDirtyMask() | TerrainTile::TOP_RIGHT_CORNER_DIRTY;
                if (updateNeighboursImmediately) top_right_tile->init(dirtyMask, true);
                else top_right_tile->setDirtyMask(dirtyMask);
            }
        }

        if (top_left_tile.valid())
        {
            if (!(top_left_tile->getTerrainTechnique()->containsNeighbour(_terrainTile)))
            {
                int dirtyMask = top_left_tile->getDirtyMask() | TerrainTile::TOP_LEFT_CORNER_DIRTY;
                if (updateNeighboursImmediately) top_left_tile->init(dirtyMask, true);
                else top_left_tile->setDirtyMask(dirtyMask);
            }
        }
#endif
    }

    osg::ref_ptr<osg::Vec3Array> skirtVectors = new osg::Vec3Array((*VNG._normals));
    VNG.computeNormals();

    //
    // populate the primitive data
    //
    bool swapOrientation = !(masterLocator->orientationOpenGL());
    bool smallTile = numVertices < 65536;

    // OSG_NOTICE<<"smallTile = "<<smallTile<<std::endl;

    osg::ref_ptr<osg::DrawElements> elements = smallTile ?
        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_TRIANGLES)) :
        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_TRIANGLES));

    elements->reserveElements((numRows-1) * (numColumns-1) * 6);

    geometry->addPrimitiveSet(elements.get());

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
                    elements->addElement(i01);
                    elements->addElement(i00);
                    elements->addElement(i11);

                    elements->addElement(i00);
                    elements->addElement(i10);
                    elements->addElement(i11);
                }
                else
                {
                    elements->addElement(i01);
                    elements->addElement(i00);
                    elements->addElement(i10);

                    elements->addElement(i01);
                    elements->addElement(i10);
                    elements->addElement(i11);
                }
            }
            else if (numValid==3)
            {
                if (i00>=0) elements->addElement(i00);
                if (i01>=0) elements->addElement(i01);
                if (i11>=0) elements->addElement(i11);
                if (i10>=0) elements->addElement(i10);
            }
        }
    }


    if (createSkirt)
    {
        osg::ref_ptr<osg::Vec3Array> vertices = VNG._vertices.get();
        osg::ref_ptr<osg::Vec3Array> normals = VNG._normals.get();

        osg::ref_ptr<osg::DrawElements> skirtDrawElements = smallTile ?
            static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
            static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));

        // create bottom skirt vertices
        int r,c;
        r=0;
        for(c=0;c<static_cast<int>(numColumns);++c)
        {
            int orig_i = VNG.vertex_index(c,r);
            if (orig_i>=0)
            {
                unsigned int new_i = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[orig_i]);

                for(VertexNormalGenerator::LayerToTexCoordMap::iterator itr = layerToTexCoordMap.begin();
                    itr != layerToTexCoordMap.end();
                    ++itr)
                {
                    itr->second.first->push_back((*itr->second.first)[orig_i]);
                }

                skirtDrawElements->addElement(orig_i);
                skirtDrawElements->addElement(new_i);
            }
            else
            {
                if (skirtDrawElements->getNumIndices()!=0)
                {
                    geometry->addPrimitiveSet(skirtDrawElements.get());
                    skirtDrawElements = smallTile ?
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));
                }

            }
        }

        if (skirtDrawElements->getNumIndices()!=0)
        {
            geometry->addPrimitiveSet(skirtDrawElements.get());
            skirtDrawElements = smallTile ?
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));
        }

        // create right skirt vertices
        c=numColumns-1;
        for(r=0;r<static_cast<int>(numRows);++r)
        {
            int orig_i = VNG.vertex_index(c,r); // index of original vertex of grid
            if (orig_i>=0)
            {
                unsigned int new_i = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[orig_i]);
                for(VertexNormalGenerator::LayerToTexCoordMap::iterator itr = layerToTexCoordMap.begin();
                    itr != layerToTexCoordMap.end();
                    ++itr)
                {
                    itr->second.first->push_back((*itr->second.first)[orig_i]);
                }

                skirtDrawElements->addElement(orig_i);
                skirtDrawElements->addElement(new_i);
            }
            else
            {
                if (skirtDrawElements->getNumIndices()!=0)
                {
                    geometry->addPrimitiveSet(skirtDrawElements.get());
                    skirtDrawElements = smallTile ?
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));
                }

            }
        }

        if (skirtDrawElements->getNumIndices()!=0)
        {
            geometry->addPrimitiveSet(skirtDrawElements.get());
            skirtDrawElements = smallTile ?
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));
        }

        // create top skirt vertices
        r=numRows-1;
        for(c=numColumns-1;c>=0;--c)
        {
            int orig_i = VNG.vertex_index(c,r); // index of original vertex of grid
            if (orig_i>=0)
            {
                unsigned int new_i = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[orig_i]);
                for(VertexNormalGenerator::LayerToTexCoordMap::iterator itr = layerToTexCoordMap.begin();
                    itr != layerToTexCoordMap.end();
                    ++itr)
                {
                    itr->second.first->push_back((*itr->second.first)[orig_i]);
                }

                skirtDrawElements->addElement(orig_i);
                skirtDrawElements->addElement(new_i);
            }
            else
            {
                if (skirtDrawElements->getNumIndices()!=0)
                {
                    geometry->addPrimitiveSet(skirtDrawElements.get());
                    skirtDrawElements = smallTile ?
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));
                }

            }
        }

        if (skirtDrawElements->getNumIndices()!=0)
        {
            geometry->addPrimitiveSet(skirtDrawElements.get());
            skirtDrawElements = smallTile ?
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUShort(GL_QUAD_STRIP)) :
                        static_cast<osg::DrawElements*>(new osg::DrawElementsUInt(GL_QUAD_STRIP));
        }

        // create left skirt vertices
        c=0;
        for(r=numRows-1;r>=0;--r)
        {
            int orig_i = VNG.vertex_index(c,r); // index of original vertex of grid
            if (orig_i>=0)
            {
                unsigned int new_i = vertices->size(); // index of new index of added skirt point
                osg::Vec3 new_v = (*vertices)[orig_i] - ((*skirtVectors)[orig_i])*skirtHeight;
                (*vertices).push_back(new_v);
                if (normals.valid()) (*normals).push_back((*normals)[orig_i]);
                for(VertexNormalGenerator::LayerToTexCoordMap::iterator itr = layerToTexCoordMap.begin();
                    itr != layerToTexCoordMap.end();
                    ++itr)
                {
                    itr->second.first->push_back((*itr->second.first)[orig_i]);
                }

                skirtDrawElements->addElement(orig_i);
                skirtDrawElements->addElement(new_i);
            }
            else
            {
                if (skirtDrawElements->getNumIndices()!=0)
                {
                    geometry->addPrimitiveSet(skirtDrawElements.get());
                    skirtDrawElements = new osg::DrawElementsUShort(GL_QUAD_STRIP);
                }
            }
        }

        if (skirtDrawElements->getNumIndices()!=0)
        {
            geometry->addPrimitiveSet(skirtDrawElements.get());
        }
    }


    geometry->setUseDisplayList(false);
    geometry->setUseVertexBufferObjects(true);

#if 0
    {
        osgUtil::VertexCacheMissVisitor vcmv_before;
        osgUtil::VertexCacheMissVisitor vcmv_after;
        osgUtil::VertexCacheVisitor vcv;
        osgUtil::VertexAccessOrderVisitor vaov;

        vcmv_before.doGeometry(*geometry);
        vcv.optimizeVertices(*geometry);
        vaov.optimizeOrder(*geometry);
        vcmv_after.doGeometry(*geometry);
#if 0
        OSG_NOTICE<<"vcmv_before.triangles="<<vcmv_before.triangles<<std::endl;
        OSG_NOTICE<<"vcmv_before.misses="<<vcmv_before.misses<<std::endl;
        OSG_NOTICE<<"vcmv_after.misses="<<vcmv_after.misses<<std::endl;
        OSG_NOTICE<<std::endl;
#endif
    }
#endif

    // Tile-specific information for the shaders
    osg::StateSet *ss = buffer._geode->getOrCreateStateSet();
    osg::ref_ptr<osg::Uniform> level = new osg::Uniform("tile_level", _terrainTile->getTileID().level);
    ss->addUniform(level);

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

    float tile_width = 1.0;
    float tile_height = 1.0;
    if (got_bl && got_br && got_tl && got_tr) {
        auto s = bottom_right - bottom_left;
        auto t = top_left - bottom_left;
        auto u = top_right - top_left;
        auto v = top_right - bottom_right;
        tile_width = 0.5 * (s.length() + u.length());
        tile_height = 0.5 * (t.length() + v.length());
    }

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Tile Level " << _terrainTile->getTileID().level << " width " << tile_width << " height " << tile_height);

    osg::ref_ptr<osg::Uniform> twu = new osg::Uniform("tile_width", tile_width);
    ss->addUniform(twu);
    osg::ref_ptr<osg::Uniform> thu = new osg::Uniform("tile_height", tile_height);
    ss->addUniform(thu);

    // Force build of KD trees?
    if (osgDB::Registry::instance()->getBuildKdTreesHint()==osgDB::ReaderWriter::Options::BUILD_KDTREES &&
        osgDB::Registry::instance()->getKdTreeBuilder())
    {

        //osg::Timer_t before = osg::Timer::instance()->tick();
        //OSG_NOTICE<<"osgTerrain::VPBTechnique::build kd tree"<<std::endl;
        osg::ref_ptr<osg::KdTreeBuilder> builder = osgDB::Registry::instance()->getKdTreeBuilder()->clone();
        buffer._geode->accept(*builder);
        //osg::Timer_t after = osg::Timer::instance()->tick();
        //OSG_NOTICE<<"KdTree build time "<<osg::Timer::instance()->delta_m(before, after)<<std::endl;
    }
}

void VPBTechnique::applyColorLayers(BufferData& buffer, Locator* masterLocator)
{   
    typedef std::map<osgTerrain::Layer*, SGMaterialCache::Atlas> LayerToAtlasMap;
    typedef std::map<osgTerrain::Layer*, osg::ref_ptr<osg::Texture2D>> LayerToTexture2DMap;
    typedef std::map<osgTerrain::Layer*, osg::ref_ptr<osg::Texture1D>> LayerToTexture1DMap;
    LayerToAtlasMap layerToAtlasMap;
    LayerToTexture2DMap layerToTextureMap; 
    LayerToTexture1DMap layerToContourMap;

    for(unsigned int layerNum=0; layerNum<_terrainTile->getNumColorLayers(); ++layerNum)
    {
        osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(layerNum);
        if (!colorLayer) continue;

        osgTerrain::SwitchLayer* switchLayer = dynamic_cast<osgTerrain::SwitchLayer*>(colorLayer);
        if (switchLayer)
        {
            if (switchLayer->getActiveLayer()<0 ||
                static_cast<unsigned int>(switchLayer->getActiveLayer())>=switchLayer->getNumLayers())
            {
                continue;
            }

            colorLayer = switchLayer->getLayer(switchLayer->getActiveLayer());
            if (!colorLayer) continue;
        }

        osg::Image* image = colorLayer->getImage();
        if (!image) continue;

        osgTerrain::ImageLayer* imageLayer = dynamic_cast<osgTerrain::ImageLayer*>(colorLayer);
        osgTerrain::ContourLayer* contourLayer = dynamic_cast<osgTerrain::ContourLayer*>(colorLayer);
        SGMaterialCache::Atlas atlas = layerToAtlasMap[colorLayer];
        if (imageLayer)
        {
            osg::StateSet* stateset = buffer._geode->getOrCreateStateSet();

            osg::ref_ptr<osg::Texture2D> texture2D  = layerToTextureMap[colorLayer];
            SGMaterialCache::Atlas atlas = layerToAtlasMap[colorLayer];

            if (!texture2D)
            {
                texture2D  = new osg::Texture2D;

                // First time generating this texture, so process to change landclass IDs to texure indexes.
                SGPropertyNode_ptr effectProp;
                SGMaterialLibPtr matlib  = _options->getMaterialLib();
                osg::Vec3d world;
                SGGeod loc;

                if (matlib)                    
                {
                    SG_LOG(SG_TERRAIN, SG_DEBUG, "Generating texture atlas for " << image->getName());
                    // Determine the center of the tile, sadly non-trivial.
                    osg::Vec3d tileloc = computeCenter(buffer, masterLocator);
                    masterLocator->convertLocalToModel(tileloc, world);
                    const SGVec3d world2 = SGVec3d(world.x(), world.y(), world.z());
                    loc = SGGeod::fromCart(world2);
                    SG_LOG(SG_TERRAIN, SG_DEBUG, "Applying VPB material " << loc);

                    if (_matcache ==0) _matcache = _options->getMaterialLib()->generateMatCache(loc, _options);
                    
                    atlas = _matcache->getAtlas();
                    SGMaterialCache::AtlasIndex atlasIndex = atlas.index;

                    // Set the "g" color channel to an index into the atlas index.
                    for (int s = 0; s < image->s(); s++) {
                        for (int t = 0; t < image->t(); t++) {
                            osg::Vec4 c = image->getColor(s, t);
                            int i = int(round(c.x() * 255.0));
                            c.set(c.x(), (float) (atlasIndex[i] / 255.0), c.z(), c.w() );
                            image->setColor(c, s, t);
                        }
                    }

                }

                texture2D->setImage(image);
                texture2D->setMaxAnisotropy(16.0f);
                texture2D->setResizeNonPowerOfTwoHint(false);

                texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
                texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

                texture2D->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
                texture2D->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);

                bool mipMapping = !(texture2D->getFilter(osg::Texture::MIN_FILTER)==osg::Texture::LINEAR || texture2D->getFilter(osg::Texture::MIN_FILTER)==osg::Texture::NEAREST);
                bool s_NotPowerOfTwo = image->s()==0 || (image->s() & (image->s() - 1));
                bool t_NotPowerOfTwo = image->t()==0 || (image->t() & (image->t() - 1));

                if (mipMapping && (s_NotPowerOfTwo || t_NotPowerOfTwo))
                {
                    SG_LOG(SG_TERRAIN, SG_ALERT, "Disabling mipmapping for non power of two tile size("<<image->s()<<", "<<image->t()<<")");
                    texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                }

                layerToTextureMap[colorLayer] = texture2D;
                layerToAtlasMap[colorLayer] = atlas;
            }

            stateset->setTextureAttributeAndModes(5*layerNum, texture2D, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(5*layerNum + 1, atlas.image, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(5*layerNum + 2, atlas.dimensions, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(5*layerNum + 3, atlas.diffuse, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(5*layerNum + 4, atlas.specular, osg::StateAttribute::ON);
        }
        else if (contourLayer)
        {
            osg::StateSet* stateset = buffer._geode->getOrCreateStateSet();

            osg::ref_ptr< osg::Texture1D> texture1D = layerToContourMap[colorLayer];
            if (!texture1D)
            {
                texture1D = new osg::Texture1D;
                texture1D->setImage(image);
                texture1D->setResizeNonPowerOfTwoHint(false);
                texture1D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
                texture1D->setFilter(osg::Texture::MAG_FILTER, colorLayer->getMagFilter());

                layerToContourMap[colorLayer] = texture1D;
            }

            stateset->setTextureAttributeAndModes(layerNum, texture1D, osg::StateAttribute::ON);
        }
    }
}

double VPBTechnique::det2(const osg::Vec2d a, const osg::Vec2d b)
{
    return a.x() * b.y() - b.x() * a.y();
}

osg::Vec2d* VPBTechnique::_randomOffsets = 0;

osg::Vec2d VPBTechnique::getRandomOffset(int lon, int lat)
{
    const unsigned initial_seed = 42;
    const int N_rnd = 1; // 1 is a uniform random distribution, increase to 5 or
                         // more for plantation
    const int bits = 5;
    const int N    = 1 << (2 * bits);
    const int N_2  = 1 <<      bits;
    const int mask = N_2 - 1;

    if (!_randomOffsets)
    {
        _randomOffsets = new osg::Vec2d[N];
        mt seed;
        mt_init(&seed, initial_seed);

        for (int i = 0; i < N; i++)
        {
            double x = 0.0;
            double y = 0.0;

            for (int j = 0; j < N_rnd; j++)
            {
                x += mt_rand(&seed);
                y += mt_rand(&seed);
            }

            x = x / N_rnd - 0.5;
            y = y / N_rnd - 0.5;
            _randomOffsets[i].set(x, y);
        }

    }

    return _randomOffsets[N_2 * (lat & mask) + (lon & mask)];
}

void VPBTechnique::applyTrees(BufferData& buffer, Locator* masterLocator)
{
    bool use_random_vegetation = false;
    float vegetation_density = 1.0; 
    int vegetation_lod_level = 6;
    float randomness = 1.0;

    SGPropertyNode* propertyNode = _options->getPropertyNode().get();

    if (propertyNode) {
        use_random_vegetation = propertyNode->getBoolValue("/sim/rendering/random-vegetation", use_random_vegetation);
        vegetation_density = propertyNode->getFloatValue("/sim/rendering/vegetation-density", vegetation_density);
        vegetation_lod_level = propertyNode->getIntValue("/sim/rendering/static-lod/vegetation-lod-level", vegetation_lod_level);
    }

    // Do not generate vegetation for tiles too far away or if we explicitly don't generate vegetation
    if ((!use_random_vegetation) || (_terrainTile->getTileID().level < vegetation_lod_level)) {
        return;
    }

    SGMaterialLibPtr matlib  = _options->getMaterialLib();
    SGMaterial* mat = 0;
    osg::Vec3d world;
    SGGeod loc;
    SGGeoc cloc;

    if (matlib)
    {
        // Determine the center of the tile, sadly non-trivial.
        osg::Vec3d tileloc = computeCenter(buffer, masterLocator);
        masterLocator->convertLocalToModel(tileloc, world);
        const SGVec3d world2 = SGVec3d(world.x(), world.y(), world.z());
        loc = SGGeod::fromCart(world2);
        cloc = SGGeoc::fromCart(world2);
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Applying VPB material " << loc);
        SGMaterialCache* matcache = _options->getMaterialLib()->generateMatCache(loc, _options);
        mat = matcache->find("EvergreenForest");
        delete matcache;
    }

    if (!mat) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to find EvergreenForestmaterial at " << loc);
        return;
    }    

    TreeBin* bin = new TreeBin();
    bin->texture = mat->get_tree_texture();
    bin->teffect = mat->get_tree_effect();
    bin->range   = mat->get_tree_range();
    bin->width   = mat->get_tree_width();
    bin->height  = mat->get_tree_height();
    bin->texture_varieties = mat->get_tree_varieties();

    const osg::Matrixd R_vert = osg::Matrixd::rotate(
     M_PI / 2.0 - loc.getLatitudeRad(), osg::Vec3d(0.0, 1.0, 0.0),
     loc.getLongitudeRad(), osg::Vec3d(0.0, 0.0, 1.0),
     0.0, osg::Vec3d(1.0, 0.0, 0.0));

    const osg::Array* vertices = buffer._geometry->getVertexArray();
    const osg::Array* texture_coords = buffer._geometry->getTexCoordArray(0);
    osgTerrain::Layer* colorLayer = _terrainTile->getColorLayer(0);
    osg::Image* image = colorLayer->getImage();

    // Just want the first primitive set, not the others which will be the "skirts" below.
    const osg::PrimitiveSet* primSet = buffer._geometry->getPrimitiveSet(0);
    const osg::DrawElements* drawElements = primSet->getDrawElements();
    const unsigned int triangle_count = drawElements->getNumPrimitives();

    const double lon          = loc.getLongitudeRad();
    const double lat          = loc.getLatitudeRad();
    const double clon         = cloc.getLongitudeRad();
    const double clat         = cloc.getLatitudeRad();
    const double r_E_lat      = /* 6356752.3 */ 6.375993e+06;
    const double r_E_lon      = /* 6378137.0 */ 6.389377e+06;
    const double C            = r_E_lon * cos(lat);
    const double one_over_C   = (fabs(C) > 1.0e-4) ? (1.0 / C) : 0.0;
    const double one_over_r_E = 1.0 / r_E_lat;
    const double delta_lat = sqrt(1000.0 / vegetation_density) / r_E_lat;
    const double delta_lon = sqrt(1000.0 / vegetation_density) / (r_E_lon * cos(loc.getLatitudeRad()));

    const osg::Matrix rotation_vertices_c = osg::Matrix::rotate(
     M_PI / 2 - clat, osg::Vec3d(0.0, 1.0, 0.0),
     clon, osg::Vec3d(0.0, 0.0, 1.0),
     0.0, osg::Vec3d(1.0, 0.0, 0.0));

    const osg::Matrix rotation_vertices_g = osg::Matrix::rotate(
     M_PI / 2 - lat, osg::Vec3d(0.0, 1.0, 0.0),
     lon, osg::Vec3d(0.0, 0.0, 1.0),
     0.0, osg::Vec3d(1.0, 0.0, 0.0));

    const osg::Vec3* vertexPtr = static_cast<const osg::Vec3*>(vertices->getDataPointer());
    const osg::Vec2* texPtr = static_cast<const osg::Vec2*>(texture_coords->getDataPointer());

    
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
        n /= n.length();

        const osg::Vec3d v_0_g = R_vert * v0;
        const osg::Vec3d v_1_g = R_vert * v1;
        const osg::Vec3d v_2_g = R_vert * v2;

        const osg::Vec2d ll_0 = osg::Vec2d(v_0_g.y() * one_over_C + lon, -v_0_g.x() * one_over_r_E + lat);
        const osg::Vec2d ll_1 = osg::Vec2d(v_1_g.y() * one_over_C + lon, -v_1_g.x() * one_over_r_E + lat);
        const osg::Vec2d ll_2 = osg::Vec2d(v_2_g.y() * one_over_C + lon, -v_2_g.x() * one_over_r_E + lat);

        const osg::Vec2d ll_O = ll_0;
        const osg::Vec2d ll_x = osg::Vec2d((v_1_g.y() - v_0_g.y()) * one_over_C, -(v_1_g.x() - v_0_g.x()) * one_over_r_E);
        const osg::Vec2d ll_y = osg::Vec2d((v_2_g.y() - v_0_g.y()) * one_over_C, -(v_2_g.x() - v_0_g.x()) * one_over_r_E);

        const int off_x = ll_O.x() / delta_lon;
        const int off_y = ll_O.y() / delta_lat;
        const int min_lon = min(min(ll_0.x(), ll_1.x()), ll_2.x()) / delta_lon;
        const int max_lon = max(max(ll_0.x(), ll_1.x()), ll_2.x()) / delta_lon;
        const int min_lat = min(min(ll_0.y(), ll_1.y()), ll_2.y()) / delta_lat;
        const int max_lat = max(max(ll_0.y(), ll_1.y()), ll_2.y()) / delta_lat;

        const osg::Vec2 t0 = texPtr[i0];
        const osg::Vec2 t1 = texPtr[i1];
        const osg::Vec2 t2 = texPtr[i2];

        const osg::Vec2d t_0 = t0;
        const osg::Vec2d t_x = t1 - t0;
        const osg::Vec2d t_y = t2 - t0;

        const double D = det2(ll_x, ll_y);

        for (int lat_int = min_lat - 1; lat_int <= max_lat + 1; lat_int++)
        {
            const double lat = (lat_int - off_y) * delta_lat;

            for (int lon_int = min_lon - 1; lon_int <= max_lon + 1; lon_int++)
            {
                const double lon = (lon_int - off_x) * delta_lon;
                const osg::Vec2d ro = getRandomOffset(lon_int, lat_int) * randomness;
                const osg::Vec2d p(lon + delta_lon * ro.x(), lat + delta_lat * ro.y());
                const double x = det2(ll_x, p) / D;
                const double y = det2(p, ll_y) / D;

                if ((x < 0.0) || (y < 0.0) || (x + y > 1.0))
                {
                    continue;
                }

                const osg::Vec2 tcp = t_x * x + t_y * y + t_0;
                const osg::Vec4 tc = image->getColor(tcp);
                const int tc_r = floor(tc.r() * 255.0 + 0.5);

                if ((tc_r < 22) || (24 < tc_r))
                {
                    continue;
                }

                const osg::Vec3 vp = v_x * x + v_y * y + v_0;
                bin->insert(SGVec3f(vp.x(), vp.y(), vp.z()), SGVec3f(n.x(), n.y(), n.z()));
            }

        }

    }

    SGTreeBinList randomForest;
    randomForest.push_back(bin);
    osg::Group* trees = createForest(randomForest, R_vert,_options, 1);
    buffer._transform->addChild(trees);
}

void VPBTechnique::applyLineFeatures(BufferData& buffer, Locator* masterLocator)
{
    unsigned int line_features_lod_range = 6;
    float minWidth = 9999.9;

    const unsigned int tileLevel = _terrainTile->getTileID().level;
    const SGPropertyNode* propertyNode = _options->getPropertyNode().get();

    if (propertyNode) {
        const SGPropertyNode* static_lod = propertyNode->getNode("/sim/rendering/static-lod");
        line_features_lod_range = static_lod->getIntValue("line-features-lod-level", line_features_lod_range);
        if (static_lod->getChildren("lod-level").size() > tileLevel) {
            minWidth = static_lod->getChildren("lod-level")[tileLevel]->getFloatValue("line-features-min-width", minWidth);
        }
    }

    if (tileLevel < line_features_lod_range) {
        // Do not generate vegetation for tiles too far away
        return;
    }

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Generating roads of width > " << minWidth << " for tile LoD level " << tileLevel);

    const SGMaterialLibPtr matlib  = _options->getMaterialLib();
    SGMaterial* mat = 0;
    const osg::Vec3d world = buffer._transform->getMatrix().getTrans();

    if (! matlib) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to get materials library to generate roads");
        return;
    }    

    // Get all appropriate roads.  We assume that the VPB terrain tile is smaller than a Bucket size.
    const SGGeod loc = SGGeod::fromCart(toSG(world));
    const SGBucket bucket = SGBucket(loc);
    auto roads = std::find_if(_lineFeatureLists.begin(), _lineFeatureLists.end(), [bucket](BucketLineFeatureBinList b){return (b.first == bucket);});

    if (roads == _lineFeatureLists.end()) return;

    SGMaterialCache* matcache = _options->getMaterialLib()->generateMatCache(loc, _options);

    for (; roads != _lineFeatureLists.end(); ++roads) {
        const LineFeatureBinList roadBins = roads->second;

        for (auto rb = roadBins.begin(); rb != roadBins.end(); ++rb)
        {
            mat = matcache->find(rb->getMaterial());

            if (!mat) {
                SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to find material " << rb->getMaterial() << " at " << loc << " " << bucket);
                continue;
            }    

            unsigned int xsize = mat->get_xsize();
            unsigned int ysize = mat->get_ysize();

            //  Generate a geometry for this set of roads.
            osg::Vec3Array* v = new osg::Vec3Array;
            osg::Vec2Array* t = new osg::Vec2Array;
            osg::Vec3Array* n = new osg::Vec3Array;
            osg::Vec4Array* c = new osg::Vec4Array;

            auto lineFeatures = rb->getLineFeatures();

            for (auto r = lineFeatures.begin(); r != lineFeatures.end(); ++r) {
                if (r->_width > minWidth) generateLineFeature(buffer, masterLocator, *r, world, v, t, n, xsize, ysize);
            }

            if (v->size() == 0) continue;

            c->push_back(osg::Vec4(1.0,1.0,1.0,1.0));

            osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
            geometry->setVertexArray(v);
            geometry->setTexCoordArray(0, t, osg::Array::BIND_PER_VERTEX);
            geometry->setTexCoordArray(1, t, osg::Array::BIND_PER_VERTEX);
            geometry->setNormalArray(n, osg::Array::BIND_PER_VERTEX);
            geometry->setColorArray(c, osg::Array::BIND_OVERALL);
            geometry->setUseDisplayList( false );
            geometry->setUseVertexBufferObjects( true );
            geometry->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLES, 0, v->size()) );

            EffectGeode* geode = new EffectGeode;
            geode->addDrawable(geometry);

            geode->setMaterial(mat);
            geode->setEffect(mat->get_one_effect(0));
            geode->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );
            buffer._transform->addChild(geode);
        }
    }
}

void VPBTechnique::generateLineFeature(BufferData& buffer, Locator* masterLocator, LineFeatureBin::LineFeature road, osg::Vec3d modelCenter, osg::Vec3Array* v, osg::Vec2Array* t, osg::Vec3Array* n, unsigned int xsize, unsigned int ysize)
{
    if (road._nodes.size() < 2) { 
        SG_LOG(SG_TERRAIN, SG_ALERT, "Coding error - LineFeatureBin::LineFeature with fewer than two nodes"); 
        return; 
    }

    osg::Vec3d ma, mb;
    std::list<osg::Vec3d> roadPoints;
    auto road_iter = road._nodes.begin();

    // We're in Earth-centered coordinates, so "up" is simply directly away from (0,0,0)
    osg::Vec3d up = modelCenter;
    up.normalize();

    ma = getMeshIntersection(buffer, masterLocator, *road_iter - modelCenter, up);
    road_iter++;

    for (; road_iter != road._nodes.end(); road_iter++) {
        mb = getMeshIntersection(buffer, masterLocator, *road_iter - modelCenter, up);
        auto esl = VPBElevationSlice::computeVPBElevationSlice(buffer._geometry, ma, mb, up);

        for(auto eslitr = esl.begin(); eslitr != esl.end(); ++eslitr) {
            roadPoints.push_back(*eslitr);
        }

        // Now traverse the next segment
        ma = mb;
    }

    if (roadPoints.size() == 0) return;

    // We now have a series of points following the topography of the elevation mesh.

    auto iter = roadPoints.begin();
    osg::Vec3d start = *iter;
    iter++;

    osg::Vec3d last_spanwise =  (*iter - start)^ up;
    last_spanwise.normalize();

    float yTexBaseA = 0.0f;
    float yTexBaseB = 0.0f;

    for (; iter != roadPoints.end(); iter++) {

        osg::Vec3d end = *iter;

        // Ignore tiny segments - likely artifacts of the elevation slicer
        if ((end - start).length2() < 1.0) continue;

        // Find a spanwise vector
        osg::Vec3d spanwise = ((end-start) ^ up);
        spanwise.normalize();

        // Define the road extents
        const osg::Vec3d a = start - last_spanwise * road._width * 0.5 + up;
        const osg::Vec3d b = start + last_spanwise * road._width * 0.5 + up;
        const osg::Vec3d c = end   - spanwise * road._width * 0.5 + up;
        const osg::Vec3d d = end   + spanwise * road._width * 0.5 + up;

        // Determine the x and y texture coordinates for the edges
        const float xTex = 0.5 * road._width / xsize;
        const float yTexA = yTexBaseA + (c-a).length() / ysize;
        const float yTexB = yTexBaseB + (d-b).length() / ysize;

        // Now generate two triangles, .
        v->push_back(a);
        v->push_back(b);
        v->push_back(c);

        t->push_back(osg::Vec2d(0, yTexBaseA));
        t->push_back(osg::Vec2d(xTex, yTexBaseB));
        t->push_back(osg::Vec2d(0, yTexA));

        v->push_back(b);
        v->push_back(d);
        v->push_back(c);

        t->push_back(osg::Vec2d(xTex, yTexBaseB));
        t->push_back(osg::Vec2d(xTex, yTexB));
        t->push_back(osg::Vec2d(0, yTexBaseA));

        // Normal is straight from the quad
        osg::Vec3d normal = (end-start)^spanwise;
        normal.normalize();
        for (unsigned int i = 0; i < 6; i++) n->push_back(normal);

        start = end;
        yTexBaseA = yTexA;
        yTexBaseB = yTexB;
        last_spanwise = spanwise;
    }
}

// Find the intersection of a given SGGeod with the terrain mesh
osg::Vec3d VPBTechnique::getMeshIntersection(BufferData& buffer, Locator* masterLocator, osg::Vec3d pt, osg::Vec3d up) 
{
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
    intersector = new osgUtil::LineSegmentIntersector(pt - up*100.0, pt + up*8000.0);
    osgUtil::IntersectionVisitor visitor(intersector.get());
    buffer._geometry->accept(visitor);

    if (intersector->containsIntersections()) {
        // We have an intersection with the terrain model, so return it
        return intersector->getFirstIntersection().getWorldIntersectPoint();
    } else {
        // No intersection.  Likely this point is outside our geometry.  So just return the original element.
        return pt;
    }
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
        _terrainTile->init(_terrainTile->getDirtyMask(), false);
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

// Simple vistor to check for any underlying terrain meshes that intersect with a given constraint therefore may need to be modified
// (e.g elevation lowered to ensure the terrain doesn't poke through an airport mesh, or line features generated)
class TerrainVisitor : public osg::NodeVisitor {
    public:
    osg::ref_ptr<osg::Node> _constraint; // Object describing the volume to be modified.   
    int _dirtyMask;                      // Dirty mask to apply.
    int _minLevel;                       // Minimum LoD level to modify.
    TerrainVisitor( osg::ref_ptr<osg::Node> node, int mask, int minLevel) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _constraint(node),
        _dirtyMask(mask),
        _minLevel(minLevel)
    { }
    virtual ~TerrainVisitor()
    { }

    void apply(osg::Node& node)
    {
        osgTerrain::TerrainTile* tile = dynamic_cast<osgTerrain::TerrainTile*>(&node);
        if (tile) {
            // Determine if the constraint should affect this tile.
            const int level = tile->getTileID().level;
            const osg::BoundingSphere tileBB = tile->getBound();
            if ((level >= _minLevel) && tileBB.intersects(_constraint->getBound())) {
                // Dirty any existing terrain tiles containing this constraint, which will force regeneration
                osgTerrain::TileID tileID = tile->getTileID();
                SG_LOG(SG_TERRAIN, SG_DEBUG, "Setting dirty mask for tile " << tileID.x << "," << tileID.y << " level " << tileID.level << " " << _dirtyMask);
                tile->setDirtyMask(_dirtyMask); 
            } else if (tileBB.intersects(_constraint->getBound())) {
                traverse(node);
            }
        } else {
            traverse(node);
        }
    }
};

// Add an osg object representing a contraint on the terrain mesh.  The generated terrain mesh will not include any vertices that
// lie above the constraint model.  (Note that geometry may result in edges intersecting the constraint model in cases where there
// are significantly higher vertices that lie just outside the constraint model.
void VPBTechnique::addElevationConstraint(osg::ref_ptr<osg::Node> constraint, osg::Group* terrain)
{ 
    const std::lock_guard<std::mutex> lock(VPBTechnique::_constraint_mutex); // Lock the _constraintGroup for this scope
    _constraintGroup->addChild(constraint.get()); 

    TerrainVisitor ftv(constraint, TerrainTile::ALL_DIRTY, 0);
    terrain->accept(ftv);
}

// Remove a previously added constraint.  E.g on model unload.
void VPBTechnique::removeElevationConstraint(osg::ref_ptr<osg::Node> constraint)
{ 
    const std::lock_guard<std::mutex> lock(VPBTechnique::_constraint_mutex); // Lock the _constraintGroup for this scope
    _constraintGroup->removeChild(constraint.get()); 
}

// Check a given vertex against any elevation constraints  E.g. to ensure the terrain mesh doesn't
// poke through any airport meshes.  If such a constraint exists, the function will return a replacement
// vertex displaces such that it lies 1m below the contraint relative to the passed in origin.  
osg::Vec3d VPBTechnique::checkAgainstElevationConstraints(osg::Vec3d origin, osg::Vec3d vertex, float vtx_gap)
{
    const std::lock_guard<std::mutex> lock(VPBTechnique::_constraint_mutex); // Lock the _constraintGroup for this scope
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
    intersector = new osgUtil::LineSegmentIntersector(origin, vertex);
    osgUtil::IntersectionVisitor visitor(intersector.get());
    _constraintGroup->accept(visitor);

    if (intersector->containsIntersections()) {
        // We have an intersection with our constraints model, so move the terrain vertex to 1m below the intersection point
        osg::Vec3d ray = intersector->getFirstIntersection().getWorldIntersectPoint() - origin;
        ray.normalize();
        return intersector->getFirstIntersection().getWorldIntersectPoint() - ray*vtx_gap;
    } else {
        return vertex;
    }
}

void VPBTechnique::addLineFeatureList(SGBucket bucket, LineFeatureBinList roadList, osg::ref_ptr<osg::Node> terrainNode)
{
    // Block to mutex the List alone
    {
        const std::lock_guard<std::mutex> lock(VPBTechnique::_lineFeatureLists_mutex); // Lock the _lineFeatureLists for this scope
        _lineFeatureLists.push_back(std::pair(bucket, roadList));
    }

    // We need to trigger a re-build of the appropriate Terrain tile, so create a pretend node and run the TerrainVisitor to
    // "dirty" the TerrainTile that it intersects with.
    osg::ref_ptr<osg::Node> n = new osg::Node();
    
    //SGVec3d coord1, coord2;
    //SGGeodesy::SGGeodToCart(SGGeod::fromDegM(bucket.get_center_lon() -0.5*bucket.get_width(), bucket.get_center_lat() -0.5*bucket.get_height(), 0.0), coord1);
    //SGGeodesy::SGGeodToCart(SGGeod::fromDegM(bucket.get_center_lon() +0.5*bucket.get_width(), bucket.get_center_lat() +0.5*bucket.get_height(), 0.0), coord2);
    //osg::BoundingBox bbox = osg::BoundingBox(toOsg(coord1), toOsg(coord2));
    //n->setInitialBound(bbox);

    SGVec3d coord;
    SGGeodesy::SGGeodToCart(SGGeod::fromDegM(bucket.get_center_lon(), bucket.get_center_lat(), 0.0), coord);
    n->setInitialBound(osg::BoundingSphere(toOsg(coord), max(bucket.get_width_m(), bucket.get_height_m())));

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Adding line features to " << bucket.gen_index_str());
    TerrainVisitor ftv(n, TerrainTile::ALL_DIRTY, 0);
    terrainNode->accept(ftv);
}

void VPBTechnique::unloadLineFeatures(SGBucket bucket)
{
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Erasing all roads with entry " << bucket);
    const std::lock_guard<std::mutex> lock(VPBTechnique::_lineFeatureLists_mutex); // Lock the _lineFeatureLists for this scope
    // C++ 20...
    //std::erase_if(_lineFeatureLists, [bucket](BucketLineFeatureBinList p) { return p.first == bucket; } );
}

 


