// sg_binobj.cxx -- routines to read and write low level flightgear 3d objects
//
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
//


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>

#include <stdio.h>
#include <time.h>
#include <cstring>
#include <cstdlib> // for system()
#include <cassert>

#include <vector>
#include <string>
#include <iostream>
#include <bitset>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/structure/exception.hxx>

#include "lowlevel.hxx"
#include "sg_binobj.hxx"


using std::string;
using std::vector;
using std::cout;
using std::endl;

enum sgObjectTypes {
    SG_BOUNDING_SPHERE = 0,

    SG_VERTEX_LIST = 1,
    SG_COLOR_LIST = 4,
    SG_NORMAL_LIST = 2,
    SG_TEXCOORD_LIST = 3,

    SG_POINTS = 9,

    SG_TRIANGLE_FACES = 10,
    SG_TRIANGLE_STRIPS = 11,
    SG_TRIANGLE_FANS = 12
};

enum sgIndexTypes {
    SG_IDX_VERTICES =  0x01,
    SG_IDX_NORMALS =   0x02,
    SG_IDX_COLORS =    0x04,
    SG_IDX_TEXCOORDS = 0x08
};

enum sgPropertyTypes {
    SG_MATERIAL = 0,
    SG_INDEX_TYPES = 1
};


class sgSimpleBuffer {

private:

    char *ptr;
    unsigned int size;
    size_t offset;
public:

    sgSimpleBuffer( unsigned int s = 0) :
        ptr(NULL),
        size(0),
        offset(0)
    {
        resize(s);
    }

    ~sgSimpleBuffer()
    {
        delete [] ptr;
    }

    unsigned int get_size() const { return size; }
    char *get_ptr() const { return ptr; }
    
    void reset()
    {
        offset = 0;
    }
    
    void resize( unsigned int s )
    {
        if ( s > size ) {
            if ( ptr != NULL ) {
                delete [] ptr;
            }
          
            if ( size == 0) {
                size = 16;
            }
          
            while ( size < s ) {
              size = size << 1;
            }
            ptr = new char[size];
        }
    }
    
    SGVec3d readVec3d()
    {
        double* p = reinterpret_cast<double*>(ptr + offset);
        
        if ( sgIsBigEndian() ) {            
            sgEndianSwap((uint64_t *) p + 0);
            sgEndianSwap((uint64_t *) p + 1);
            sgEndianSwap((uint64_t *) p + 2);
        }
        
        offset += 3 * sizeof(double);
        return SGVec3d(p);
    }
    
    float readFloat()
    {
        float* p = reinterpret_cast<float*>(ptr + offset);
        if ( sgIsBigEndian() ) {            
            sgEndianSwap((uint32_t *) p);
        }
        
        offset += sizeof(float);
        return *p;
    }
    
    SGVec2f readVec2f()
    {
        float* p = reinterpret_cast<float*>(ptr + offset);
        
        if ( sgIsBigEndian() ) {            
            sgEndianSwap((uint32_t *) p + 0);
            sgEndianSwap((uint32_t *) p + 1);
        }
        
        offset += 2 * sizeof(float);
        return SGVec2f(p);
    }
    
    SGVec3f readVec3f()
    {
        float* p = reinterpret_cast<float*>(ptr + offset);
        
        if ( sgIsBigEndian() ) {            
            sgEndianSwap((uint32_t *) p + 0);
            sgEndianSwap((uint32_t *) p + 1);
            sgEndianSwap((uint32_t *) p + 2);
        }
        
        offset += 3 * sizeof(float);
        return SGVec3f(p);
    }
    
    SGVec4f readVec4f()
    {
        float* p = reinterpret_cast<float*>(ptr + offset);
        
        if ( sgIsBigEndian() ) {            
            sgEndianSwap((uint32_t *) p + 0);
            sgEndianSwap((uint32_t *) p + 1);
            sgEndianSwap((uint32_t *) p + 2);
            sgEndianSwap((uint32_t *) p + 3);
        }
        
        offset += 4 * sizeof(float);
        return SGVec4f(p);
    }
};

