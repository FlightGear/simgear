/*
Szymon Rusinkiewicz
Princeton University

TriMesh_normals.cc
Compute per-vertex normals for TriMeshes

For meshes, uses average of per-face normals, weighted according to:
  Max, N.
  "Weights for Computing Vertex Normals from Facet Normals,"
  Journal of Graphics Tools, Vol. 4, No. 2, 1999.

For raw point clouds, fits plane to k nearest neighbors.
*/

#include <simgear/scene/tgdb/dem.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/debug/logstream.hxx>

#include "SGMesh.hxx"
// #include "KDtree.h"
// #include "lineqn.h"
using namespace std;

SGMesh::SGMesh( const SGDemPtr dem,
        const SGGeod& sw, const SGGeod& se,
        const SGGeod& nw, const SGGeod& ne,
        unsigned x0, unsigned y0,
        double incu, double incv,
        int mw, int mh,
        const osg::Matrixd& transform )
{
    flag_curr = 0;
    grid_width  = mw;
    grid_height = mh;

    double inci = ( se.getLongitudeDeg() - sw.getLongitudeDeg() ) / (mw - 1);
    double incj = ( nw.getLatitudeDeg()  - sw.getLatitudeDeg() )  / (mh - 1);

    // populate vertices and texCoords
    vertices  = new osg::Vec3Array( mw*mh );
    normals   = NULL;
    texCoords = new osg::Vec2Array( mw*mh );

    // session can't be paralell yet - save alts in geode array
    SGDemSession s = dem->openSession( sw, ne, true );
    ::std::vector<SGGeod> geodes( mw*mh );
    for ( int di = 0; di < mw; di++ ) {
        for ( int dj = 0; dj < mh; dj++ ) {
            SGGeod pos = SGGeod::fromDeg(sw.getLongitudeDeg() + (di+0)*inci, sw.getLatitudeDeg() + (dj+0)*incj );
            pos.setElevationM( (double)dem->getAlt(s, pos) );
            geodes[di*mh+dj] = pos;

        }
    }
    s.close();

    // we can convert to cartesian in paralell
    unsigned int nv = geodes.size();
#pragma omp parallel for
    for (unsigned int i = 0; i < nv; i++) {
        (*vertices)[i].set( toOsg( SGVec3f::fromGeod( geodes[i] ) ) );
    }

    need_faces();
    need_normals();

    // translate pos after normals computed?
#pragma omp parallel for
    for ( unsigned int i=0; i < nv; i++ ) {
        (*vertices)[i].set( transform.preMult( (*vertices)[i]) );
    }

    for ( int di = 0; di < mw; di++ ) {
        for ( int dj = 0; dj < mh; dj++ ) {
            (*texCoords)[di*mh+dj].set( toOsg(SGVec2f(x0*1.0/(360*8) + di*incu, y0*1.0/(180*8) + dj*incv)) );
        }
    }
}

void SGMesh::need_faces()
{
    for (int i = 0; i < grid_width-1; i++) {
        for (int j = 0; j < grid_height-1; j++) {
            // for each point - create 2 triangles with current point in the 
            // lower left corner 

            // 0,0 - 1,0 - 0,1
            faces.push_back( Face( (i+0)*grid_height + (j+0), 
                                   (i+1)*grid_height + (j+0), 
                                   (i+0)*grid_height + (j+1) ) );

            // 1,1 - 0,1 - 1,0
            faces.push_back( Face( (i+1)*grid_height + (j+1), 
                                   (i+0)*grid_height + (j+1), 
                                   (i+1)*grid_height + (j+0) ) );
        }
    }
}

// Compute per-vertex normals
void SGMesh::need_normals()
{
    if ( normals == NULL )
    {
        normals = new osg::Vec3Array( vertices->size() );

        // need_faces();
        if ( !faces.empty() ) {
            // Compute from faces
            int nf = faces.size();
#pragma omp parallel for
            for (int i = 0; i < nf; i++) {
                const osg::Vec3 &p0 = (*vertices)[faces[i][0]];
                const osg::Vec3 &p1 = (*vertices)[faces[i][1]];
                const osg::Vec3 &p2 = (*vertices)[faces[i][2]];

                osg::Vec3 a = p0-p1, b = p1-p2, c = p2-p0;
                float l2a = a.length2(), l2b = b.length2(), l2c = c.length2();
                if (!l2a || !l2b || !l2c) {
                    continue;
                }
                osg::Vec3 facenormal = a ^ b;
                (*normals)[faces[i][0]] += facenormal * (1.0f / (l2a * l2c));
                (*normals)[faces[i][1]] += facenormal * (1.0f / (l2b * l2a));
                (*normals)[faces[i][2]] += facenormal * (1.0f / (l2c * l2b));
            }
        }

        // Make them all unit-length
        unsigned int nn = normals->size();
#pragma omp parallel for
        for (unsigned int i = 0; i < nn; i++)
            (*normals)[i].normalize();
    }
}
