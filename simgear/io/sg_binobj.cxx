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

#include <vector>
#include STL_STRING

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>

#include "lowlevel.hxx"
#include "sg_binobj.hxx"


SG_USING_STD( string );
SG_USING_STD( vector );


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

public:

    inline sgSimpleBuffer( unsigned int s )
    {
	size = 1;
	while ( size < s ) {
	    size *= 2;
	}
        SG_LOG(SG_EVENT, SG_DEBUG, "Creating a new buffer of size = " << size);
	ptr = new char[size];
    }

    inline ~sgSimpleBuffer() {
	delete [] ptr;
    }

    inline unsigned int get_size() const { return size; }
    inline char *get_ptr() const { return ptr; }
    inline void resize( unsigned int s ) {
	if ( s > size ) {
	    if ( ptr != NULL ) {
		delete [] ptr;
	    }
	    while ( size < s ) {
		size *= 2;
	    }
            SG_LOG(SG_EVENT, SG_DEBUG, "resizing buffer to size = " << size);
	    ptr = new char[size];
	}
    }
};


// calculate the center of a list of points, by taking the halfway
// point between the min and max points.
Point3D sgCalcCenter( point_list& wgs84_nodes ) {
    Point3D p, min, max;

    if ( wgs84_nodes.size() ) {
	min = max = wgs84_nodes[0];
    } else {
	min = max = Point3D( 0 );
    }

    for ( int i = 0; i < (int)wgs84_nodes.size(); ++i ) {
	p = wgs84_nodes[i];

	if ( p.x() < min.x() ) { min.setx( p.x() ); }
	if ( p.y() < min.y() ) { min.sety( p.y() ); }
	if ( p.z() < min.z() ) { min.setz( p.z() ); }

	if ( p.x() > max.x() ) { max.setx( p.x() ); }
	if ( p.y() > max.y() ) { max.sety( p.y() ); }
	if ( p.z() > max.z() ) { max.setz( p.z() ); }
    }

    return ( min + max ) / 2.0;
}

// calculate the bounding sphere.  Center is the center of the
// tile and zero elevation
double sgCalcBoundingRadius( Point3D center, point_list& wgs84_nodes ) {
    double dist_squared;
    double radius_squared = 0;
    
    for ( int i = 0; i < (int)wgs84_nodes.size(); ++i ) {
        dist_squared = center.distance3Dsquared( wgs84_nodes[i] );
	if ( dist_squared > radius_squared ) {
            radius_squared = dist_squared;
        }
    }

    return sqrt(radius_squared);
}