template <class T>
static void read_indices(char* buffer, 
                         size_t bytes,
                         int indexMask,
                         int_list& vertices, 
                         int_list& normals,
                         int_list& colors,
                         int_list& texCoords)
{
    const int indexSize = sizeof(T) * std::bitset<32>(indexMask).count();
    const int count = bytes / indexSize;
    
// fix endian-ness of the whole lot, if required
    if (sgIsBigEndian()) {
        int indices = bytes / sizeof(T);
        T* src = reinterpret_cast<T*>(buffer);
        for (int i=0; i<indices; ++i) {
            sgEndianSwap(src++);
        }
    }
    
    T* src = reinterpret_cast<T*>(buffer);
    for (int i=0; i<count; ++i) {
        if (indexMask & SG_IDX_VERTICES) vertices.push_back(*src++);
        if (indexMask & SG_IDX_NORMALS) normals.push_back(*src++);
        if (indexMask & SG_IDX_COLORS) colors.push_back(*src++);
        if (indexMask & SG_IDX_TEXCOORDS) texCoords.push_back(*src++);
    } // of elements in the index
}

template <class T>
void write_indice(gzFile fp, T value)
{
    sgWriteBytes(fp, sizeof(T), &value);
}

// specialize template to call endian-aware conversion methods
template <>
void write_indice(gzFile fp, uint16_t value)
{
    sgWriteUShort(fp, value);
}

template <>
void write_indice(gzFile fp, uint32_t value)
{
    sgWriteUInt(fp, value);
}


template <class T>
void write_indices(gzFile fp, unsigned char indexMask, 
    const int_list& vertices, 
    const int_list& normals, 
    const int_list& colors,
    const int_list& texCoords)
{
    unsigned int count = vertices.size();
    const int indexSize = sizeof(T) * std::bitset<32>(indexMask).count();
    sgWriteUInt(fp, indexSize * count);
            
    for (unsigned int i=0; i < count; ++i) {
        write_indice(fp, static_cast<T>(vertices[i]));
        
        if (indexMask & SG_IDX_NORMALS) {
            write_indice(fp, static_cast<T>(normals[i]));
        }
        if (indexMask & SG_IDX_COLORS) {
            write_indice(fp, static_cast<T>(colors[i]));
        }
        if (indexMask & SG_IDX_TEXCOORDS) {
            write_indice(fp, static_cast<T>(texCoords[i]));
        }
    }
}


// read object properties
void SGBinObject::read_object( gzFile fp,
                         int obj_type,
                         int nproperties,
                         int nelements,
                         group_list& vertices, 
                          group_list& normals,
                          group_list& colors,
                          group_list& texCoords,
                          string_list& materials)
{
    unsigned int nbytes;
    unsigned char idx_mask;
    int j;
    sgSimpleBuffer buf( 32768 );  // 32 Kb
    char material[256];

    // default values
    if ( obj_type == SG_POINTS ) {
        idx_mask = SG_IDX_VERTICES;
    } else {
        idx_mask = (char)(SG_IDX_VERTICES | SG_IDX_TEXCOORDS);
    }

    for ( j = 0; j < nproperties; ++j ) {
        char prop_type;
        sgReadChar( fp, &prop_type );
        sgReadUInt( fp, &nbytes );
        buf.resize(nbytes);
        char *ptr = buf.get_ptr();
        sgReadBytes( fp, nbytes, ptr );
        if ( prop_type == SG_MATERIAL ) {
            if (nbytes > 255) {
                nbytes = 255;
            }
            strncpy( material, ptr, nbytes );
            material[nbytes] = '\0';
            // cout << "material type = " << material << endl;
        } else if ( prop_type == SG_INDEX_TYPES ) {
            idx_mask = ptr[0];
            //cout << std::hex << "index mask:" << idx_mask << std::dec << endl;
        }
    }

    if ( sgReadError() ) {
        throw sg_exception("Error reading object properties");
    }
    
    size_t indexCount = std::bitset<32>(idx_mask).count();
    if (indexCount == 0) {
        throw sg_exception("object index mask has no bits set");
    }
    
    for ( j = 0; j < nelements; ++j ) {
        sgReadUInt( fp, &nbytes );
        if ( sgReadError() ) {
            throw sg_exception("Error reading element size");
        }
        
        buf.resize( nbytes );
        char *ptr = buf.get_ptr();
        sgReadBytes( fp, nbytes, ptr );
        
        if ( sgReadError() ) {
            throw sg_exception("Error reading element bytes");
        }
                
        int_list vs;
        int_list ns;
        int_list cs;
        int_list tcs;
        if (version >= 10) {
            read_indices<uint32_t>(ptr, nbytes, idx_mask, vs, ns, cs, tcs);
        } else {
            read_indices<uint16_t>(ptr, nbytes, idx_mask, vs, ns, cs, tcs);
        }

        vertices.push_back( vs );
        normals.push_back( ns );
        colors.push_back( cs );
        texCoords.push_back( tcs );
        materials.push_back( material );
    } // of element iteration
}


