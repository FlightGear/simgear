// OrthophotoManager.cxx -- manages satellite orthophotos
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

#include "OrthophotoManager.hxx"

namespace simgear {
    
    OrthophotoBounds OrthophotoBounds::fromBucket(const SGBucket& bucket) {
        OrthophotoBounds bounds;
        bounds.expandToInclude(bucket);
        return bounds;
    }

    void OrthophotoBounds::_updateHemisphere() {
        if (_minPosLon <= 180 && _maxPosLon >= 0 && _minNegLon < 0 && _maxNegLon >= -180) {
            // We have negative and positive longitudes.
            // Choose whether we're straddling the Prime Meridian or 180th meridian
            if (_maxPosLon - _minNegLon < 180) {
                _hemisphere = StraddlingPm;
            } else {
                _hemisphere = StraddlingIdl;
            }
        }
        else if (_minPosLon <= 180.0 && _maxPosLon >= 0.0) {
            _hemisphere = Eastern;
        }
        else if (_minNegLon < 0.0 && _maxNegLon >= -180.0) {
            _hemisphere = Western;
        }
        else {
            _hemisphere = Invalid;
        }
    }

    double OrthophotoBounds::getWidth() const {
        switch (_hemisphere) {
            case Eastern:
                return _maxPosLon - _minPosLon;
            case Western:
                return _maxNegLon - _minNegLon;
            case StraddlingPm:
                return _maxPosLon - _minNegLon;
            case StraddlingIdl:
                return (180.0 - _minPosLon) + (_maxNegLon + 180.0);
            default:
                SG_LOG(SG_TERRAIN, SG_ALERT, "OrthophotoBounds::getWidth: My data is invalid. Returning 0.");
                return 0.0;
        }
    }

    double OrthophotoBounds::getHeight() const {
        return _maxLat - _minLat;
    }

    SGVec2f OrthophotoBounds::getTexCoord(const SGGeod& geod) const {
        
        const double lon = geod.getLongitudeDeg();
        const double width = getWidth();
        float x = 0.0;

        switch (_hemisphere) {
            case Eastern:
                x = (lon - _minPosLon) / width;
                break;
            case Western:
                x = (lon - _minNegLon) / width;
                break;
            case StraddlingPm:
                x = (lon - _minNegLon) / width;
                break;
            case StraddlingIdl:
                if (lon >= 0) {
                    // Geod is in the eastern hemisphere
                    x = (lon - _minPosLon) / width;
                } else {
                    // Geod is in the western hemisphere
                    x = (180.0 - _minPosLon + lon) / width;
                }
                break;
            default:
                SG_LOG(SG_TERRAIN, SG_ALERT, "OrthophotoBounds::getTexCoord: My data is invalid.");
                break;
        }

        const float y = (_maxLat - geod.getLatitudeDeg()) / getHeight();

        return SGVec2f(x, y);
    }

    double OrthophotoBounds::getLonOffset(const OrthophotoBounds& other) const {
        
        std::string error_message = "";

        switch (_hemisphere) {
            case Eastern:
                if (other._hemisphere == Eastern)
                    return other._minPosLon - _minPosLon; 
                else
                    error_message = "I'm not in the same hemisphere as other.";
                break;
            case Western:
                if (other._hemisphere == Western)
                    return other._minNegLon - _minNegLon;
                else
                    error_message = "I'm not in the same hemisphere as other.";
                break;
            case StraddlingPm:
                if (other._hemisphere == Western || other._hemisphere == StraddlingPm)
                    return other._minNegLon - _minNegLon;
                else if (other._hemisphere == Eastern)
                    return -_minNegLon + other._minPosLon;
                else
                    error_message = "I'm not in the same hemisphere as other";
                break;
            case StraddlingIdl:
                if (other._hemisphere == Eastern || other._hemisphere == StraddlingIdl) {
                    return other._minPosLon - _minPosLon;
                } else if (other._hemisphere == StraddlingIdl) {
                    return (180.0 - _minPosLon) + (other._minNegLon + 180.0);
                } else {
                    error_message = "Other has invalid data.";
                }
                break;
            default:
                error_message = "My data is invalid.";
                break;
        }

        SG_LOG(SG_TERRAIN, SG_ALERT, "OrthophotoBounds::getLonOffset: " << error_message << " Returning 0.");
        return 0.0;
    }

