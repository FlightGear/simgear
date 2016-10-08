// SGMesh.hxx -- mesh normals and curvature from a regular grid
//
// Copyright (C) 2016  Peter Sadrozinski
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
//

#ifndef _SGMESH_HXX
#define _SGMESH_HXX

/*
The Simgear Mesh object is based on the GPLv2 library trimesh by

Szymon Rusinkiewicz
Princeton University

TriMesh.h
Class for triangle meshes.

from http://gfx.cs.princeton.edu/gfx/proj/trimesh2

Based on a paper: http://gfx.cs.princeton.edu/pubs/_2004_ECA/curvpaper.pdf

This class has been reduced in scope, and the native types have been 
converted to OpenSceneGraph for use in SimGear/FlightGear

*/

#include <osg/Geometry>
#include <osg/Geode>

#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/math/SGMath.hxx>

class KDtree {
private:
	class Node;
	Node *root;
	void build(const float *ptlist, size_t n);

public:
	// Compatibility function for closest-compatible-point searches
	struct CompatFunc
	{
		virtual bool operator () (const float *p) const = 0;
		virtual ~CompatFunc() {}  // To make the compiler shut up
	};

	// Constructor from an array of points
	KDtree(const float *ptlist, size_t n)
		{ build(ptlist, n); }

	// Constructor from a vector of points
	template <class T> KDtree(const ::std::vector<T> &v)
		{ build((const float *) &v[0], v.size()); }

	// Destructor - recursively frees the tree
	~KDtree();

	// The queries: returns closest point to a point or a ray,
	// provided it's within sqrt(maxdist2) and is compatible
	const float *closest_to_pt(const float *p,
				   float maxdist2 = 0.0f,
				   const CompatFunc *iscompat = NULL) const;
	const float *closest_to_ray(const float *p, const float *dir,
				    float maxdist2 = 0.0f,
				    const CompatFunc *iscompat = NULL) const;

	// Find the k nearest neighbors
	void find_k_closest_to_pt(::std::vector<const float *> &knn,
				  int k,
				  const float *p,
				  float maxdist2 = 0.0f,
				  const CompatFunc *iscompat = NULL) const;
};

// Windows defines min and max as macros, which prevents us from
// using the type-safe versions from std::
// Also define NOMINMAX, which prevents future bad definitions.
#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif
#ifndef NOMINMAX
# define NOMINMAX
#endif
#ifndef _USE_MATH_DEFINES
 #define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <algorithm>
#include <limits>

// LU decomposition
template <class T, int N>
static inline bool ludcmp(T a[N][N], int indx[N], T *d = NULL)
{
	using namespace ::std;
	T vv[N];

	if (d)
		*d = T(1);
	for (int i = 0; i < N; i++) {
		T big = T(0);
		for (int j = 0; j < N; j++) {
			T tmp = fabs(a[i][j]);
			if (tmp > big)
				big = tmp;
		}
		if (big == T(0))
			return false;
		vv[i] = T(1) / big;
	}
	for (int j = 0; j < N; j++) {
		for (int i = 0; i < j; i++) {
			T sum = a[i][j];
			for (int k = 0; k < i; k++)
				sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
		}
		T big = T(0);
		int imax = j;
		for (int i = j; i < N; i++) {
			T sum = a[i][j];
			for (int k = 0; k < j; k++)
				sum -= a[i][k]*a[k][j];
			a[i][j] = sum;
			T tmp = vv[i] * fabs(sum);
			if (tmp > big) {
				big = tmp;
				imax = i;
			}
		}
		if (imax != j) {
			for (int k = 0; k < N; k++)
				swap(a[imax][k], a[j][k]);
			if (d)
				*d = -(*d);
			vv[imax] = vv[j];
		}
		indx[j] = imax;
		if (unlikely(a[j][j] == T(0)))
			return false;
		if (j != N-1) {
			T tmp = T(1) / a[j][j];
			for (int i = j+1; i < N; i++)
				a[i][j] *= tmp;
		}
	}
	return true;
}


