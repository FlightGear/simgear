// OrthophotoManager.hxx -- manages satellite orthophotos
//
// Copyright (C) 2020  Nathaniel MacArthur-Warner nathanielwarner77@gmail.com
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA  02110-1301, USA.

#ifndef SG_SCENE_ORTHOPHOTO_MANAGER
#define SG_SCENE_ORTHOPHOTO_MANAGER

#include <unordered_map>

#include <osg/Referenced>
#include <osg/Image>
#include <osg/Texture2D>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGLimits.hxx>
#include "SGSceneFeatures.hxx"
#include "OsgSingleton.hxx"

namespace simgear {

    using ImageRef = osg::ref_ptr<osg::Image>;

    class Orthophoto;
    using OrthophotoRef = osg::ref_ptr<Orthophoto>;

    class OrthophotoBounds {
    private:
        double _minNegLon = SGLimitsd::max();
        double _minPosLon = SGLimitsd::max();
        double _maxNegLon = SGLimitsd::lowest();
        double _maxPosLon = SGLimitsd::lowest();
        double _minLat = SGLimitsd::max();
        double _maxLat = SGLimitsd::lowest();

        enum Hemisphere {Eastern, Western, StraddlingPm, StraddlingIdl, Invalid} _hemisphere = Invalid;
        void _updateHemisphere();

    public:
        static OrthophotoBounds fromBucket(const SGBucket& bucket);

        double getWidth() const;
        double getHeight() const;
        SGVec2f getTexCoord(const SGGeod& geod) const;
        double getLonOffset(const OrthophotoBounds& other) const;
        double getLatOffset(const OrthophotoBounds& other) const;

        void expandToInclude(const SGBucket& bucket);
        void expandToInclude(const double lon, const double lat);
        void expandToInclude(const OrthophotoBounds& bounds);
    };

    class Orthophoto : public osg::Referenced {
    private:
        ImageRef _image;
        OrthophotoBounds _bbox;

    public:
        static OrthophotoRef fromBucket(const SGBucket& bucket, const PathList& scenery_paths);

        Orthophoto(const ImageRef& image, const OrthophotoBounds& bbox) { _image = image; _bbox = bbox; }
        Orthophoto(const std::vector<OrthophotoRef>& orthophotos, const OrthophotoBounds& needed_bbox);

        osg::ref_ptr<osg::Texture2D> getTexture();
        OrthophotoBounds getBbox() const { return _bbox; };
    };

    class OrthophotoManager : public osg::Referenced {
    private:
        std::unordered_map<long, OrthophotoRef> _orthophotos;
    public:
        static OrthophotoManager* instance();

        void registerOrthophoto(const long bucket_idx, const OrthophotoRef& orthophoto);
        void unregisterOrthophoto(const long bucket_idx);

        /**
         * Get an orthophoto by bucket index
         **/
        OrthophotoRef getOrthophoto(const long bucket_idx);

        /**
         * Get an orthophoto given a set of nodes.
         * Used for airports, since they are not buckets.
         **/
        OrthophotoRef getOrthophoto(const std::vector<SGVec3d>& nodes, const SGVec3d& center);
    };
}

#endif