    double OrthophotoBounds::getLatOffset(const OrthophotoBounds& other) const {
        return _maxLat - other._maxLat;
    }

    void OrthophotoBounds::expandToInclude(const SGBucket& bucket) {
        double center_lon = bucket.get_center_lon();
        double center_lat = bucket.get_center_lat();
        double width = bucket.get_width();
        double height = bucket.get_height();

        double left = center_lon - width / 2;
        double right = center_lon + width / 2;
        double bottom = center_lat - height / 2;
        double top = center_lat + height / 2;

        expandToInclude(left, bottom);
        expandToInclude(right, top);
    }
    
    void OrthophotoBounds::expandToInclude(const double lon, const double lat) {
        if (lon >= 0) {
            if (lon < _minPosLon)
                _minPosLon = lon;
            if (lon > _maxPosLon)
                _maxPosLon = lon;
        } else {
            if (lon < _minNegLon)
                _minNegLon = lon;
            if (lon > _maxNegLon)
                _maxNegLon = lon;
        }

        if (lat < _minLat)
            _minLat = lat;
        if (lat > _maxLat)
            _maxLat = lat;

        _updateHemisphere();
    }

    void OrthophotoBounds::expandToInclude(const OrthophotoBounds& bounds) {
        switch (bounds._hemisphere) {
            case Eastern:
                expandToInclude(bounds._minPosLon, bounds._minLat);
                expandToInclude(bounds._maxPosLon, bounds._maxLat);
                break;
            case Western:
                expandToInclude(bounds._minNegLon, bounds._minLat);
                expandToInclude(bounds._maxNegLon, bounds._maxLat);
                break;
            case StraddlingPm:
                expandToInclude(bounds._minNegLon, bounds._minLat);
                expandToInclude(bounds._maxPosLon, bounds._maxLat);
            case StraddlingIdl:
                expandToInclude(bounds._minPosLon, bounds._minLat);
                expandToInclude(bounds._maxNegLon, bounds._maxLat);
                break;
            case Invalid:
                SG_LOG(SG_TERRAIN, SG_ALERT, "OrthophotoBounds::absorb: Data in bounds to absorb is invalid. Aborting.");
                break;
        }
    }

    OrthophotoRef Orthophoto::fromBucket(const SGBucket& bucket, const PathList& scenery_paths) {

        const std::string bucket_path = bucket.gen_base_path();

        for (const auto& scenery_path : scenery_paths) {
            SGPath path = scenery_path / "Orthophotos" / bucket_path / std::to_string(bucket.gen_index());

            path.concat(".png");
            if (path.exists()) {
                ImageRef image = osgDB::readRefImageFile(path.str());
                image->flipVertical();
                OrthophotoBounds bbox = OrthophotoBounds::fromBucket(bucket);
                return new Orthophoto(image, bbox);
            }
        }

        return nullptr;
    }

