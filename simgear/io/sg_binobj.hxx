/**
 * \file sg_binobj.hxx
 * Routines to read and write the low level (binary) simgear 3d object format.
 */

// Written by Curtis Olson, started January 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_BINOBJ_HXX
#define _SG_BINOBJ_HXX

#include <zlib.h> // for gzFile

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/math/SGMath.hxx>

#include <vector>
#include <string>
#include <boost/array.hpp>

#define MAX_TC_SETS     (4)
#define MAX_VAS         (8)

// I really want to pass around fixed length arrays, as the size 
// has to be hardcoded
// but it's a C++0x feature use boost in its absence
typedef boost::array<int_list, MAX_TC_SETS> tci_list;
typedef boost::array<int_list, MAX_VAS>     vai_list;

/** STL Structure used to store (integer index) object information */
typedef std::vector < int_list > group_list;
typedef group_list::iterator group_list_iterator;
typedef group_list::const_iterator const_group_list_iterator;

/** STL Structure used to store (tc index) object information */
typedef std::vector < tci_list > group_tci_list;
typedef group_tci_list::iterator group_tci_list_iterator;
typedef group_tci_list::const_iterator const_group_tci_list_iterator;

/** STL Structure used to store (va index) object information */
typedef std::vector < vai_list > group_vai_list;
typedef group_vai_list::iterator group_vai_list_iterator;
typedef group_vai_list::const_iterator const_group_vai_list_iterator;


// forward decls
class SGBucket;
class SGPath;

class SGBinObjectPoint {
public:
    std::string material;
    int_list    v_list;
    int_list    n_list;
    int_list    c_list;
    
    void clear( void ) {
        material = "";
        v_list.clear();
        n_list.clear();
        c_list.clear();
    };    
};

class SGBinObjectTriangle {
public:
    std::string material;
    int_list    v_list;
    int_list    n_list;
    int_list    c_list;
    
    tci_list    tc_list;
    vai_list    va_list;

    void clear( void ) {
        material = "";
        v_list.clear();
        n_list.clear();
        c_list.clear();
        for ( unsigned int i=0; i<MAX_TC_SETS; i++ ) {
            tc_list[i].clear();
        }
        for ( unsigned int i=0; i<MAX_VAS; i++ ) {
            va_list[i].clear();
        }
    };
    
};



/**
 * A class to manipulate the simgear 3d object format.
 * This class provides functionality to both read and write the binary format.
 *
 * Here is a really quick overview of the file syntax:
 *
 * - scenery-file: magic, nobjects, object+
 *
 * - magic: "TG" + version
 *
 * - object: obj_typecode, nproperties, nelements, property+, element+
 *
 * - element: nbytes, BYTE+
 *
 * - property: prop_typecode, nbytes, BYTE+
 *
 * - obj_typecode: bounding sphere | vertices | normals | texcoords |
 *                 points | triangles | fans | strips 
 *
 * - prop_typecode: material_name | ???
 *
 * - nelements: USHORT (Gives us 65536 which ought to be enough, right?)
 *
 * - nproperties: USHORT
 *
 * - *_typecode: CHAR
 *
 * - nbytes: INTEGER (If we used short here that would mean 65536 bytes = 16384
 *                    floats = 5461 vertices which is not enough for future
 *	              growth)
 *
 * - vertex: FLOAT, FLOAT, FLOAT
*/
class SGBinObject {
private:
    unsigned short version;

    SGVec3d gbs_center;
    float gbs_radius;

    std::vector<SGVec3d> wgs84_nodes;   // vertex list
    std::vector<SGVec4f> colors;        // color list
    std::vector<SGVec3f> normals;       // normal list
    std::vector<SGVec2f> texcoords;     // texture coordinate list
    std::vector<float>   va_flt;        // vertex attribute list (floats)
    std::vector<int>     va_int;        // vertex attribute list (ints) 
    
    group_list pts_v;               	// points vertex index
    group_list pts_n;               	// points normal index
    group_list pts_c;               	// points color index
    group_tci_list pts_tcs;             // points texture coordinates ( up to 4 sets )
    group_vai_list pts_vas;             // points vertex attributes ( up to 8 sets )
    string_list pt_materials;           // points materials

    group_list tris_v;              	// triangles vertex index
    group_list tris_n;              	// triangles normal index
    group_list tris_c;              	// triangles color index
    group_tci_list tris_tcs;            // triangles texture coordinates ( up to 4 sets )
    group_vai_list tris_vas;            // triangles vertex attributes ( up to 8 sets )
    string_list tri_materials;          // triangles materials

    group_list strips_v;            	// tristrips vertex index
    group_list strips_n;            	// tristrips normal index
    group_list strips_c;            	// tristrips color index
    group_tci_list strips_tcs;          // tristrips texture coordinates ( up to 4 sets )
    group_vai_list strips_vas;          // tristrips vertex attributes ( up to 8 sets )
    string_list strip_materials;        // tristrips materials

