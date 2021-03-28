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

#include <osgTerrain/TerrainTechnique>
#include <osgTerrain/Locator>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/LineFeatureBin.hxx>

using namespace osgTerrain;

namespace simgear {

class VPBTechnique : public TerrainTechnique
{
    public:

        VPBTechnique();
        VPBTechnique(const SGReaderWriterOptions* options);

        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        VPBTechnique(const VPBTechnique&,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Object(osgTerrain, VPBTechnique);

        virtual void init(int dirtyMask, bool assumeMultiThreaded);

        virtual Locator* computeMasterLocator();


        virtual void update(osg::NodeVisitor& nv);

        virtual void cull(osg::NodeVisitor& nv);

        /** Traverse the terain subgraph.*/
        virtual void traverse(osg::NodeVisitor& nv);

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

        // Elevation constraints ensure that the terrain mesh is placed underneath objects such as airports
        static void addElevationConstraint(osg::ref_ptr<osg::Node> constraint, osg::Group* terrain);
        static void removeElevationConstraint(osg::ref_ptr<osg::Node> constraint);
        static osg::Vec3d checkAgainstElevationConstraints(osg::Vec3d origin, osg::Vec3d vertex, float vertex_gap);

        // LineFeatures are draped over the underlying mesh.
        static void addLineFeatureList(SGBucket bucket, LineFeatureBinList roadList, osg::ref_ptr<osg::Node> terrainNode);
        static void unloadLineFeatures(SGBucket bucket);

    protected:

        virtual ~VPBTechnique();

        class BufferData : public osg::Referenced
        {
        public:
            BufferData() {}

            osg::ref_ptr<osg::MatrixTransform>  _transform;
            osg::ref_ptr<EffectGeode>           _landGeode;
            osg::ref_ptr<EffectGeode>           _waterGeode;
            osg::ref_ptr<osg::Geometry>         _landGeometry;
            osg::ref_ptr<osg::Geometry>         _waterGeometry;
            float                               _width;
            float                               _height;

        protected:
            ~BufferData() {}
        };

        virtual osg::Vec3d computeCenter(BufferData& buffer, Locator* masterLocator);
        virtual osg::Vec3d computeCenterModel(BufferData& buffer, Locator* masterLocator);
        const virtual SGGeod computeCenterGeod(BufferData& buffer, Locator* masterLocator);

        virtual void generateGeometry(BufferData& buffer, Locator* masterLocator, const osg::Vec3d& centerModel);

        virtual void applyColorLayers(BufferData& buffer, Locator* masterLocator);

        virtual double det2(const osg::Vec2d a, const osg::Vec2d b);

        static osg::Vec2d getRandomOffset();

        virtual void applyTrees(BufferData& buffer, Locator* masterLocator);

        virtual void applyLineFeatures(BufferData& buffer, Locator* masterLocator);
        virtual void generateLineFeature(BufferData& buffer, Locator* 
            masterLocator, LineFeatureBin::LineFeature road, 
            osg::Vec3d modelCenter, 
            osg::Vec3Array* v, 
            osg::Vec2Array* t, 
            osg::Vec3Array* n,
            unsigned int xsize,
            unsigned int ysize);
        virtual osg::Vec3d getMeshIntersection(BufferData& buffer, Locator* masterLocator, osg::Vec3d pt, osg::Vec3d up);

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

        inline static osg::ref_ptr<osg::Group>  _constraintGroup = new osg::Group();;
        inline static std::mutex _constraint_mutex;  // protects the _constraintGroup;
        static osg::Vec2d*                  _randomOffsets;

        typedef std::pair<SGBucket, LineFeatureBinList> BucketLineFeatureBinList;

        inline static std::list<BucketLineFeatureBinList>  _lineFeatureLists;
        inline static std::mutex _lineFeatureLists_mutex;  // protects the _roadLists;

};

}

#endif
