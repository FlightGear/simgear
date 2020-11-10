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

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>

#include <osgTerrain/TerrainTechnique>
#include <osgTerrain/Locator>

#include <simgear/scene/material/EffectGeode.hxx>

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


    protected:

        virtual ~VPBTechnique();

        class BufferData : public osg::Referenced
        {
        public:
            BufferData() {}

            osg::ref_ptr<osg::MatrixTransform>  _transform;
            osg::ref_ptr<EffectGeode>           _geode;
            osg::ref_ptr<osg::Geometry>         _geometry;

        protected:
            ~BufferData() {}
        };

        virtual osg::Vec3d computeCenter(BufferData& buffer, Locator* masterLocator);
        virtual osg::Vec3d computeCenterModel(BufferData& buffer, Locator* masterLocator);

        virtual void generateGeometry(BufferData& buffer, Locator* masterLocator, const osg::Vec3d& centerModel);

        virtual void applyColorLayers(BufferData& buffer);

        virtual void applyTransparency(BufferData& buffer);


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
};

}

#endif