// Backsubstitution after ludcmp
template <class T, int N>
static inline void lubksb(T a[N][N], int indx[N], T b[N])
{
	int ii = -1;
	for (int i = 0; i < N; i++) {
		int ip = indx[i];
		T sum = b[ip];
		b[ip] = b[i];
		if (ii != -1)
			for (int j = ii; j < i; j++)
				sum -= a[i][j] * b[j];
		else if (sum)
			ii = i;
		b[i] = sum;
	}
	for (int i = N-1; i >= 0; i--) {
		T sum = b[i];
		for (int j = i+1; j < N; j++)
			sum -= a[i][j] * b[j];
		b[i] = sum / a[i][i];
	}
}


// Perform LDL^T decomposition of a symmetric positive definite matrix.
// Like Cholesky, but no square roots.  Overwrites lower triangle of matrix.
template <class T, int N>
static inline bool ldltdc(T A[N][N], T rdiag[N])
{
	T v[N-1];
	for (int i = 0; i < N; i++) {
		for (int k = 0; k < i; k++)
			v[k] = A[i][k] * rdiag[k];
		for (int j = i; j < N; j++) {
			T sum = A[i][j];
			for (int k = 0; k < i; k++)
				sum -= v[k] * A[j][k];
			if (i == j) {
				if (unlikely(sum <= T(0)))
					return false;
				rdiag[i] = T(1) / sum;
			} else {
				A[j][i] = sum;
			}
		}
	}

	return true;
}


// Solve Ax=B after ldltdc
template <class T, int N>
static inline void ldltsl(T A[N][N], T rdiag[N], T B[N], T x[N])
{
	for (int i = 0; i < N; i++) {
		T sum = B[i];
		for (int k = 0; k < i; k++)
			sum -= A[i][k] * x[k];
		x[i] = sum * rdiag[i];
	}
	for (int i = N - 1; i >= 0; i--) {
		T sum = 0;
		for (int k = i + 1; k < N; k++)
			sum += A[k][i] * x[k];
		x[i] -= sum * rdiag[i];
	}
}