    // Handy image cropping function from osgEarth
    osg::Image* cropImage(const osg::Image* image,
                          double src_minx, double src_miny, double src_maxx, double src_maxy,
                          double &dst_minx, double &dst_miny, double &dst_maxx, double &dst_maxy) {
        if ( image == 0L )
            return 0L;

        //Compute the desired cropping rectangle
        int windowX        = osg::clampBetween( (int)floor( (dst_minx - src_minx) / (src_maxx - src_minx) * (double)image->s()), 0, image->s()-1);
        int windowY        = osg::clampBetween( (int)floor( (dst_miny - src_miny) / (src_maxy - src_miny) * (double)image->t()), 0, image->t()-1);
        int windowWidth    = osg::clampBetween( (int)ceil(  (dst_maxx - src_minx) / (src_maxx - src_minx) * (double)image->s()) - windowX, 0, image->s());
        int windowHeight   = osg::clampBetween( (int)ceil(  (dst_maxy - src_miny) / (src_maxy - src_miny) * (double)image->t()) - windowY, 0, image->t());

        if (windowX + windowWidth > image->s())
        {
            windowWidth = image->s() - windowX;
        }

        if (windowY + windowHeight > image->t())
        {
            windowHeight = image->t() - windowY;
        }

        if ((windowWidth * windowHeight) == 0)
        {
            return NULL;
        }

        //Compute the actual bounds of the area we are computing
        double res_s = (src_maxx - src_minx) / (double)image->s();
        double res_t = (src_maxy - src_miny) / (double)image->t();

        dst_minx = src_minx + (double)windowX * res_s;
        dst_miny = src_miny + (double)windowY * res_t;
        dst_maxx = dst_minx + (double)windowWidth * res_s;
        dst_maxy = dst_miny + (double)windowHeight * res_t;

        //OE_NOTICE << "Copying from " << windowX << ", " << windowY << ", " << windowWidth << ", " << windowHeight << std::endl;

        //Allocate the croppped image
        osg::Image* cropped = new osg::Image;
        cropped->allocateImage(windowWidth, windowHeight, image->r(), image->getPixelFormat(), image->getDataType());
        cropped->setInternalTextureFormat( image->getInternalTextureFormat() );

        for (int layer=0; layer<image->r(); ++layer)
        {
            for (int src_row = windowY, dst_row=0; dst_row < windowHeight; src_row++, dst_row++)
            {
                if (src_row > image->t()-1) SG_LOG(SG_OSG, SG_INFO, "HeightBroke");
                const void* src_data = image->data(windowX, src_row, layer);
                void* dst_data = cropped->data(0, dst_row, layer);
                memcpy( dst_data, src_data, cropped->getRowSizeInBytes());
            }
        }
        return cropped;
    }

    Orthophoto::Orthophoto(const std::vector<OrthophotoRef>& orthophotos, const OrthophotoBounds& needed_bbox) {
        
        OrthophotoBounds prelim_bbox;
        for (const auto& orthophoto : orthophotos) {
            prelim_bbox.expandToInclude(orthophoto->getBbox());
        }

        const OrthophotoRef& some_orthophoto = orthophotos[0];
        const ImageRef& some_image = some_orthophoto->_image;
        const OrthophotoBounds& some_bbox = some_orthophoto->getBbox();
        const double degs_to_pixels = some_image->s() / some_bbox.getWidth();
        
        const int total_width = degs_to_pixels * prelim_bbox.getWidth();
        const int total_height = degs_to_pixels * prelim_bbox.getHeight();

        const int depth = some_image->r();
        GLenum pixel_format = some_image->getPixelFormat();
        GLenum data_type = some_image->getDataType();
        int packing = some_image->getPacking();

        ImageRef prelim_image = new osg::Image();
        prelim_image->allocateImage(total_width, total_height, depth, pixel_format, data_type, packing);

        for (const auto& orthophoto : orthophotos) {
            
            const OrthophotoBounds& bounds = orthophoto->getBbox();
            const int width = degs_to_pixels * bounds.getWidth();
            const int height = degs_to_pixels * bounds.getHeight();
            const int s_offset = degs_to_pixels * prelim_bbox.getLonOffset(bounds);
            const int t_offset = degs_to_pixels * prelim_bbox.getLatOffset(bounds);

            // Make a deep copy of the orthophoto's image so that we don't modify the original when scaling
            ImageRef sub_image = new osg::Image(*orthophoto->_image, osg::CopyOp::DEEP_COPY_ALL);

            sub_image->scaleImage(width, height, depth);
            
            prelim_image->copySubImage(s_offset, t_offset, 0, sub_image);
        }

        double x_min = prelim_bbox.getLonOffset(needed_bbox) * degs_to_pixels;
        double y_min = prelim_bbox.getLatOffset(needed_bbox) * degs_to_pixels;
        double x_max = x_min + needed_bbox.getWidth() * degs_to_pixels;
        double y_max = y_min + needed_bbox.getHeight() * degs_to_pixels;

        _image = cropImage(prelim_image, 0.0, 0.0, total_width, total_height, x_min, y_min, x_max, y_max);
        _bbox = needed_bbox; // Theoretically, it would be good to adjust from how the bounds have changed from cropping
    }