// read a binary file and populate the provided structures.
bool SGBinObject::read_bin( const string& file ) {
    SGVec3d p;
    int i, k;
    size_t j;
    unsigned int nbytes;
    sgSimpleBuffer buf( 32768 );  // 32 Kb

    // zero out structures
    gbs_center = SGVec3d(0, 0, 0);
    gbs_radius = 0.0;

    wgs84_nodes.clear();
    normals.clear();
    texcoords.clear();

    pts_v.clear();
    pts_n.clear();
    pts_c.clear();
    pts_tc.clear();
    pt_materials.clear();

    tris_v.clear();
    tris_n.clear();
    tris_c.clear();
    tris_tc.clear();
    tri_materials.clear();

    strips_v.clear();
    strips_n.clear();
    strips_c.clear();
    strips_tc.clear();
    strip_materials.clear();

    fans_v.clear();
    fans_n.clear();
    fans_c.clear();
    fans_tc.clear();
    fan_materials.clear();

    gzFile fp;
    if ( (fp = gzopen( file.c_str(), "rb" )) == NULL ) {
        string filegz = file + ".gz";
        if ( (fp = gzopen( filegz.c_str(), "rb" )) == NULL ) {
            SG_LOG( SG_EVENT, SG_ALERT,
               "ERROR: opening " << file << " or " << filegz << " for reading!");

            throw sg_io_exception("Error opening for reading (and .gz)", sg_location(file));
        }
    }

    sgClearReadError();

    // read headers
    unsigned int header;
    sgReadUInt( fp, &header );
    if ( ((header & 0xFF000000) >> 24) == 'S' &&
         ((header & 0x00FF0000) >> 16) == 'G' ) {
        // cout << "Good header" << endl;
        // read file version
        version = (header & 0x0000FFFF);
        // cout << "File version = " << version << endl;
    } else {
        // close the file before we return
        gzclose(fp);
        throw sg_io_exception("Bad BTG magic/version", sg_location(file));
    }
    
    // read creation time
    unsigned int foo_calendar_time;
    sgReadUInt( fp, &foo_calendar_time );

#if 0
    time_t calendar_time = foo_calendar_time;
    // The following code has a global effect on the host application
    // and can screws up the time elsewhere.  It should be avoided
    // unless you need this for debugging in which case you should
    // disable it again once the debugging task is finished.
    struct tm *local_tm;
    local_tm = localtime( &calendar_time );
    char time_str[256];
    strftime( time_str, 256, "%a %b %d %H:%M:%S %Z %Y", local_tm);
    SG_LOG( SG_EVENT, SG_DEBUG, "File created on " << time_str);
#endif

    // read number of top level objects
    int nobjects;
    if ( version >= 10) { // version 10 extends everything to be 32-bit
        sgReadInt( fp, &nobjects );
    } else if ( version >= 7 ) {
        uint16_t v;
        sgReadUShort( fp, &v );
        nobjects = v;
    } else {
        int16_t v;
        sgReadShort( fp, &v );
        nobjects = v;
    }
     
     //cout << "Total objects to read = " << nobjects << endl;

    if ( sgReadError() ) {
        throw sg_io_exception("Error reading BTG file header", sg_location(file));
    }
    
    // read in objects
    for ( i = 0; i < nobjects; ++i ) {
        // read object header
        char obj_type;
        uint32_t nproperties, nelements;
        sgReadChar( fp, &obj_type );
        if ( version >= 10 ) {
            sgReadUInt( fp, &nproperties );
            sgReadUInt( fp, &nelements );
        } else if ( version >= 7 ) {
            uint16_t v;
            sgReadUShort( fp, &v );
            nproperties = v;
            sgReadUShort( fp, &v );
            nelements = v;
        } else {
            int16_t v;
            sgReadShort( fp, &v );
            nproperties = v;
            sgReadShort( fp, &v );
            nelements = v;
        }

         //cout << "object " << i << " = " << (int)obj_type << " props = "
         //     << nproperties << " elements = " << nelements << endl;
            
        if ( obj_type == SG_BOUNDING_SPHERE ) {
            // read bounding sphere properties
            read_properties( fp, nproperties );
            
            // read bounding sphere elements
            for ( j = 0; j < nelements; ++j ) {
                sgReadUInt( fp, &nbytes );
                buf.resize( nbytes );
                buf.reset();
                char *ptr = buf.get_ptr();
                sgReadBytes( fp, nbytes, ptr );
                gbs_center = buf.readVec3d();
                gbs_radius = buf.readFloat();
            }
        } else if ( obj_type == SG_VERTEX_LIST ) {
            // read vertex list properties
            read_properties( fp, nproperties );

            // read vertex list elements
            for ( j = 0; j < nelements; ++j ) {
                sgReadUInt( fp, &nbytes );
                buf.resize( nbytes );
                buf.reset();
                char *ptr = buf.get_ptr();
                sgReadBytes( fp, nbytes, ptr );
                int count = nbytes / (sizeof(float) * 3);
                wgs84_nodes.reserve( count );
                for ( k = 0; k < count; ++k ) {
                    SGVec3f v = buf.readVec3f();
                // extend from float to double, hmmm
                    wgs84_nodes.push_back( SGVec3d(v[0], v[1], v[2]) );
                }
            }
        } else if ( obj_type == SG_COLOR_LIST ) {
            // read color list properties
            read_properties( fp, nproperties );

            // read color list elements
            for ( j = 0; j < nelements; ++j ) {
                sgReadUInt( fp, &nbytes );
                buf.resize( nbytes );
                buf.reset();
                char *ptr = buf.get_ptr();
                sgReadBytes( fp, nbytes, ptr );
                int count = nbytes / (sizeof(float) * 4);
                colors.reserve(count);
                for ( k = 0; k < count; ++k ) {
                    colors.push_back( buf.readVec4f() );
                }
            }
        } else if ( obj_type == SG_NORMAL_LIST ) {
            // read normal list properties
            read_properties( fp, nproperties );

            // read normal list elements
            for ( j = 0; j < nelements; ++j ) {
                sgReadUInt( fp, &nbytes );
                buf.resize( nbytes );
                buf.reset();
                unsigned char *ptr = (unsigned char *)(buf.get_ptr());
                sgReadBytes( fp, nbytes, ptr );
                int count = nbytes / 3;
                normals.reserve( count );
 
                for ( k = 0; k < count; ++k ) {
                    SGVec3f normal( (ptr[0]) / 127.5 - 1.0,
                                    (ptr[1]) / 127.5 - 1.0, 
                                    (ptr[2]) / 127.5 - 1.0);
                    normals.push_back(normalize(normal));
                    ptr += 3;
                }
            }
        } else if ( obj_type == SG_TEXCOORD_LIST ) {
            // read texcoord list properties
            read_properties( fp, nproperties );

            // read texcoord list elements
            for ( j = 0; j < nelements; ++j ) {
                sgReadUInt( fp, &nbytes );
                buf.resize( nbytes );
                buf.reset();
                char *ptr = buf.get_ptr();
                sgReadBytes( fp, nbytes, ptr );
                int count = nbytes / (sizeof(float) * 2);
                texcoords.reserve(count);
                for ( k = 0; k < count; ++k ) {
                    texcoords.push_back( buf.readVec2f() );
                }
            }
        } else if ( obj_type == SG_POINTS ) {
            // read point elements
            read_object( fp, SG_POINTS, nproperties, nelements,
                         pts_v, pts_n, pts_c, pts_tc, pt_materials );
        } else if ( obj_type == SG_TRIANGLE_FACES ) {
            // read triangle face properties
            read_object( fp, SG_TRIANGLE_FACES, nproperties, nelements,
                         tris_v, tris_n, tris_c, tris_tc, tri_materials );
        } else if ( obj_type == SG_TRIANGLE_STRIPS ) {
            // read triangle strip properties
            read_object( fp, SG_TRIANGLE_STRIPS, nproperties, nelements,
                         strips_v, strips_n, strips_c, strips_tc,
                         strip_materials );
        } else if ( obj_type == SG_TRIANGLE_FANS ) {
            // read triangle fan properties
            read_object( fp, SG_TRIANGLE_FANS, nproperties, nelements,
                         fans_v, fans_n, fans_c, fans_tc, fan_materials );
        } else {
            // unknown object type, just skip
            read_properties( fp, nproperties );

            // read elements
            for ( j = 0; j < nelements; ++j ) {
                sgReadUInt( fp, &nbytes );
                // cout << "element size = " << nbytes << endl;
                if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
                char *ptr = buf.get_ptr();
                sgReadBytes( fp, nbytes, ptr );
            }
        }
        
        if ( sgReadError() ) {
            throw sg_io_exception("Error while reading object", sg_location(file, i));
        }
    }

    // close the file
    gzclose(fp);

    return true;
}

