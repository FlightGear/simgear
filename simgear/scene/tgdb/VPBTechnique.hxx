// VPBTechnique.hxx -- VirtualPlanetBuilder Effects technique
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

#ifndef VPBTECHNIQUE
#define VPBTECHNIQUE 1

#include <mutex>

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgTerrain/TerrainTile>
#include <osgTerrain/Terrain>

#include <osgTerrain/TerrainTechnique>
#include <osgTerrain/Locator>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/bvh/BVHMaterial.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/AreaFeatureBin.hxx>
#include <simgear/scene/tgdb/LightBin.hxx>
#include <simgear/scene/tgdb/LineFeatureBin.hxx>
#include <simgear/scene/tgdb/CoastlineBin.hxx>
#include <simgear/scene/tgdb/VPBBufferData.hxx>

using namespace osgTerrain;

namespace simgear {

class VPBTechnique : public TerrainTechnique
{
    public:

        VPBTechnique();
        VPBTechnique(const SGReaderWriterOptions* options, const std::string fileName);

        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        VPBTechnique(const VPBTechnique&,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Object(osgTerrain, VPBTechnique);

        virtual void init(int dirtyMask, bool assumeMultiThreaded);

        virtual Locator* computeMasterLocator();


        virtual void update(osg::NodeVisitor& nv);

        virtual void cull(osg::NodeVisitor& nv);

        /** Traverse the terain subgraph.*/
        virtual void traverse(osg::NodeVisitor& nv);

        virtual BVHMaterial* getMaterial(osg::Vec3d point);
        virtual SGSphered computeBoundingSphere() const;

        virtual void cleanSceneGraph();

        void setFilterBias(float filterBias);
        float getFilterBias() const { return _filterBias; }

        void setFilterWidth(float filterWidth);
        float getFilterWidth() const { return _filterWidth; }

        void setFilterMatrix(const osg::Matrix3& matrix);
        osg::Matrix3& getFilterMatrix() { return _filterMatrix; }
        const osg::Matrix3& getFilterMatrix() const { return _filterMatrix; }

        enum FilterType
        {
            GAUSSIAN,
            SMOOTH,
            SHARPEN
        };

        void setFilterMatrixAs(FilterType filterType);

        void setOptions(const SGReaderWriterOptions* options);

        /** If State is non-zero, this function releases any associated OpenGL objects for
        * the specified graphics context. Otherwise, releases OpenGL objects
        * for all graphics contexts. */
        virtual void releaseGLObjects(osg::State* = 0) const;

        // Elevation constraints ensure that the terrain mesh is placed underneath objects such as airports.
        // As airports are generated in a separate loading thread, these are static.
        static void addElevationConstraint(osg::ref_ptr<osg::Node> constraint);
        static void removeElevationConstraint(osg::ref_ptr<osg::Node> constraint);
        static double getConstrainedElevation(osg::Vec3d ndc, Locator* masterLocator, double vtx_gap);
        static bool checkAgainstElevationConstraints(osg::Vec3d origin, osg::Vec3d vertex);

        static void clearConstraints();

        inline static const char* Z_UP_TRANSFORM = "fg_zUpTransform";
        inline static const char* MODEL_OFFSET   = "fg_modelOffset";
        inline static const char* PHOTO_SCENERY  = "fg_photoScenery";

    protected:

        virtual ~VPBTechnique();

        class VertexNormalGenerator
        {
        public:

            typedef std::vector<int> Indices;
            typedef std::pair< osg::ref_ptr<osg::Vec2Array>, Locator* > TexCoordLocatorPair;
            typedef std::map< Layer*, TexCoordLocatorPair > LayerToTexCoordMap;

            VertexNormalGenerator(Locator* masterLocator, const osg::Vec3d& centerModel, int numRows, int numColmns, float scaleHeight, float vtx_gap, bool createSkirt);

            void populateCenter(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas, osg::Vec2Array* texcoords);
            void populateLeftBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas);
            void populateRightBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas);
            void populateAboveBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas);
            void populateBelowBoundary(osgTerrain::Layer* elevationLayer, osgTerrain::Layer* colorLayer, osg::ref_ptr<Atlas> atlas);

            void computeNormals();

            unsigned int capacity() const { return _vertices->capacity(); }

            inline void setVertex(int c, int r, const osg::Vec3& v, const osg::Vec3& n)
            {
                int& i = index(c,r);
                if (i==0) {
                    if (r<0 || r>=_numRows || c<0 || c>=_numColumns) {
                        i = -(1+static_cast<int>(_boundaryVertices->size()));
                        _boundaryVertices->push_back(v);
                    } else {
                        i = _vertices->size() + 1;
                        _vertices->push_back(v);
                        _normals->push_back(n);
                    }
                } else if (i<0) {
                    (*_boundaryVertices)[-i-1] = v;
                } else {
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
            std::vector<float>                _elevationConstraints;

            osg::ref_ptr<osg::Vec3Array>    _boundaryVertices;
        };


        virtual osg::Vec3d computeCenter(BufferData& buffer);
        virtual osg::Vec3d computeCenterModel(BufferData& buffer);
        const virtual SGGeod computeCenterGeod(BufferData& buffer);

        virtual void generateGeometry(BufferData& buffer, const osg::Vec3d& centerModel, osg::ref_ptr<SGMaterialCache> matcache);

        virtual void applyColorLayers(BufferData& buffer, osg::ref_ptr<SGMaterialCache> matcache);

        virtual double det2(const osg::Vec2d a, const osg::Vec2d b);

        virtual void applyMaterials(BufferData& buffer, osg::ref_ptr<SGMaterialCache> matcache);

        virtual osg::Image* generateWaterTexture(Atlas* atlas);

        static void updateStats(int tileLevel, float loadTime);

        // Check a given vertex against any constraints  E.g. to ensure we
        // don't get objects like trees sprouting from roads or runways.
        bool checkAgainstRandomObjectsConstraints(BufferData& buffer, 
                                                  osg::Vec3d origin, osg::Vec3d vertex);


        bool checkAgainstWaterConstraints(BufferData& buffer, osg::Vec2d point);

        OpenThreads::Mutex                  _writeBufferMutex;
        osg::ref_ptr<BufferData>            _currentBufferData;
        osg::ref_ptr<BufferData>            _newBufferData;

        float                               _filterBias;
        osg::ref_ptr<osg::Uniform>          _filterBiasUniform;
        float                               _filterWidth;
        osg::ref_ptr<osg::Uniform>          _filterWidthUniform;
        osg::Matrix3                        _filterMatrix;
        osg::ref_ptr<osg::Uniform>          _filterMatrixUniform;
        osg::ref_ptr<SGReaderWriterOptions> _options;
        const std::string _fileName;
        osg::ref_ptr<osg::Group>            _randomObjectsConstraintGroup;

        inline static osg::ref_ptr<osg::Group>  _elevationConstraintGroup = new osg::Group();
        inline static std::mutex _elevationConstraintMutex;  // protects the _elevationConstraintGroup;

        inline static std::mutex _stats_mutex; // Protects the loading statistics
        typedef std::pair<unsigned int, float> LoadStat;
        inline static std::map<int, LoadStat> _loadStats;
        inline static SGPropertyNode* _statsPropertyNode;

};

};

#endif