    osg::ref_ptr<osg::Texture2D> Orthophoto::getTexture() {
        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(_image);
        texture->setWrap(osg::Texture::WrapParameter::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WrapParameter::WRAP_T, osg::Texture::WrapMode::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WrapParameter::WRAP_R, osg::Texture::WrapMode::CLAMP_TO_EDGE);
        texture->setMaxAnisotropy(SGSceneFeatures::instance()->getTextureFilter());
        return texture;
    }

    OrthophotoManager* OrthophotoManager::instance() {
        return SingletonRefPtr<OrthophotoManager>::instance();
    }

    void OrthophotoManager::registerOrthophoto(const long bucket_idx, const OrthophotoRef& orthophoto) {
        OrthophotoRef& entry = _orthophotos[bucket_idx];

        if (entry) {
            SG_LOG(SG_TERRAIN, SG_WARN, "OrthophotoManager::registerOrthophoto(): Bucket index " << bucket_idx << " already has a registered orthophoto.");
        }

        if (!orthophoto) {
            SG_LOG(SG_TERRAIN, SG_WARN, "OrthophotoManager::registerOrthophoto(): Registering null orthophoto for bucket index " << bucket_idx);
        }

        entry = orthophoto;

        SG_LOG(SG_TERRAIN, SG_INFO, "Registered orthophoto for bucket index " << bucket_idx);
    }

    void OrthophotoManager::unregisterOrthophoto(const long bucket_idx) {
        if (_orthophotos[bucket_idx]) {
            _orthophotos.erase(bucket_idx);
            SG_LOG(SG_TERRAIN, SG_INFO, "Unregistered orthophoto with bucket index " << bucket_idx);
        } else {
            SG_LOG(SG_TERRAIN, SG_WARN, "OrthophotoManager::unregisterOrthophoto(): Attempted to unregister orthophoto with bucket index that is not currently registered.");
        }
    }

    OrthophotoRef OrthophotoManager::getOrthophoto(const long bucket_idx) {
        if (_orthophotos[bucket_idx]) {
            return _orthophotos[bucket_idx];
        } else {
            return nullptr;
        }
    }

    OrthophotoRef OrthophotoManager::getOrthophoto(const std::vector<SGVec3d>& nodes, const SGVec3d& center) {
        
        std::unordered_map<long, bool> orthophotos_attempted;
        std::vector<OrthophotoRef> orthophotos;
        OrthophotoBounds needed_bounds;

        for (const auto& node : nodes) {
            const SGGeod node_geod = SGGeod::fromCart(node + center);
            needed_bounds.expandToInclude(node_geod.getLongitudeDeg(), node_geod.getLatitudeDeg());
            const SGBucket bucket(node_geod);
            const long bucket_idx = bucket.gen_index();
            bool& orthophoto_attempted = orthophotos_attempted[bucket_idx];
            if (!orthophoto_attempted) {
                OrthophotoRef orthophoto = this->getOrthophoto(bucket_idx);
                if (orthophoto) {
                    orthophotos.push_back(orthophoto);
                }
                orthophoto_attempted = true;
            }
        }

        if (orthophotos.size() == 0) {
            return nullptr;
        }

        return new Orthophoto(orthophotos, needed_bounds);
    }
}