void SGBinObject::write_header(gzFile fp, int type, int nProps, int nElements)
{
    sgWriteChar(fp, (unsigned char) type);
    if (version == 7) {
        sgWriteUShort(fp, nProps);
        sgWriteUShort(fp, nElements);
    } else {
        sgWriteUInt(fp, nProps);
        sgWriteUInt(fp, nElements);
    }
}

unsigned int SGBinObject::count_objects(const string_list& materials)
{
    unsigned int result = 0;
    unsigned int start = 0, end = 1;
    unsigned int count = materials.size();
    string m;
    
    while ( start < count ) {
        m = materials[start];
        for (end = start+1; (end < count) && (m == materials[end]); ++end) { }     
        ++result;
        start = end; 
    }
    
    return result;
}

void SGBinObject::write_objects(gzFile fp, int type, const group_list& verts,
    const group_list& normals, const group_list& colors, 
    const group_list& texCoords, const string_list& materials)
{
    if (verts.empty()) {
        return;
    }
        
    unsigned int start = 0, end = 1;
    string m;
    int_list emptyList;
    
    while (start < materials.size()) {
        m = materials[start];
    // find range of objects with identical material, write out as a single object
        for (end = start+1; (end < materials.size()) && (m == materials[end]); ++end) {}
    
        const int count = end - start;
        write_header(fp, type, 2, count);
        
    // properties
        sgWriteChar( fp, (char)SG_MATERIAL );        // property
        sgWriteUInt( fp, m.length() );        // nbytes
        sgWriteBytes( fp, m.length(), m.c_str() );
    
        unsigned char idx_mask = 0;
        if ( !verts.empty() && !verts.front().empty()) idx_mask |= SG_IDX_VERTICES;
        if ( !normals.empty() && !normals.front().empty()) idx_mask |= SG_IDX_NORMALS;
        if ( !colors.empty() && !colors.front().empty()) idx_mask |= SG_IDX_COLORS;
        if ( !texCoords.empty() && !texCoords.front().empty()) idx_mask |= SG_IDX_TEXCOORDS;
        
        if (idx_mask == 0) {
            SG_LOG(SG_IO, SG_ALERT, "SGBinObject::write_objects: object with material:"
                << m << "has no indices set");
        }
    
    
        sgWriteChar( fp, (char)SG_INDEX_TYPES );     // property
        sgWriteUInt( fp, 1 );                        // nbytes
        sgWriteChar( fp, idx_mask );
    
//        cout << "material:" << m << ", count =" << count << endl;
    // elements
        for (unsigned int i=start; i < end; ++i) {
            const int_list& va(verts[i]);
            const int_list& na((idx_mask & SG_IDX_NORMALS) ? normals[i] : emptyList);
            const int_list& ca((idx_mask & SG_IDX_COLORS) ? colors[i] : emptyList);
            const int_list& tca((idx_mask & SG_IDX_TEXCOORDS) ? texCoords[i] : emptyList);
            
            if (version == 7) {
                write_indices<uint16_t>(fp, idx_mask, va, na, ca, tca);
            } else {
                write_indices<uint32_t>(fp, idx_mask, va, na, ca, tca);
            }
        }
    
        start = end;
    } // of materials iteration
}