// Eigenvector decomposition for real, symmetric matrices,
// a la Bowdler et al. / EISPACK / JAMA
// Entries of d are eigenvalues, sorted smallest to largest.
// A changed in-place to have its columns hold the corresponding eigenvectors.
// Note that A must be completely filled in on input.
template <class T, int N>
static inline void eigdc(T A[N][N], T d[N])
{
	using namespace ::std;

	// Householder
	T e[N];
	for (int j = 0; j < N; j++) {
		d[j] = A[N-1][j];
		e[j] = T(0);
	}
	for (int i = N-1; i > 0; i--) {
		T scale = T(0);
		for (int k = 0; k < i; k++)
			scale += fabs(d[k]);
		if (scale == T(0)) {
			e[i] = d[i-1];
			for (int j = 0; j < i; j++) {
				d[j] = A[i-1][j];
				A[i][j] = A[j][i] = T(0);
			}
			d[i] = T(0);
		} else {
			T h(0);
			T invscale = T(1) / scale;
			for (int k = 0; k < i; k++) {
				d[k] *= invscale;
				h += sqr(d[k]);
			}
			T f = d[i-1];
			T g = (f > T(0)) ? -sqrt(h) : sqrt(h);
			e[i] = scale * g;
			h -= f * g;
			d[i-1] = f - g;
			for (int j = 0; j < i; j++)
				e[j] = T(0);
			for (int j = 0; j < i; j++) {
				f = d[j];
				A[j][i] = f;
				g = e[j] + f * A[j][j];
				for (int k = j+1; k < i; k++) {
					g += A[k][j] * d[k];
					e[k] += A[k][j] * f;
				}
				e[j] = g;
			}
			f = T(0);
			T invh = T(1) / h;
			for (int j = 0; j < i; j++) {
				e[j] *= invh;
				f += e[j] * d[j];
			}
			T hh = f / (h + h);
			for (int j = 0; j < i; j++)
				e[j] -= hh * d[j];
			for (int j = 0; j < i; j++) {
				f = d[j];
				g = e[j];
				for (int k = j; k < i; k++)
					A[k][j] -= f * e[k] + g * d[k];
				d[j] = A[i-1][j];
				A[i][j] = T(0);
			}
			d[i] = h;
		}
	}

	for (int i = 0; i < N-1; i++) {
		A[N-1][i] = A[i][i];
		A[i][i] = T(1);
		T h = d[i+1];
		if (h != T(0)) {
			T invh = T(1) / h;
			for (int k = 0; k <= i; k++)
				d[k] = A[k][i+1] * invh;
			for (int j = 0; j <= i; j++) {
				T g = T(0);
				for (int k = 0; k <= i; k++)
					g += A[k][i+1] * A[k][j];
				for (int k = 0; k <= i; k++)
					A[k][j] -= g * d[k];
			}
		}
		for (int k = 0; k <= i; k++)
			A[k][i+1] = T(0);
	}
	for (int j = 0; j < N; j++) {
		d[j] = A[N-1][j];
		A[N-1][j] = T(0);
	}
	A[N-1][N-1] = T(1);

	// QL
	for (int i = 1; i < N; i++)
		e[i-1] = e[i];
	e[N-1] = T(0);
	T f = T(0), tmp = T(0);
	const T eps = ::std::numeric_limits<T>::epsilon();
	for (int l = 0; l < N; l++) {
		tmp = max(tmp, fabs(d[l]) + fabs(e[l]));
		int m = l;
		while (m < N) {
			if (fabs(e[m]) <= eps * tmp)
				break;
			m++;
		}
		if (m > l) {
			do {
				T g = d[l];
				T p = (d[l+1] - g) / (e[l] + e[l]);
				T r = T(hypot(p, T(1)));
				if (p < T(0))
					r = -r;
				d[l] = e[l] / (p + r);
				d[l+1] = e[l] * (p + r);
				T dl1 = d[l+1];
				T h = g - d[l];
				for (int i = l+2; i < N; i++)
					d[i] -= h; 
				f += h;
				p = d[m];   
				T c = T(1), c2 = T(1), c3 = T(1);
				T el1 = e[l+1], s = T(0), s2 = T(0);
				for (int i = m - 1; i >= l; i--) {
					c3 = c2;
					c2 = c;
					s2 = s;
					g = c * e[i];
					h = c * p;
					r = T(hypot(p, e[i]));
					e[i+1] = s * r;
					s = e[i] / r;
					c = p / r;
					p = c * d[i] - s * g;
					d[i+1] = h + s * (c * g + s * d[i]);
					for (int k = 0; k < N; k++) {
						h = A[k][i+1];
						A[k][i+1] = s * A[k][i] + c * h;
						A[k][i] = c * A[k][i] - s * h;
					}
				}
				p = -s * s2 * c3 * el1 * e[l] / dl1;
				e[l] = s * p;
				d[l] = c * p;
			} while (fabs(e[l]) > eps * tmp);
		}
		d[l] += f;
		e[l] = T(0);
	}

	// Sort
	for (int i = 0; i < N-1; i++) {
		int k = i;
		T p = d[i];
		for (int j = i+1; j < N; j++) {
			if (d[j] < p) {
				k = j;
				p = d[j];
			}
		}
		if (k == i)
			continue;
		d[k] = d[i];
		d[i] = p;
		for (int j = 0; j < N; j++)
			swap(A[j][i], A[j][k]);
	}
}


// x <- A * d * A' * b
template <class T, int N>
static inline void eigmult(T A[N][N],
			   T d[N],
			   T b[N],
			   T x[N])
{
	T e[N];
	for (int i = 0; i < N; i++) {
		e[i] = T(0);
		for (int j = 0; j < N; j++)
			e[i] += A[j][i] * b[j];
		e[i] *= d[i];
	}
	for (int i = 0; i < N; i++) {
		x[i] = T(0);
		for (int j = 0; j < N; j++)
			x[i] += A[i][j] * e[j];
	}
}

class SGMesh {
public:
	//
	// Types
	//
    typedef enum {
        TEXTURE_RASTER,
        TEXTURE_BLUEMARBLE,
        TEXTURE_DEBUG,
    } TextureMethod;

    struct Face {
		unsigned int v[3];

