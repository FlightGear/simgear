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

#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/CullFace>
#include <osg/Texture2D>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/debug/logstream.hxx>

#include "SGDemSession.hxx"
#include "SGMesh.hxx"

#define GW_LVL1          (153)
#define SKIP_MULT_LVL1   (1)

#define GW_LVL2          (78)
#define SKIP_MULT_LVL2   (2)

#define GW_LVL3          (53)
#define SKIP_MULT_LVL3   (3)

#define GW_LVL4          (33)
#define SKIP_MULT_LVL4   (5)

#define GW_LVL5          (18)
#define SKIP_MULT_LVL5   (10)

#define GW_LVL6          (13)
#define SKIP_MULT_LVL6   (15)

#define GW_LVL7          (9)
#define SKIP_MULT_LVL7   (25)

#define GW_TEST          GW_LVL2
#define SKIP_TEST        SKIP_MULT_LVL2


using namespace std;

SGMesh::SGMesh( const SGDemPtr dem,
                unsigned wo, unsigned so,
                unsigned eo, unsigned no,
                unsigned heightLevel,
                unsigned widthLevel,
                const osg::Matrixd& transform,
                TextureMethod tm,
                const osgDB::Options* options )
{
    ::std::vector<SGGeod>       skirt_geods;
    ::std::vector<unsigned int> index;

    flag_curr = 0;

    int lvl = -1;
    int skipx = -1, skipy = -1;

    bool Debug1 = false;
    bool Debug2 = false;

    if ( wo == 1444 && eo == 1446 && so == 1055 && no == 1056 ) {
        Debug1 = true;
    } else if ( wo == 1444 && eo == 1446 && so == 1056 && no == 1057 ) {
        Debug2 = true;
    }

    switch ( heightLevel ) {
        case 9: lvl = 0; grid_height = 153; skipy = 1;
            // at highest lod for height, tiles have different widths based on latitude
            switch( widthLevel ) {
                case 9: grid_width = 153; skipx = 1;  break; // 1/8 x 1/8
                case 8: grid_width = 153; skipx = 2;  break; // 1/4 x 1/8
                case 7: grid_width = 153; skipx = 4;  break; // 1/2 x 1/8
                case 6: grid_width = 153; skipx = 8;  break; //   1 x 1/8
                case 5: grid_width = 153; skipx = 16; break; //   2 x 1/8
                case 4: grid_width = 153; skipx = 32; break; //   4 x 1/8
                case 3: grid_width =  78; skipx = 32; break; //  12 x 1/8
            }
            break;

        case 8: lvl = 0; grid_height = GW_TEST; skipy = 2*SKIP_TEST; grid_width = GW_TEST; skipx = 2*SKIP_TEST; break; // 1/4 x 1/4
        case 7: lvl = 0; grid_height = GW_TEST; skipy = 4*SKIP_TEST; grid_width = GW_TEST; skipx = 4*SKIP_TEST; break; // 1/2 x 1/2
        case 6: lvl = 0; grid_height = GW_TEST; skipy = 8*SKIP_TEST; grid_width = GW_TEST; skipx = 8*SKIP_TEST; break; // 1   x 1

        case 5: lvl = 1; grid_height = GW_TEST; skipy = 1*SKIP_TEST; grid_width = GW_TEST; skipx = 1*SKIP_TEST; break; //  2 x 2
        case 4: lvl = 1; grid_height = GW_TEST; skipy = 2*SKIP_TEST; grid_width = GW_TEST; skipx = 2*SKIP_TEST; break; //  4 x 4
        case 3: lvl = 1; grid_height = GW_TEST; skipy = 6*SKIP_TEST;
            switch ( widthLevel ) {
                case 3: grid_width = GW_TEST; skipx =  6*SKIP_TEST; break;  // 12 x 12
                case 2: grid_width = GW_TEST; skipx = 18*SKIP_TEST; break;  // 36 x 12
            }
            break;

        case 2: lvl = 2; grid_height = GW_TEST; skipy = 3*SKIP_TEST; grid_width = GW_TEST; skipx = 1*SKIP_TEST; break; // 180 x 60
        case 1: lvl = 2; grid_height = GW_TEST; skipy = 6*SKIP_TEST; grid_width = GW_TEST; skipx = 3*SKIP_TEST; break; // 360 x 180

        case 0: printf("ERROR - no height level 0\n"); exit(0); break;
    }

    double incu = ( eo*1.0/(360*8) - wo*1.0/(360*8) ) / (grid_width  - 3);
    double incv = ( no*1.0/(180*8) - so*1.0/(180*8) ) / (grid_height - 3);

    // try to use native mesh res 
    vertices  = new osg::Vec3Array( grid_width*grid_height );
    normals   = NULL;
    texCoords = new osg::Vec2Array( grid_width*grid_height );

    ::std::vector<SGGeod> geodes(grid_width*grid_height);

    // session can't be parallel yet - save alts in geode array
    fprintf( stderr, "SGMesh::SGMesh - create session - num dem roots is %d\n", dem->getNumRoots() );
    SGDemSession s = dem->openSession( wo, so, eo, no, lvl, true );
    s.getGeods( wo, so, eo, no, grid_width, grid_height, skipx, skipy, geodes, Debug1, Debug2 );
    s.close();

    // save the west skirt vertices
    unsigned int src_idx  = 0;
    unsigned int edge_idx = 0;

    // save west skirt
    for ( edge_idx = 0, src_idx = 0; edge_idx < grid_height; edge_idx++, src_idx++ ) {
        skirt_geods.push_back( geodes[src_idx] );
        index.push_back( src_idx );
    }

    // save the north skirt vertices
    for ( edge_idx = 1, src_idx = (grid_height*2)-1; edge_idx < grid_width; edge_idx++, src_idx += grid_height ) {
        skirt_geods.push_back( geodes[src_idx] );
        index.push_back( src_idx );
    }

    // save the east skirt vertices
    for ( edge_idx = 0, src_idx = grid_height*(grid_width-1); edge_idx < grid_height-1; edge_idx++, src_idx++ ) {
        skirt_geods.push_back( geodes[src_idx] );
        index.push_back( src_idx );
    }

    // save the south skirt vertices
    for ( edge_idx = 1, src_idx = grid_height; edge_idx < grid_width-1; src_idx += grid_height, edge_idx++ ) {
        skirt_geods.push_back( geodes[src_idx] );
        index.push_back( src_idx );
    }

    // we can convert to cartesian in parallel
    long nv = geodes.size();
#pragma omp parallel for
    for (long i = 0; i < nv; i++) {
        (*vertices)[i].set( toOsg( SGVec3f::fromGeod( geodes[i] ) ) );
    }

    need_faces();
    need_normals();

    // lower skirt verts based on lvl and skip
    unsigned int lower = 0;
    switch( lvl ) {
        case 0:
            lower = skipx*100;
            break;

        case 1:
            lower = skipx*10000;
            break;

        case 2:
            lower = skipx*1000000;
            break;
    }

    for ( unsigned int i=0; i<skirt_geods.size(); i++ ) {
        skirt_geods[i].setElevationM( skirt_geods[i].getElevationM() - lower );
    }

    for ( unsigned int i=0; i<index.size(); i++ ) {
        (*vertices)[index[i]].set( toOsg( SGVec3f::fromGeod( skirt_geods[i] ) ) );
    }

    // translate pos after normals computed
#pragma omp parallel for
    for ( long i=0; i < nv; i++ ) {
        (*vertices)[i].set( transform.preMult( (*vertices)[i]) );
    }

    float startu = SGMiscd::normalizePeriodic( 0.0, 1.0, wo*1.0/(360*8) - incu);
    float startv = SGMiscd::normalizePeriodic( 0.0, 1.0, so*1.0/(180*8) - incv);

    for ( unsigned int di = 0; di < grid_width; di++ ) {
        for ( unsigned int dj = 0; dj < grid_height; dj++ ) {
            (*texCoords)[di*grid_height+dj].set( toOsg(SGVec2f(startu + di*incu, startv + dj*incv)) );
        }
    }

    osg::Vec4Array* colors = new osg::Vec4Array;
    if ( tm == SGMesh::TEXTURE_DEBUG ) {
        osg::Vec4 lvlColor;
        switch( heightLevel ) {
            case 9: lvlColor = osg::Vec4(0.5, 0.7, 0.9, 1); break;
            case 8: lvlColor = osg::Vec4(1, 1, 0, 1); break;
            case 7: lvlColor = osg::Vec4(0, 1, 0, 1); break;
            case 6: lvlColor = osg::Vec4(0, 1, 1, 1); break;

            case 5: lvlColor = osg::Vec4(0, 0, 1, 1); break;
            case 4: lvlColor = osg::Vec4(1, 1, 1, 1); break;
            case 3: lvlColor = osg::Vec4(1, 0, 1, 1); break;

            case 2: lvlColor = osg::Vec4(0.5, 0.5, 0.5, 1); break;
            case 1: lvlColor = osg::Vec4(0.5, 0, 0.5, 1); break;
        }

        // special colors - green, and ref
        if ( Debug1 ) {
            lvlColor = osg::Vec4(0.0, 0.7, 0.0, 1);
        } else if ( Debug2 ) {
            lvlColor = osg::Vec4(0.7, 0.0, 0.0, 1);
        }

        for ( unsigned int v=0; v<getVertices()->size(); v++ ) {
            colors->push_back(lvlColor);
        }
    } else if ( tm == SGMesh::TEXTURE_BLUEMARBLE ) {
        colors->push_back(osg::Vec4(1, 1, 1, 1));
    } else if ( tm == SGMesh::TEXTURE_RASTER ) {
        // TODO
    }

    geode = new osg::Geode;
    osg::Geometry* geometry = new osg::Geometry;

    char geoName[64];
    snprintf( geoName, sizeof(geoName), "tilemesh (%u,%u)-(%u,%u):level%u,%u", 
                wo, so, eo, no,
                widthLevel, heightLevel );
    geometry->setName(geoName);

    geometry->setDataVariance(osg::Object::STATIC);
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexArray( getVertices() );

    geometry->setNormalArray( getNormals() );
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(colors);

    if ( tm == SGMesh::TEXTURE_DEBUG ) {
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    } else {
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        geometry->setTexCoordArray(0, getTexCoords() );
    }

    // generate triangles from the grid
    osg::DrawElementsUInt* indices = getIndices();
    geometry->addPrimitiveSet(indices);

    if ( geometry ) {
        geode->setDataVariance(osg::Object::STATIC);
        geode->addDrawable(geometry);

        if ( tm == SGMesh::TEXTURE_DEBUG ) {
            // set up the state
            osg::StateSet* stateSet = geode->getOrCreateStateSet();
            stateSet->setRenderBinDetails(-10, "RenderBin");

            osg::ShadeModel* shadeModel = new osg::ShadeModel;
            shadeModel->setMode(osg::ShadeModel::SMOOTH);
            stateSet->setAttributeAndModes(shadeModel);

            stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
            stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
            stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
            stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
            stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
            stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

            stateSet->setAttribute(new osg::CullFace(osg::CullFace::BACK));

            osg::Material* material = new osg::Material;
            material->setColorMode(osg::Material::DIFFUSE);
            material->setDiffuse(osg::Material::FRONT_AND_BACK,
                                osg::Vec4(1, 1, 1, 1));
            material->setAmbient(osg::Material::FRONT_AND_BACK,
                                osg::Vec4(0, 0, 0, 1));
            material->setEmission(osg::Material::FRONT_AND_BACK,
                                osg::Vec4(0, 0, 0, 1));
            material->setSpecular(osg::Material::FRONT_AND_BACK,
                                osg::Vec4(0, 0, 0, 1));
            material->setShininess(osg::Material::FRONT_AND_BACK, 0);
            stateSet->setAttribute(material);

            geode->setStateSet(stateSet);
        } else if ( tm == SGMesh::TEXTURE_BLUEMARBLE ) {
            osg::StateSet* stateSet = new osg::StateSet;
            stateSet->setAttributeAndModes(new osg::CullFace);

            std::string imageFileName = options->getPluginStringData("SimGear::FG_WORLD_TEXTURE");
            if (imageFileName.empty()) {
                imageFileName = options->getPluginStringData("SimGear::FG_ROOT");
                imageFileName = osgDB::concatPaths(imageFileName, "Textures");
                imageFileName = osgDB::concatPaths(imageFileName, "Globe");
                imageFileName = osgDB::concatPaths(imageFileName, "world.topo.bathy.200407.3x4096x2048.png");
            }
            if (osg::Image* image = osgDB::readImageFile(imageFileName, options)) {
                osg::Texture2D* texture = new osg::Texture2D;
                texture->setImage(image);
                texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
                texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP);
                stateSet->setTextureAttributeAndModes(0, texture);
            }

            geode->setStateSet(stateSet);
        } else if ( tm == SGMesh::TEXTURE_RASTER ) {
            // TODO
        } else {
            printf("texture unknown!\n");
            exit(0);
        }
    }
}