// read object properties
static void read_object( gzFile fp,
			 int obj_type,
			 int nproperties,
			 int nelements,
			 group_list *vertices,
			 group_list *normals,
			 group_list *colors,
			 group_list *texcoords,
			 string_list *materials )
{
    unsigned int nbytes;
    unsigned char idx_mask;
    int idx_size;
    bool do_vertices, do_normals, do_colors, do_texcoords;
    int j, k, idx;
    static sgSimpleBuffer buf( 32768 );  // 32 Kb
    char material[256];

    // default values
    if ( obj_type == SG_POINTS ) {
	idx_size = 1;
	idx_mask = SG_IDX_VERTICES;
	do_vertices = true;
	do_normals = false;
	do_colors = false;
	do_texcoords = false;
    } else {
	idx_size = 2;
	idx_mask = (char)(SG_IDX_VERTICES | SG_IDX_TEXCOORDS);
	do_vertices = true;
	do_normals = false;
	do_colors = false;
	do_texcoords = true;
    }

    for ( j = 0; j < nproperties; ++j ) {
	char prop_type;
	sgReadChar( fp, &prop_type );

	sgReadUInt( fp, &nbytes );
	// cout << "property size = " << nbytes << endl;
	if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
	char *ptr = buf.get_ptr();
	sgReadBytes( fp, nbytes, ptr );
	if ( prop_type == SG_MATERIAL ) {
	    strncpy( material, ptr, nbytes );
	    material[nbytes] = '\0';
	    // cout << "material type = " << material << endl;
	} else if ( prop_type == SG_INDEX_TYPES ) {
	    idx_mask = ptr[0];
	    // cout << "idx_mask = " << (int)idx_mask << endl;
	    idx_size = 0;
	    do_vertices = false;
	    do_normals = false;
	    do_colors = false;
	    do_texcoords = false;
	    if ( idx_mask & SG_IDX_VERTICES ) {
		do_vertices = true;
		++idx_size;
	    }
	    if ( idx_mask & SG_IDX_NORMALS ) {
		do_normals = true;
		++idx_size;
	    }
	    if ( idx_mask & SG_IDX_COLORS ) {
		do_colors = true;
		++idx_size;
	    }
	    if ( idx_mask & SG_IDX_TEXCOORDS ) {
		do_texcoords = true;
		++idx_size;
	    }
	}
    }

    for ( j = 0; j < nelements; ++j ) {
	sgReadUInt( fp, &nbytes );
	// cout << "element size = " << nbytes << endl;
	if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
	char *ptr = buf.get_ptr();
	sgReadBytes( fp, nbytes, ptr );
	int count = nbytes / (idx_size * sizeof(unsigned short));
	unsigned short *sptr = (unsigned short *)ptr;
	int_list vs; vs.clear();
	int_list ns; ns.clear();
	int_list cs; cs.clear();
	int_list tcs; tcs.clear();
	for ( k = 0; k < count; ++k ) {
	    if ( sgIsBigEndian() ) {
		for ( idx = 0; idx < idx_size; ++idx ) {
		    sgEndianSwap( (uint16_t *)&(sptr[idx]) );
		}
	    }
	    idx = 0;
	    if ( do_vertices ) {
		vs.push_back( sptr[idx++] );
	    }
	    if ( do_normals ) {
		ns.push_back( sptr[idx++] );
		    }
	    if ( do_colors ) {
		cs.push_back( sptr[idx++] );
	    }
	    if ( do_texcoords ) {
		tcs.push_back( sptr[idx++] );
	    }
	    // cout << sptr[0] << " ";
	    sptr += idx_size;
	}
	// cout << endl;
	vertices->push_back( vs );
	normals->push_back( ns );
	colors->push_back( cs );
	texcoords->push_back( tcs );
	materials->push_back( material );
    }
}