		Face() {}
		Face(const int &v0, const int &v1, const int &v2)
			{ v[0] = v0; v[1] = v1; v[2] = v2; }
		Face(const int *v_)
			{ v[0] = v_[0]; v[1] = v_[1]; v[2] = v_[2]; }
//		template <class S> explicit Face(const S &x)
//			{ v[0] = x[0];  v[1] = x[1];  v[2] = x[2]; }
		unsigned int &operator[] (int i) { return v[i]; }
		const unsigned int &operator[] (int i) const { return v[i]; }
		operator const unsigned int * () const { return &(v[0]); }
		operator const unsigned int * () { return &(v[0]); }
		operator unsigned int * () { return &(v[0]); }
		int indexof(unsigned int v_) const
		{
			return (v[0] == v_) ? 0 :
			       (v[1] == v_) ? 1 :
			       (v[2] == v_) ? 2 : -1;
		}
	};

#if 0
	struct BSphere {
		point center;
		float r;
		bool valid;
		BSphere() : valid(false)
			{}
	};

	//
	// Enums
	//
	enum TstripRep { TSTRIP_LENGTH, TSTRIP_TERM };
	enum { GRID_INVALID = -1 };
	enum StatOp { STAT_MIN, STAT_MAX, STAT_MEAN, STAT_MEANABS,
		STAT_RMS, STAT_MEDIAN, STAT_STDEV, STAT_TOTAL };
	enum StatVal { STAT_VALENCE, STAT_FACEAREA, STAT_ANGLE,
		STAT_DIHEDRAL, STAT_EDGELEN, STAT_X, STAT_Y, STAT_Z };
#endif
        
	//
	// Constructor
	//
	SGMesh() : flag_curr(0)
		{}

    SGMesh( const SGDemPtr dem,
            unsigned wo, unsigned so,
            unsigned eo, unsigned no,
            unsigned heightLevel,
            unsigned widthLevel,
            const osg::Matrixd& transform,
            TextureMethod tm,
            const osgDB::Options* options
          );

    //
    // Members
    //
    osg::Vec3Array* getVertices(void) {
        return vertices;
    }

    osg::Vec3Array* getNormals(void) {
        return normals;
    }

    osg::Vec2Array* getTexCoords(void) {
        return texCoords;
    }

    osg::Geode* getGeode(void) {
        return geode;
    }

    osg::DrawElementsUInt* getIndices(void);

    // The basics: vertices and faces
    // note:
    // vertices, normals, texcoords, etc are allocated by the mesh,
    // but are inserted into the scene graph.  They are smart pointers
    osg::Vec3Array*        vertices;
	osg::Vec3Array*        normals;
    osg::Vec2Array*        texCoords;
    ::std::vector<Face>    faces;

//	// Triangle strips
//	::std::vector<int> tstrips;

	// Grid, if present
//	::std::vector<int> grid;
    unsigned int grid_width, grid_height;

	// Other per-vertex properties
//	::std::vector<Color> colors;
	::std::vector<float> confidences;
	::std::vector<unsigned> flags;
	unsigned flag_curr;

	// Computed per-vertex properties
	::std::vector<osg::Vec3> pdir1, pdir2;
	::std::vector<float> curv1, curv2;
	::std::vector<osg::Vec4> dcurv;
	::std::vector<osg::Vec3> cornerareas;
	::std::vector<float> pointareas;

    osg::Geode*         geode;

	// Bounding structures
	//box bbox;
	//BSphere bsphere;

	// Connectivity structures:
	//  For each vertex, all neighboring vertices
	::std::vector< ::std::vector<unsigned int> > neighbors;
	//  For each vertex, all neighboring faces
	::std::vector< ::std::vector<unsigned int> > adjacentfaces;
	//  For each face, the three faces attached to its edges
	//  (for example, across_edge[3][2] is the number of the face
	//   that's touching the edge opposite vertex 2 of face 3)
	::std::vector<Face> across_edge;

    inline osg::Vec3 trinorm(const osg::Vec3 &v0, const osg::Vec3 &v1, const osg::Vec3 &v2)
    {
        return  ( (v1 - v0) ^ (v2 - v0) ) * 0.5;
    }

    static inline const float angle(const osg::Vec3 &v1, const osg::Vec3 &v2)
    {
        return std::atan2( (v1 ^ v2).length(), (v1 * v2) );
    }

