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

#include "CoastlineBin.hxx"

using namespace osg;

namespace simgear
{

CoastlineBin::CoastlineBin(const SGPath& absoluteFileName)
{
    if (!absoluteFileName.exists()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Coastline list file " << absoluteFileName << " does not exist.");
        return;
    }

    sg_gzifstream stream(absoluteFileName);
    if (!stream.is_open()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open " << absoluteFileName);
        return;
    }

    while (!stream.eof()) {
        // read a line.  Each line defines a coast line feature, consisting of a series of lon/lat positions.
        // Or a comment, starting with #
        std::string line;
        std::getline(stream, line);

        // strip comments
        std::string::size_type hash_pos = line.find('#');
        if (hash_pos != std::string::npos)
            line.resize(hash_pos);

        if (line.length() == 0) continue;

        // and process further
        std::stringstream in(line);

        if (in.bad() || in.fail()) {
            SG_LOG(SG_TERRAIN, SG_WARN, "Error parsing area entry in: " << absoluteFileName << " line: \"" << line << "\"");
            continue;
        }

        std::list<osg::Vec3d> nodes;

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

        if (nodes.size() > 1) {
            insert(Coastline(nodes));
        } else {
            SG_LOG(SG_TERRAIN, SG_WARN, "Coastline definition with fewer than two lon/lat nodes : " << absoluteFileName << " line: \"" << line << "\"");
        }
    }

    stream.close();
};


}