// read a binary file and populate the provided structures.
bool SGBinObject::read_bin( const string& file ) {
    Point3D p;
    int i, j, k;
    unsigned int nbytes;
    static sgSimpleBuffer buf( 32768 );  // 32 Kb

    // zero out structures
    gbs_center = Point3D( 0 );
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
               "ERROR: opening " << file << " or " << filegz << "for reading!");

	    return false;
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

	return false;
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
    short nobjects;
    sgReadShort( fp, &nobjects );
    // cout << "Total objects to read = " << nobjects << endl;

    // read in objects
    for ( i = 0; i < nobjects; ++i ) {
	// read object header
	char obj_type;
	short nproperties, nelements;
	sgReadChar( fp, &obj_type );
	sgReadShort( fp, &nproperties );
	sgReadShort( fp, &nelements );

	// cout << "object " << i << " = " << (int)obj_type << " props = "
	//      << nproperties << " elements = " << nelements << endl;
	    
	if ( obj_type == SG_BOUNDING_SPHERE ) {
	    // read bounding sphere properties
	    for ( j = 0; j < nproperties; ++j ) {
		char prop_type;
		sgReadChar( fp, &prop_type );

		sgReadUInt( fp, &nbytes );
		// cout << "property size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }

	    // read bounding sphere elements
	    for ( j = 0; j < nelements; ++j ) {
		sgReadUInt( fp, &nbytes );
		// cout << "element size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );

		double *dptr = (double *)ptr;
		if ( sgIsBigEndian() ) {
		    sgEndianSwap( (uint64_t *)&(dptr[0]) );
		    sgEndianSwap( (uint64_t *)&(dptr[1]) );
		    sgEndianSwap( (uint64_t *)&(dptr[2]) );
		}
		gbs_center = Point3D( dptr[0], dptr[1], dptr[2] );
		// cout << "Center = " << gbs_center << endl;
		ptr += sizeof(double) * 3;
		
		float *fptr = (float *)ptr;
		if ( sgIsBigEndian() ) {
		    sgEndianSwap( (uint32_t *)fptr );
		}
		gbs_radius = fptr[0];
		// cout << "Bounding radius = " << gbs_radius << endl;
	    }
	} else if ( obj_type == SG_VERTEX_LIST ) {
	    // read vertex list properties
	    for ( j = 0; j < nproperties; ++j ) {
		char prop_type;
		sgReadChar( fp, &prop_type );

		sgReadUInt( fp, &nbytes );
		// cout << "property size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }

	    // read vertex list elements
	    for ( j = 0; j < nelements; ++j ) {
		sgReadUInt( fp, &nbytes );
		// cout << "element size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
		int count = nbytes / (sizeof(float) * 3);
		float *fptr = (float *)ptr;
		wgs84_nodes.reserve( count );
		for ( k = 0; k < count; ++k ) {
		    if ( sgIsBigEndian() ) {
			sgEndianSwap( (uint32_t *)&(fptr[0]) );
			sgEndianSwap( (uint32_t *)&(fptr[1]) );
			sgEndianSwap( (uint32_t *)&(fptr[2]) );
		    }
		    wgs84_nodes.push_back( Point3D(fptr[0], fptr[1], fptr[2]) );
		    fptr += 3;
		}
	    }
	} else if ( obj_type == SG_COLOR_LIST ) {
	    // read color list properties
	    for ( j = 0; j < nproperties; ++j ) {
		char prop_type;
		sgReadChar( fp, &prop_type );

		sgReadUInt( fp, &nbytes );
		// cout << "property size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }

	    // read color list elements
	    for ( j = 0; j < nelements; ++j ) {
		sgReadUInt( fp, &nbytes );
		// cout << "element size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
		int count = nbytes / (sizeof(float) * 4);
		float *fptr = (float *)ptr;
		colors.reserve(count);
		for ( k = 0; k < count; ++k ) {
		    if ( sgIsBigEndian() ) {
			sgEndianSwap( (uint32_t *)&(fptr[0]) );
			sgEndianSwap( (uint32_t *)&(fptr[1]) );
			sgEndianSwap( (uint32_t *)&(fptr[2]) );
			sgEndianSwap( (uint32_t *)&(fptr[3]) );
		    }
		    colors.push_back( Point3D( fptr[0], fptr[1], fptr[2] ) );
		    fptr += 4;
		}
	    }
	} else if ( obj_type == SG_NORMAL_LIST ) {
	    // read normal list properties
	    for ( j = 0; j < nproperties; ++j ) {
		char prop_type;
		sgReadChar( fp, &prop_type );

		sgReadUInt( fp, &nbytes );
		// cout << "property size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }

	    // read normal list elements
	    for ( j = 0; j < nelements; ++j ) {
		sgReadUInt( fp, &nbytes );
		// cout << "element size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		unsigned char *ptr = (unsigned char *)(buf.get_ptr());
		sgReadBytes( fp, nbytes, ptr );
		int count = nbytes / 3;
		normals.reserve( count );
		for ( k = 0; k < count; ++k ) {
                    sgdVec3 normal;
                    sgdSetVec3( normal,
                               (ptr[0]) / 127.5 - 1.0,
                               (ptr[1]) / 127.5 - 1.0,
                               (ptr[2]) / 127.5 - 1.0 );
                    sgdNormalizeVec3( normal );

		    normals.push_back(Point3D(normal[0], normal[1], normal[2]));
		    ptr += 3;
		}
	    }
	} else if ( obj_type == SG_TEXCOORD_LIST ) {
	    // read texcoord list properties
	    for ( j = 0; j < nproperties; ++j ) {
		char prop_type;
		sgReadChar( fp, &prop_type );

		sgReadUInt( fp, &nbytes );
		// cout << "property size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }

	    // read texcoord list elements
	    for ( j = 0; j < nelements; ++j ) {
		sgReadUInt( fp, &nbytes );
		// cout << "element size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
		int count = nbytes / (sizeof(float) * 2);
		float *fptr = (float *)ptr;
		texcoords.reserve(count);
		for ( k = 0; k < count; ++k ) {
		    if ( sgIsBigEndian() ) {
			sgEndianSwap( (uint32_t *)&(fptr[0]) );
			sgEndianSwap( (uint32_t *)&(fptr[1]) );
		    }
		    texcoords.push_back( Point3D( fptr[0], fptr[1], 0 ) );
		    fptr += 2;
		}
	    }
	} else if ( obj_type == SG_POINTS ) {
	    // read point elements
	    read_object( fp, SG_POINTS, nproperties, nelements,
			 &pts_v, &pts_n, &pts_c, &pts_tc, &pt_materials );
	} else if ( obj_type == SG_TRIANGLE_FACES ) {
	    // read triangle face properties
	    read_object( fp, SG_TRIANGLE_FACES, nproperties, nelements,
			 &tris_v, &tris_n, &tris_c, &tris_tc, &tri_materials );
	} else if ( obj_type == SG_TRIANGLE_STRIPS ) {
	    // read triangle strip properties
	    read_object( fp, SG_TRIANGLE_STRIPS, nproperties, nelements,
			 &strips_v, &strips_n, &strips_c, &strips_tc,
			 &strip_materials );
	} else if ( obj_type == SG_TRIANGLE_FANS ) {
	    // read triangle fan properties
	    read_object( fp, SG_TRIANGLE_FANS, nproperties, nelements,
			 &fans_v, &fans_n, &fans_c, &fans_tc, &fan_materials );
	} else {
	    // unknown object type, just skip

	    // read properties
	    for ( j = 0; j < nproperties; ++j ) {
		char prop_type;
		sgReadChar( fp, &prop_type );

		sgReadUInt( fp, &nbytes );
		// cout << "property size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }

	    // read elements
	    for ( j = 0; j < nelements; ++j ) {
		sgReadUInt( fp, &nbytes );
		// cout << "element size = " << nbytes << endl;
		if ( nbytes > buf.get_size() ) { buf.resize( nbytes ); }
		char *ptr = buf.get_ptr();
		sgReadBytes( fp, nbytes, ptr );
	    }
	}
    }

    // close the file
    gzclose(fp);

    if ( sgReadError() ) {
	cout << "We detected an error while reading the file." << endl;
	return false;
    }

    return true;
}


// write out the structures to a binary file.  We assume that the
// groups come to us sorted by material property.  If not, things
// don't break, but the result won't be as optimal.
bool SGBinObject::write_bin( const string& base, const string& name,
			     const SGBucket& b )
{
    Point3D p;
    sgVec2 t;
    sgVec3 pt;
    sgVec4 color;
    int i, j;
    unsigned char idx_mask;
    int idx_size;

    SGPath file = base + "/" + b.gen_base_path() + "/" + name + ".gz";
    file.create_dir( 0755 );
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

    // write header magic
    sgWriteUInt( fp, SG_FILE_MAGIC_NUMBER );
    time_t calendar_time = time(NULL);
    sgWriteLong( fp, (int32_t)calendar_time );

    // calculate and write number of top level objects
    string material;
    int start;
    int end;
    short nobjects = 0;
    nobjects++;			// for gbs
    nobjects++;			// for vertices
    nobjects++;			// for colors
    nobjects++;			// for normals
    nobjects++;			// for texcoords

    // points
    short npts = 0;
    start = 0; end = 1;
    while ( start < (int)pt_materials.size() ) {
	material = pt_materials[start];
	while ( (end < (int)pt_materials.size()) &&
		(material == pt_materials[end]) ) {
	    end++;
	}
	npts++;
	start = end; end = start + 1;
    }
    nobjects += npts;

    // tris
    short ntris = 0;
    start = 0; end = 1;
    while ( start < (int)tri_materials.size() ) {
	material = tri_materials[start];
	while ( (end < (int)tri_materials.size()) &&
		(material == tri_materials[end]) ) {
	    end++;
	}
	ntris++;
	start = end; end = start + 1;
    }
    nobjects += ntris;

    // strips
    short nstrips = 0;
    start = 0; end = 1;
    while ( start < (int)strip_materials.size() ) {
	material = strip_materials[start];
	while ( (end < (int)strip_materials.size()) &&
		(material == strip_materials[end]) ) {
	    end++;
	}
	nstrips++;
	start = end; end = start + 1;
    }
    nobjects += nstrips;

    // fans
    short nfans = 0;
    start = 0; end = 1;
    while ( start < (int)fan_materials.size() ) {
	material = fan_materials[start];
	while ( (end < (int)fan_materials.size()) &&
		(material == fan_materials[end]) ) {
	    end++;
	}
	nfans++;
	start = end; end = start + 1;
    }
    nobjects += nfans;

    cout << "total top level objects = " << nobjects << endl;
    sgWriteShort( fp, nobjects );

    // write bounding sphere
    sgWriteChar( fp, (char)SG_BOUNDING_SPHERE );        // type
    sgWriteShort( fp, 0 );		                // nproperties
    sgWriteShort( fp, 1 );		                // nelements

    sgWriteUInt( fp, sizeof(double) * 3 + sizeof(float) ); // nbytes
    sgdVec3 center;
    sgdSetVec3( center, gbs_center.x(), gbs_center.y(), gbs_center.z() );
    sgWritedVec3( fp, center );
    sgWriteFloat( fp, gbs_radius );

    // dump vertex list
    sgWriteChar( fp, (char)SG_VERTEX_LIST );             // type
    sgWriteShort( fp, 0 );		                 // nproperties
    sgWriteShort( fp, 1 );		                 // nelements
    sgWriteUInt( fp, wgs84_nodes.size() * sizeof(float) * 3 ); // nbytes
    for ( i = 0; i < (int)wgs84_nodes.size(); ++i ) {
	p = wgs84_nodes[i] - gbs_center;
	sgSetVec3( pt, p.x(), p.y(), p.z() );
	sgWriteVec3( fp, pt );
    }

    // dump vertex color list
    sgWriteChar( fp, (char)SG_COLOR_LIST );             // type
    sgWriteShort( fp, 0 );		                 // nproperties
    sgWriteShort( fp, 1 );		                 // nelements
    sgWriteUInt( fp, colors.size() * sizeof(float) * 4 ); // nbytes
    for ( i = 0; i < (int)colors.size(); ++i ) {
	p = colors[i];
        // Right now we have a place holder for color alpha but we
        // need to update the interface so the calling program can
        // provide the info.
	sgSetVec4( color, p.x(), p.y(), p.z(), 1.0 );
	sgWriteVec4( fp, color );
    }

    // dump vertex normal list
    sgWriteChar( fp, (char)SG_NORMAL_LIST );            // type
    sgWriteShort( fp, 0 );		                // nproperties
    sgWriteShort( fp, 1 );		                // nelements
    sgWriteUInt( fp, normals.size() * 3 );              // nbytes
    char normal[3];
    for ( i = 0; i < (int)normals.size(); ++i ) {
	p = normals[i];
	normal[0] = (unsigned char)((p.x() + 1.0) * 127.5);
	normal[1] = (unsigned char)((p.y() + 1.0) * 127.5);
	normal[2] = (unsigned char)((p.z() + 1.0) * 127.5);
	sgWriteBytes( fp, 3, normal );
    }

    // dump texture coordinates
    sgWriteChar( fp, (char)SG_TEXCOORD_LIST );          // type
    sgWriteShort( fp, 0 );		                // nproperties
    sgWriteShort( fp, 1 );		                // nelements
    sgWriteUInt( fp, texcoords.size() * sizeof(float) * 2 ); // nbytes
    for ( i = 0; i < (int)texcoords.size(); ++i ) {
	p = texcoords[i];
	sgSetVec2( t, p.x(), p.y() );
	sgWriteVec2( fp, t );
    }

    // dump point groups if they exist
    if ( pts_v.size() > 0 ) {
	int start = 0;
	int end = 1;
	string material;
	while ( start < (int)pt_materials.size() ) {
	    // find next group
	    material = pt_materials[start];
	    while ( (end < (int)pt_materials.size()) && 
		    (material == pt_materials[end]) )
		{
		    // cout << "end = " << end << endl;
		    end++;
		}
	    // cout << "group = " << start << " to " << end - 1 << endl;

	    // write group headers
	    sgWriteChar( fp, (char)SG_POINTS );          // type
	    sgWriteShort( fp, 2 );		         // nproperties
	    sgWriteShort( fp, end - start );             // nelements

	    sgWriteChar( fp, (char)SG_MATERIAL );        // property
	    sgWriteUInt( fp, material.length() );        // nbytes
	    sgWriteBytes( fp, material.length(), material.c_str() );

	    idx_mask = 0;
	    idx_size = 0;
	    if ( pts_v.size() ) { idx_mask |= SG_IDX_VERTICES; ++idx_size; }
	    if ( pts_n.size() ) { idx_mask |= SG_IDX_NORMALS; ++idx_size; }
	    if ( pts_c.size() ) { idx_mask |= SG_IDX_COLORS; ++idx_size; }
	    if ( pts_tc.size() ) { idx_mask |= SG_IDX_TEXCOORDS; ++idx_size; }
	    sgWriteChar( fp, (char)SG_INDEX_TYPES );     // property
	    sgWriteUInt( fp, 1 );                        // nbytes
	    sgWriteChar( fp, idx_mask );

	    // write strips
	    for ( i = start; i < end; ++i ) {
		// nbytes
		sgWriteUInt( fp, pts_v[i].size() * idx_size * sizeof(short) );
		for ( j = 0; j < (int)pts_v[i].size(); ++j ) {
		    if ( pts_v.size() ) { 
			sgWriteShort( fp, (short)pts_v[i][j] );
		    }
		    if ( pts_n.size() ) { 
			sgWriteShort( fp, (short)pts_n[i][j] );
		    }
		    if ( pts_c.size() ) { 
			sgWriteShort( fp, (short)pts_c[i][j] );
		    }
		    if ( pts_tc.size() ) { 
			sgWriteShort( fp, (short)pts_tc[i][j] );
		    }
		}
	    }
	    
	    start = end;
	    end = start + 1;
	}
    }

    // dump individual triangles if they exist
    if ( tris_v.size() > 0 ) {
	int start = 0;
	int end = 1;
	string material;
	while ( start < (int)tri_materials.size() ) {
	    // find next group
	    material = tri_materials[start];
	    while ( (end < (int)tri_materials.size()) && 
		    (material == tri_materials[end]) &&
		    3*(end-start) < 32760 )
	    {
		// cout << "end = " << end << endl;
		end++;
	    }
	    // cout << "group = " << start << " to " << end - 1 << endl;

	    // write group headers
	    sgWriteChar( fp, (char)SG_TRIANGLE_FACES ); // type
	    sgWriteShort( fp, 2 );		        // nproperties
	    sgWriteShort( fp, 1 );                      // nelements

	    sgWriteChar( fp, (char)SG_MATERIAL );       // property
	    sgWriteUInt( fp, material.length() );        // nbytes
	    sgWriteBytes( fp, material.length(), material.c_str() );

	    idx_mask = 0;
	    idx_size = 0;
	    if ( tris_v.size() ) { idx_mask |= SG_IDX_VERTICES; ++idx_size; }
	    if ( tris_n.size() ) { idx_mask |= SG_IDX_NORMALS; ++idx_size; }
	    if ( tris_c.size() ) { idx_mask |= SG_IDX_COLORS; ++idx_size; }
	    if ( tris_tc.size() ) { idx_mask |= SG_IDX_TEXCOORDS; ++idx_size; }
	    sgWriteChar( fp, (char)SG_INDEX_TYPES );     // property
	    sgWriteUInt( fp, 1 );                        // nbytes
	    sgWriteChar( fp, idx_mask );

	    // nbytes
	    sgWriteUInt( fp, (end - start) * 3 * idx_size * sizeof(short) );

	    // write group
	    for ( i = start; i < end; ++i ) {
		for ( j = 0; j < 3; ++j ) {
		    if ( tris_v.size() ) {
			sgWriteShort( fp, (short)tris_v[i][j] );
		    }
		    if ( tris_n.size() ) {
			sgWriteShort( fp, (short)tris_n[i][j] );
		    }
		    if ( tris_c.size() ) {
			sgWriteShort( fp, (short)tris_c[i][j] );
		    }
		    if ( tris_tc.size() ) {
			sgWriteShort( fp, (short)tris_tc[i][j] );
		    }
		}
	    }

	    start = end;
	    end = start + 1;
	}
    }

    // dump triangle strips
    if ( strips_v.size() > 0 ) {
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

	    // write group headers
	    sgWriteChar( fp, (char)SG_TRIANGLE_STRIPS ); // type
	    sgWriteShort( fp, 2 );		         // nproperties
	    sgWriteShort( fp, end - start );             // nelements

	    sgWriteChar( fp, (char)SG_MATERIAL );        // property
	    sgWriteUInt( fp, material.length() );        // nbytes
	    sgWriteBytes( fp, material.length(), material.c_str() );

	    idx_mask = 0;
	    idx_size = 0;
	    if ( strips_v.size() ) { idx_mask |= SG_IDX_VERTICES; ++idx_size; }
	    if ( strips_n.size() ) { idx_mask |= SG_IDX_NORMALS; ++idx_size; }
	    if ( strips_c.size() ) { idx_mask |= SG_IDX_COLORS; ++idx_size; }
	    if ( strips_tc.size() ) { idx_mask |= SG_IDX_TEXCOORDS; ++idx_size;}
	    sgWriteChar( fp, (char)SG_INDEX_TYPES );     // property
	    sgWriteUInt( fp, 1 );                        // nbytes
	    sgWriteChar( fp, idx_mask );

	    // write strips
	    for ( i = start; i < end; ++i ) {
		// nbytes
		sgWriteUInt( fp, strips_v[i].size() * idx_size * sizeof(short));
		for ( j = 0; j < (int)strips_v[i].size(); ++j ) {
		    if ( strips_v.size() ) { 
			sgWriteShort( fp, (short)strips_v[i][j] );
		    }
		    if ( strips_n.size() ) { 
			sgWriteShort( fp, (short)strips_n[i][j] );
		    }
		    if ( strips_c.size() ) { 
			sgWriteShort( fp, (short)strips_c[i][j] );
		    }
		    if ( strips_tc.size() ) { 
			sgWriteShort( fp, (short)strips_tc[i][j] );
		    }
		}
	    }
	    
	    start = end;
	    end = start + 1;
	}
    }

    // dump triangle fans
    if ( fans_v.size() > 0 ) {
	int start = 0;
	int end = 1;
	string material;
	while ( start < (int)fan_materials.size() ) {
	    // find next group
	    material = fan_materials[start];
	    while ( (end < (int)fan_materials.size()) && 
		    (material == fan_materials[end]) )
		{
		    // cout << "end = " << end << endl;
		    end++;
		}
	    // cout << "group = " << start << " to " << end - 1 << endl;

	    // write group headers
	    sgWriteChar( fp, (char)SG_TRIANGLE_FANS );   // type
	    sgWriteShort( fp, 2 );		         // nproperties
	    sgWriteShort( fp, end - start );             // nelements

	    sgWriteChar( fp, (char)SG_MATERIAL );       // property
	    sgWriteUInt( fp, material.length() );        // nbytes
	    sgWriteBytes( fp, material.length(), material.c_str() );

	    idx_mask = 0;
	    idx_size = 0;
	    if ( fans_v.size() ) { idx_mask |= SG_IDX_VERTICES; ++idx_size; }
	    if ( fans_n.size() ) { idx_mask |= SG_IDX_NORMALS; ++idx_size; }
	    if ( fans_c.size() ) { idx_mask |= SG_IDX_COLORS; ++idx_size; }
	    if ( fans_tc.size() ) { idx_mask |= SG_IDX_TEXCOORDS; ++idx_size; }
	    sgWriteChar( fp, (char)SG_INDEX_TYPES );     // property
	    sgWriteUInt( fp, 1 );                        // nbytes
	    sgWriteChar( fp, idx_mask );

	    // write fans
	    for ( i = start; i < end; ++i ) {
		// nbytes
		sgWriteUInt( fp, fans_v[i].size() * idx_size * sizeof(short) );
		for ( j = 0; j < (int)fans_v[i].size(); ++j ) {
		    if ( fans_v.size() ) {
			sgWriteShort( fp, (short)fans_v[i][j] );
		    }
		    if ( fans_n.size() ) {
			sgWriteShort( fp, (short)fans_n[i][j] );
		    }
		    if ( fans_c.size() ) {
			sgWriteShort( fp, (short)fans_c[i][j] );
		    }
		    if ( fans_tc.size() ) {
			sgWriteShort( fp, (short)fans_tc[i][j] );
		    }
		}
	    }
	    
	    start = end;
	    end = start + 1;
	}
    }

    // close the file
    gzclose(fp);

    if ( sgWriteError() ) {
	cout << "We detected an error while writing the file." << endl;
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
    Point3D p;
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
	p = wgs84_nodes[i] - gbs_center;
	
	fprintf(fp,  "v %.5f %.5f %.5f\n", p.x(), p.y(), p.z() );
    }
    fprintf(fp, "\n");

    fprintf(fp, "# vertex normal list\n");
    for ( i = 0; i < (int)normals.size(); ++i ) {
	p = normals[i];
	fprintf(fp,  "vn %.5f %.5f %.5f\n", p.x(), p.y(), p.z() );
    }
    fprintf(fp, "\n");

    // dump texture coordinates
    fprintf(fp, "# texture coordinate list\n");
    for ( i = 0; i < (int)texcoords.size(); ++i ) {
	p = texcoords[i];
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

	    // make a list of points for the group
	    point_list group_nodes;
	    group_nodes.clear();
	    Point3D bs_center;
	    double bs_radius = 0;
	    for ( i = start; i < end; ++i ) {
		for ( j = 0; j < (int)tris_v[i].size(); ++j ) {
		    group_nodes.push_back( wgs84_nodes[ tris_v[i][j] ] );
		    bs_center = sgCalcCenter( group_nodes );
		    bs_radius = sgCalcBoundingRadius( bs_center, group_nodes );
		}
	    }

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

	    // make a list of points for the group
	    point_list group_nodes;
	    group_nodes.clear();
	    Point3D bs_center;
	    double bs_radius = 0;
	    for ( i = start; i < end; ++i ) {
		for ( j = 0; j < (int)strips_v[i].size(); ++j ) {
		    group_nodes.push_back( wgs84_nodes[ strips_v[i][j] ] );
		    bs_center = sgCalcCenter( group_nodes );
		    bs_radius = sgCalcBoundingRadius( bs_center, group_nodes );
		}
	    }

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
    system(command.c_str());

    return true;
}