// write out the structures to a binary file.  We assume that the
// groups come to us sorted by material property.  If not, things
// don't break, but the result won't be as optimal.
bool SGBinObject::write_bin( const string& base, const string& name,
                             const SGBucket& b )
{

    SGPath file = base + "/" + b.gen_base_path() + "/" + name + ".gz";
    return write_bin_file(file);
}

static unsigned int max_object_size( const string_list& materials )
{
    unsigned int max_size = 0;

    for (unsigned int start=0; start < materials.size();) {
        string m = materials[start];
        unsigned int end = start + 1;
        // find range of objects with identical material, calc its size
        for (; (end < materials.size()) && (m == materials[end]); ++end) {}

        unsigned int cur_size = end - start;
        max_size = std::max(max_size, cur_size);
        start = end;
    }

    return max_size;
}

const unsigned int VERSION_7_MATERIAL_LIMIT = 0x7fff;

bool SGBinObject::write_bin_file(const SGPath& file)
{
    int i;
    
    SGPath file2(file);
    file2.create_dir( 0755 );
    cout << "Output file = " << file.str() << endl;

    gzFile fp;
    if ( (fp = gzopen( file.c_str(), "wb9" )) == NULL ) {
        cout << "ERROR: opening " << file.str() << " for writing!" << endl;
        return false;
    }

    sgClearWriteError();

    cout << "points size = " << pts_v.size() << "  pt_materials = " 
         << pt_materials.size() << endl;
    cout << "triangles size = " << tris_v.size() << "  tri_materials = " 
         << tri_materials.size() << endl;
    cout << "strips size = " << strips_v.size() << "  strip_materials = " 
         << strip_materials.size() << endl;
    cout << "fans size = " << fans_v.size() << "  fan_materials = " 
         << fan_materials.size() << endl;

    cout << "nodes = " << wgs84_nodes.size() << endl;
    cout << "colors = " << colors.size() << endl;
    cout << "normals = " << normals.size() << endl;
    cout << "tex coords = " << texcoords.size() << endl;

    version = 10;
    bool shortMaterialsRanges = 
        (max_object_size(pt_materials) < VERSION_7_MATERIAL_LIMIT) &&
        (max_object_size(fan_materials) < VERSION_7_MATERIAL_LIMIT) &&
        (max_object_size(strip_materials) < VERSION_7_MATERIAL_LIMIT) &&
        (max_object_size(tri_materials) < VERSION_7_MATERIAL_LIMIT);
                    
    if ((wgs84_nodes.size() < 0xffff) &&
        (normals.size() < 0xffff) &&
        (texcoords.size() < 0xffff) &&
        shortMaterialsRanges) {
        version = 7; // use smaller indices if possible
    }

    // write header magic
    
    /** Magic Number for our file format */
    #define SG_FILE_MAGIC_NUMBER  ( ('S'<<24) + ('G'<<16) + version )
    
    sgWriteUInt( fp, SG_FILE_MAGIC_NUMBER );
    time_t calendar_time = time(NULL);
    sgWriteLong( fp, (int32_t)calendar_time );

    // calculate and write number of top level objects
    int nobjects = 5; // gbs, vertices, colors, normals, texcoords
    nobjects += count_objects(pt_materials);
    nobjects += count_objects(tri_materials);
    nobjects += count_objects(strip_materials);
    nobjects += count_objects(fan_materials);

    cout << "total top level objects = " << nobjects << endl;
    if (version == 7) {
        sgWriteUShort( fp, (uint16_t) nobjects );
    } else {
        sgWriteInt( fp, nobjects );
    } 
    
    // write bounding sphere
    write_header( fp, SG_BOUNDING_SPHERE, 0, 1);
    sgWriteUInt( fp, sizeof(double) * 3 + sizeof(float) ); // nbytes
    sgWritedVec3( fp, gbs_center );
    sgWriteFloat( fp, gbs_radius );

    // dump vertex list
    write_header( fp, SG_VERTEX_LIST, 0, 1);
    sgWriteUInt( fp, wgs84_nodes.size() * sizeof(float) * 3 ); // nbytes
    for ( i = 0; i < (int)wgs84_nodes.size(); ++i ) {
        sgWriteVec3( fp, toVec3f(wgs84_nodes[i] - gbs_center));
    }

    // dump vertex color list
    write_header( fp, SG_COLOR_LIST, 0, 1);
    sgWriteUInt( fp, colors.size() * sizeof(float) * 4 ); // nbytes
    for ( i = 0; i < (int)colors.size(); ++i ) {
      sgWriteVec4( fp, colors[i]);
    }

    // dump vertex normal list
    write_header( fp, SG_NORMAL_LIST, 0, 1);
    sgWriteUInt( fp, normals.size() * 3 );              // nbytes
    char normal[3];
    for ( i = 0; i < (int)normals.size(); ++i ) {
        SGVec3f p = normals[i];
        normal[0] = (unsigned char)((p.x() + 1.0) * 127.5);
        normal[1] = (unsigned char)((p.y() + 1.0) * 127.5);
        normal[2] = (unsigned char)((p.z() + 1.0) * 127.5);
        sgWriteBytes( fp, 3, normal );
    }

    // dump texture coordinates
    write_header( fp, SG_TEXCOORD_LIST, 0, 1);
    sgWriteUInt( fp, texcoords.size() * sizeof(float) * 2 ); // nbytes
    for ( i = 0; i < (int)texcoords.size(); ++i ) {
      sgWriteVec2( fp, texcoords[i]);
    }

    write_objects(fp, SG_POINTS, pts_v, pts_n, pts_c, pts_tc, pt_materials);
    write_objects(fp, SG_TRIANGLE_FACES, tris_v, tris_n, tris_c, tris_tc, tri_materials);
    write_objects(fp, SG_TRIANGLE_STRIPS, strips_v, strips_n, strips_c, strips_tc, strip_materials);
    write_objects(fp, SG_TRIANGLE_FANS, fans_v, fans_n, fans_c, fans_tc, fan_materials);
    
    // close the file
    gzclose(fp);

    if ( sgWriteError() ) {
        cout << "Error while writing file " << file.str() << endl;
        return false;
    }

    return true;
}