void SGMesh::need_faces()
{
    for (unsigned int i = 0; i < grid_width-1; i++) {
        for (unsigned int j = 0; j < grid_height-1; j++) {
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
            long nf = faces.size();
#pragma omp parallel for
            for (long i = 0; i < nf; i++) {
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
        long nn = normals->size();
#pragma omp parallel for
        for (long i = 0; i < nn; i++)
            (*normals)[i].normalize();
    }
}

osg::DrawElementsUInt* SGMesh::getIndices(void)
{
    osg::DrawElementsUInt* indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);

    for (unsigned int i = 0; i < grid_width-1; i++) {
        for (unsigned int j = 0; j < grid_height-1; j++) {
            // for each point - create 2 triangles with current point in the 
            // lower left corner 
            indices->push_back( (i+0)*grid_height + (j+0) );  // 0,0
            indices->push_back( (i+1)*grid_height + (j+0) );  // 1,0
            indices->push_back( (i+0)*grid_height + (j+1) );  // 0,1

            indices->push_back( (i+1)*grid_height + (j+1) );  // 1,1
            indices->push_back( (i+0)*grid_height + (j+1) );  // 0,1
            indices->push_back( (i+1)*grid_height + (j+0) );  // 1,0
        }
    }
    indices->setDataVariance(osg::Object::STATIC);

    return indices;
}
