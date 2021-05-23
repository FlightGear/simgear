/* -*-c++-*-
 *
 * Copyright (C) 2008 Stuart Buchanan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include "AreaFeatureBin.hxx"

using namespace osg;

namespace simgear
{

AreaFeatureBin::AreaFeatureBin(const SGPath& absoluteFileName, const std::string material) :
  _material(material)
{
    if (!absoluteFileName.exists()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "AreaFeature list file " << absoluteFileName << " does not exist.");
        return;
    }

    sg_gzifstream stream(absoluteFileName);
    if (!stream.is_open()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open " << absoluteFileName);
        return;
    }

    while (!stream.eof()) {
        // read a line.  Each line defines a single line feature, consisting of a width followed by a series of lon/lat positions.
        // Or a comment, starting with #
        std::string line;
        std::getline(stream, line);

        if (line.length() == 0) continue;

        // strip comments
        std::string::size_type hash_pos = line.find('#');
        if (hash_pos != std::string::npos)
            line.resize(hash_pos);

        // and process further
        std::stringstream in(line);

        // Area format is Area A B C D lon0 lat0 lon1 lat1 lon2 lat2 lon3 lat4....
        // where:
        // Area is the area of the feature in m^2
        // A, B, C, D are generic attributes.  Their interpretation may vary by feature type
        // lon[n], lat[n] are pairs of lon/lat defining straight road segments
        int attributes = 0;
        float area=0, a=0, b=0, c=0, d=0;
        std::list<osg::Vec3d> nodes;

        in >> area >> attributes >> a >> b >> c >> d;

        if (in.bad() || in.fail()) {
            SG_LOG(SG_TERRAIN, SG_WARN, "Error parsing area entry in: " << absoluteFileName << " line: \"" << line << "\"");
            continue;
        }

        while (true) {
            double lon = 0.0f, lat=0.0f;
            in >> lon >> lat;

            if (in.bad() || in.fail()) {
                break;
            }

            SGVec3d tmp;
            SGGeodesy::SGGeodToCart(SGGeod::fromDeg(lon, lat), tmp);
            nodes.push_back(toOsg(tmp));
        }

        if (nodes.size() > 2) {
            insert(AreaFeature(nodes, area, attributes, a, b, c, d));
        } else {
            SG_LOG(SG_TERRAIN, SG_WARN, "AreaFeature definition with fewer than three lon/lat nodes : " << absoluteFileName << " line: \"" << line << "\"");
        }
    }

    stream.close();
};


}