	//
	// Compute all this stuff...
	//
//	void need_tstrips();
//	void convert_strips(TstripRep rep);
//	void unpack_tstrips();
//	void triangulate_grid(bool remove_slivers = true);
	void need_faces();
//	{
//		if (!faces.empty())
//			return;
//		if (!tstrips.empty())
//			unpack_tstrips();
//		else if (!grid.empty())
//			triangulate_grid();
//	}
    
	void need_normals();
	void need_pointareas();
	void need_curvatures();
	void need_dcurv();
//	void need_bbox();
//	void need_bsphere();
	void need_neighbors();
	void need_adjacentfaces();
	void need_across_edge();

	//
	// Delete everything
	//
	void clear()
	{
        vertices = NULL;
        faces.clear(); 
// tstrips.clear();
//  	grid.clear(); 
        grid_width = grid_height = 0;
//		colors.clear(); 
        confidences.clear();
		flags.clear(); flag_curr = 0;
		normals = NULL; pdir1.clear(); pdir2.clear();
		curv1.clear(); curv2.clear(); dcurv.clear();
		cornerareas.clear(); pointareas.clear();
//		bbox.valid = bsphere.valid = false;
		neighbors.clear(); adjacentfaces.clear(); across_edge.clear();
        texCoords = NULL;
	}

	//
	// Input and output
	//
protected:
	// static bool read_helper(const char *filename, TriMesh *mesh);
public:
	// static TriMesh *read(const char *filename);
	// static TriMesh *read(const ::std::string &filename);
	// bool write(const char *filename);
	// bool write(const ::std::string &filename);


	//
	// Useful queries
	//

	// Is vertex v on the mesh boundary?
	bool is_bdy(int v)
	{
		if (neighbors.empty()) need_neighbors();
		if (adjacentfaces.empty()) need_adjacentfaces();
		return neighbors[v].size() != adjacentfaces[v].size();
	}

	// Centroid of face f
	osg::Vec3 centroid(int f)
	{
		if (faces.empty()) need_faces();
		return ( (*vertices)[faces[f][0]] + (*vertices)[faces[f][1]] + (*vertices)[faces[f][2]] ) * (1.0f / 3.0f);
	}

	// Normal of face f
	osg::Vec3 trinorm(int f)
	{
		if (faces.empty()) need_faces();
		return trinorm( (*vertices)[faces[f][0]], (*vertices)[faces[f][1]], (*vertices)[faces[f][2]]);
	}

	// Angle of corner j in triangle
	float cornerangle(int i, int j)
	{
		using namespace ::std;

		if (faces.empty()) need_faces();
		const osg::Vec3 &p0 = (*vertices)[faces[i][j]];
		const osg::Vec3 &p1 = (*vertices)[faces[i][(j+1)%3]];
		const osg::Vec3 &p2 = (*vertices)[faces[i][(j+2)%3]];
		
        return acos( (p1 - p0) * (p2 - p0) );
	}

	// Dihedral angle between face i and face across_edge[i][j]
	float dihedral(int i, int j)
	{
		if (across_edge.empty()) need_across_edge();
		if (across_edge[i][j] < 0) return 0.0f;
		osg::Vec3 mynorm = trinorm(i);
		osg::Vec3 othernorm = trinorm(across_edge[i][j]);
		float ang = angle(mynorm, othernorm);
		osg::Vec3 towards = ( ((*vertices)[faces[i][(j+1)%3]] + (*vertices)[faces[i][(j+2)%3]]) - (*vertices)[faces[i][j]] ) * 0.5f;
		if ( (towards * othernorm) < 0.0f)
			return SG_PI + ang;
		else
			return SG_PI - ang;
	}

	// Statistics
//	float stat(StatOp op, StatVal val);
//	float feature_size();

	//
	// Debugging
	//

	// Debugging printout, controllable by a "verbose"ness parameter
//	static int verbose;
//	static void set_verbose(int);
//	static void (*dprintf_hook)(const char *);
//	static void set_dprintf_hook(void (*hook)(const char *));
//	static void dprintf(const char *format, ...);

	// Same as above, but fatal-error printout
//	static void (*eprintf_hook)(const char *);
//	static void set_eprintf_hook(void (*hook)(const char *));
//	static void eprintf(const char *format, ...);
};

#endif