    group_list fans_v;              	// fans vertex index
    group_list fans_n;              	// fans normal index
    group_list fans_c;              	// fans color index
    group_tci_list fans_tcs;            // fanss texture coordinates ( up to 4 sets )
    group_vai_list fans_vas;            // fans vertex attributes ( up to 8 sets )
    string_list fan_materials;	        // fans materials

    void read_properties(gzFile fp, int nproperties);
    
    void read_object( gzFile fp,
                             int obj_type,
                             int nproperties,
                             int nelements,
                             group_list& vertices, 
                             group_list& normals,
                             group_list& colors,
                             group_tci_list& texCoords,
                             group_vai_list& vertexAttribs,
                             string_list& materials);
                             
    void write_header(gzFile fp, int type, int nProps, int nElements);
    void write_objects(gzFile fp, 
                       int type, 
                       const group_list& verts,
                       const group_list& normals, 
                       const group_list& colors, 
                       const group_tci_list& texCoords, 
                       const group_vai_list& vertexAttribs,
                       const string_list& materials);
        
    unsigned int count_objects(const string_list& materials);
    
public:    
    inline unsigned short get_version() const { return version; }

    inline const SGVec3d& get_gbs_center() const { return gbs_center; }
    inline void set_gbs_center( const SGVec3d& p ) { gbs_center = p; }

    inline float get_gbs_radius() const { return gbs_radius; }
    inline void set_gbs_radius( float r ) { gbs_radius = r; }

    inline const std::vector<SGVec3d>& get_wgs84_nodes() const { return wgs84_nodes; }
    inline void set_wgs84_nodes( const std::vector<SGVec3d>& n ) { wgs84_nodes = n; }

    inline const std::vector<SGVec4f>& get_colors() const { return colors; }
    inline void set_colors( const std::vector<SGVec4f>& c ) { colors = c; }
    
    inline const std::vector<SGVec3f>& get_normals() const { return normals; }
    inline void set_normals( const std::vector<SGVec3f>& n ) { normals = n; }
    
    inline const std::vector<SGVec2f>& get_texcoords() const { return texcoords; }
    inline void set_texcoords( const std::vector<SGVec2f>& t ) { texcoords = t; }
    
    // Points API
    bool add_point( const SGBinObjectPoint& pt );
    inline const group_list& get_pts_v() const { return pts_v; }
    inline const group_list& get_pts_n() const { return pts_n; }    
    inline const group_tci_list& get_pts_tcs() const { return pts_tcs; }
    inline const group_vai_list& get_pts_vas() const { return pts_vas; }
    inline const string_list& get_pt_materials() const { return pt_materials; }

    // Triangles API
    bool add_triangle( const SGBinObjectTriangle& tri );
    inline const group_list& get_tris_v() const { return tris_v; }
    inline const group_list& get_tris_n() const { return tris_n; }
    inline const group_list& get_tris_c() const { return tris_c; }
    inline const group_tci_list& get_tris_tcs() const { return tris_tcs; }
    inline const group_vai_list& get_tris_vas() const { return tris_vas; }
    inline const string_list& get_tri_materials() const { return tri_materials; }
    
    // Strips API (deprecated - read only)
    inline const group_list& get_strips_v() const { return strips_v; }
    inline const group_list& get_strips_n() const { return strips_n; }
    inline const group_list& get_strips_c() const { return strips_c; }
    inline const group_tci_list& get_strips_tcs() const { return strips_tcs; }
    inline const group_vai_list& get_strips_vas() const { return strips_vas; }
    inline const string_list& get_strip_materials() const { return strip_materials; }

    // Fans API (deprecated - read only )
    inline const group_list& get_fans_v() const { return fans_v; }
    inline const group_list& get_fans_n() const { return fans_n; }
    inline const group_list& get_fans_c() const { return fans_c; }
    inline const group_tci_list& get_fans_tcs() const { return fans_tcs; }
    inline const group_vai_list& get_fans_vas() const { return fans_vas; }
    inline const string_list& get_fan_materials() const { return fan_materials; }

    /**
     * Read a binary file object and populate the provided structures.
     * @param file input file name
     * @return result of read
     */
    bool read_bin( const std::string& file );

    /** 
     * Write out the structures to a binary file.  We assume that the
     * groups come to us sorted by material property.  If not, things
     * don't break, but the result won't be as optimal.
     * @param base name of output path
     * @param name name of output file
     * @param b bucket for object location
     * @return result of write
     */
    bool write_bin( const std::string& base, const std::string& name, const SGBucket& b );


    bool write_bin_file(const SGPath& file);

    /**
     * Write out the structures to an ASCII file.  We assume that the
     * groups come to us sorted by material property.  If not, things
     * don't break, but the result won't be as optimal.
     * @param base name of output path
     * @param name name of output file
     * @param b bucket for object location
     * @return result of write
     */
    bool write_ascii( const std::string& base, const std::string& name,
		      const SGBucket& b );
};

#endif // _SG_BINOBJ_HXX