// write out the structures to an ASCII file.  We assume that the
// groups come to us sorted by material property.  If not, things
// don't break, but the result won't be as optimal.
bool SGBinObject::write_ascii( const string& base, const string& name,
                               const SGBucket& b )
{
    int i, j;

    SGPath file = base + "/" + b.gen_base_path() + "/" + name;
    file.create_dir( 0755 );
    cout << "Output file = " << file.str() << endl;

    FILE *fp;
    if ( (fp = fopen( file.c_str(), "w" )) == NULL ) {
        cout << "ERROR: opening " << file.str() << " for writing!" << endl;
        return false;
    }

    cout << "triangles size = " << tris_v.size() << "  tri_materials = " 
         << tri_materials.size() << endl;
    cout << "strips size = " << strips_v.size() << "  strip_materials = " 
         << strip_materials.size() << endl;
    cout << "fans size = " << fans_v.size() << "  fan_materials = " 
         << fan_materials.size() << endl;

    cout << "points = " << wgs84_nodes.size() << endl;
    cout << "tex coords = " << texcoords.size() << endl;
    // write headers
    fprintf(fp, "# FGFS Scenery\n");
    fprintf(fp, "# Version %s\n", SG_SCENERY_FILE_FORMAT);

    time_t calendar_time = time(NULL);
    struct tm *local_tm;
    local_tm = localtime( &calendar_time );
    char time_str[256];
    strftime( time_str, 256, "%a %b %d %H:%M:%S %Z %Y", local_tm);
    fprintf(fp, "# Created %s\n", time_str );
    fprintf(fp, "\n");

    // write bounding sphere
    fprintf(fp, "# gbs %.5f %.5f %.5f %.2f\n",
            gbs_center.x(), gbs_center.y(), gbs_center.z(), gbs_radius);
    fprintf(fp, "\n");

    // dump vertex list
    fprintf(fp, "# vertex list\n");
    for ( i = 0; i < (int)wgs84_nodes.size(); ++i ) {
        SGVec3d p = wgs84_nodes[i] - gbs_center;
        
        fprintf(fp,  "v %.5f %.5f %.5f\n", p.x(), p.y(), p.z() );
    }
    fprintf(fp, "\n");

    fprintf(fp, "# vertex normal list\n");
    for ( i = 0; i < (int)normals.size(); ++i ) {
        SGVec3f p = normals[i];
        fprintf(fp,  "vn %.5f %.5f %.5f\n", p.x(), p.y(), p.z() );
    }
    fprintf(fp, "\n");

    // dump texture coordinates
    fprintf(fp, "# texture coordinate list\n");
    for ( i = 0; i < (int)texcoords.size(); ++i ) {
        SGVec2f p = texcoords[i];
        fprintf(fp,  "vt %.5f %.5f\n", p.x(), p.y() );
    }
    fprintf(fp, "\n");

    // dump individual triangles if they exist
    if ( tris_v.size() > 0 ) {
        fprintf(fp, "# triangle groups\n");

        int start = 0;
        int end = 1;
        string material;
        while ( start < (int)tri_materials.size() ) {
            // find next group
            material = tri_materials[start];
            while ( (end < (int)tri_materials.size()) && 
                    (material == tri_materials[end]) )
            {
                // cout << "end = " << end << endl;
                end++;
            }
            // cout << "group = " << start << " to " << end - 1 << endl;

      SGSphered d;
      for ( i = start; i < end; ++i ) {
        for ( j = 0; j < (int)tris_v[i].size(); ++j ) {
          d.expandBy(wgs84_nodes[ tris_v[i][j] ]);
        }
      }
      
      SGVec3d bs_center = d.getCenter();
      double bs_radius = d.getRadius();
            
            // write group headers
            fprintf(fp, "\n");
            fprintf(fp, "# usemtl %s\n", material.c_str());
            fprintf(fp, "# bs %.4f %.4f %.4f %.2f\n",
                    bs_center.x(), bs_center.y(), bs_center.z(), bs_radius);

            // write groups
            for ( i = start; i < end; ++i ) {
                fprintf(fp, "f");
                for ( j = 0; j < (int)tris_v[i].size(); ++j ) {
                    fprintf(fp, " %d/%d", tris_v[i][j], tris_tc[i][j] );
                }
                fprintf(fp, "\n");
            }

            start = end;
            end = start + 1;
        }
    }

    // dump triangle groups
    if ( strips_v.size() > 0 ) {
        fprintf(fp, "# triangle strips\n");

        int start = 0;
        int end = 1;
        string material;
        while ( start < (int)strip_materials.size() ) {
            // find next group
            material = strip_materials[start];
            while ( (end < (int)strip_materials.size()) && 
                    (material == strip_materials[end]) )
                {
                    // cout << "end = " << end << endl;
                    end++;
                }
            // cout << "group = " << start << " to " << end - 1 << endl;


      SGSphered d;
      for ( i = start; i < end; ++i ) {
        for ( j = 0; j < (int)tris_v[i].size(); ++j ) {
          d.expandBy(wgs84_nodes[ tris_v[i][j] ]);
        }
      }
      
      SGVec3d bs_center = d.getCenter();
      double bs_radius = d.getRadius();

            // write group headers
            fprintf(fp, "\n");
            fprintf(fp, "# usemtl %s\n", material.c_str());
            fprintf(fp, "# bs %.4f %.4f %.4f %.2f\n",
                    bs_center.x(), bs_center.y(), bs_center.z(), bs_radius);

            // write groups
            for ( i = start; i < end; ++i ) {
                fprintf(fp, "ts");
                for ( j = 0; j < (int)strips_v[i].size(); ++j ) {
                    fprintf(fp, " %d/%d", strips_v[i][j], strips_tc[i][j] );
                }
                fprintf(fp, "\n");
            }
            
            start = end;
            end = start + 1;
        }
    }

    // close the file
    fclose(fp);

    string command = "gzip --force --best " + file.str();
    int err = system(command.c_str());
    if (err)
    {
        cout << "ERROR: gzip " << file.str() << " failed!" << endl;
    }

    return (err == 0);
}

void SGBinObject::read_properties(gzFile fp, int nproperties)
{
    sgSimpleBuffer buf;
    uint32_t nbytes;
    
    // read properties
    for ( int j = 0; j < nproperties; ++j ) {
        char prop_type;
        sgReadChar( fp, &prop_type );
        sgReadUInt( fp, &nbytes );
        // cout << "property size = " << nbytes << endl;
        if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
        char *ptr = buf.get_ptr();
        sgReadBytes( fp, nbytes, ptr );
    }
}

