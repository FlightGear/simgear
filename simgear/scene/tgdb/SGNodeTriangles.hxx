// future API - just run through once to convert from OSG to SG
// then we can use these triangle lists for random 
// trees/lights/buildings/objects
struct SGTexturedTriangle
{
public:
    std::vector<SGVec3f> vertices;
    std::vector<SGVec2f> texcoords;
};

struct SGBorderContour
{
public:
    SGVec3d start;
    SGVec3d end;
};

class SGTriangleInfo
{
public:
    SGTriangleInfo( const SGVec3d& center ) {
        gbs_center = center;
        mt_init(&seed, 123);
    }

    // API used to build the Info by the visitor
    void addGeometry( osg::Geometry* g ) { 
        geometries.push_back(g); 
    }
    
    void setMaterial( SGMaterial* m ) { 
        mat = m; 
    }

    SGMaterial* getMaterial( void ) const {
        return mat; 
    }
    
    // API used to get a specific texture or effect from a material.  Materials can have
    // multiple textures - use the floor of the x coordinate of the first vertes to select it.
    // This will be constant, and give the same result each time to select one effect/texture per drawable.
    int getTextureIndex( void ) const {
        int texInfo = 0;
        const osg::Vec3Array* vertices =  dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());
        if ( vertices ) {
            const osg::Vec3 *v0 = &vertices->operator[](0);
            texInfo = floor(v0->x()); 
        }
        return texInfo;
    }
    
    // new API - TODO
    void getTriangles( std::vector<SGTexturedTriangle>& tris )
    {
        const osg::Vec3Array* vertices  = dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());
        const osg::Vec2Array* texcoords = dynamic_cast<osg::Vec2Array*>(geometries[0]->getTexCoordArray(0));
        
        int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
        if ( numPrimitiveSets > 0 ) {
            const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
            unsigned int numIndices = ps->getNumIndices();
            
            for ( unsigned int i=2; i<numIndices; i+= 3 ) {
                SGTexturedTriangle tri;
                
                tri.vertices.push_back( toSG(vertices->operator[](ps->index(i-2))) );
                tri.vertices.push_back( toSG(vertices->operator[](ps->index(i-1))) );
                tri.vertices.push_back( toSG(vertices->operator[](ps->index(i-0))) );
                
                tri.texcoords.push_back( toSG(texcoords->operator[](ps->index(i-2))) );
                tri.texcoords.push_back( toSG(texcoords->operator[](ps->index(i-1))) );
                tri.texcoords.push_back( toSG(texcoords->operator[](ps->index(i-0))) );
            }
        }
    }
    
    void getBorderContours( std::vector<SGBorderContour>& border )
    {
        // each structure contains a list of target indexes and a count
        int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
        if ( numPrimitiveSets > 0 ) {
            const osg::Vec3Array* vertices  = dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());

            const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
            unsigned int numTriangles = ps->getNumIndices()/3;

            // use a map for fast lookup map the segment as a 64 bit int
            std::map<uint64_t, int> segCounter;
            uint32_t idx1, idx2;
            uint64_t key;
            
            for ( unsigned int i=0; i<numTriangles; i+= 3 ) {
                // first seg
                if ( ps->index(i+0) < ps->index(i+1) ) {
                    idx1 = ps->index(i+0);
                    idx2 = ps->index(i+1);
                } else {
                    idx1 = ps->index(i+1);
                    idx2 = ps->index(i+0);                    
                }
                
                key=( (uint64_t)idx1<<32) | (uint64_t)idx2;
                SG_LOG(SG_TERRAIN, SG_ALERT, "key " << std::hex << key << std::dec << " count is " << segCounter[key] );
                segCounter[key]++;
                SG_LOG(SG_TERRAIN, SG_ALERT, "after increment key " << std::hex << key << std::dec << " count is " << segCounter[key] );
                
                // second seg
                if ( ps->index(i+1) < ps->index(i+2) ) {
                    idx1 = ps->index(i+1);
                    idx2 = ps->index(i+2);
                } else {
                    idx1 = ps->index(i+2);
                    idx2 = ps->index(i+1);             
                }
                
                key=( (uint64_t)idx1<<32) | (uint64_t)idx2;
                SG_LOG(SG_TERRAIN, SG_ALERT, "key " << std::hex << key << std::dec << " count is " << segCounter[key] );
                segCounter[key]++;
                SG_LOG(SG_TERRAIN, SG_ALERT, "after increment key " << std::hex << key << std::dec << " count is " << segCounter[key] );
                
                // third seg
                if ( ps->index(i+2) < ps->index(i+0) ) {
                    idx1 = ps->index(i+2);
                    idx2 = ps->index(i+0);
                } else {
                    idx1 = ps->index(i+0);
                    idx2 = ps->index(i+2);                    
                }
                
                key=( (uint64_t)idx1<<32) | (uint64_t)idx2;
                SG_LOG(SG_TERRAIN, SG_ALERT, "key " << std::hex << key << std::dec << " count is " << segCounter[key] );
                segCounter[key]++;
                SG_LOG(SG_TERRAIN, SG_ALERT, "after increment key " << std::hex << key << std::dec << " count is " << segCounter[key] );
            }
            
            // return all segments with count = 1 ( border )
            std::map<uint64_t, int>::iterator segIt = segCounter.begin();            
            while ( segIt != segCounter.end() ) {
                if ( segIt->second == 1 ) {
                    SG_LOG(SG_TERRAIN, SG_ALERT, "key " << std::hex << segIt->first << std::dec << " count is " << segIt->second );
                    
                    unsigned int iStart = segIt->first >> 32;
                    unsigned int iEnd   = segIt->first & 0x00000000FFFFFFFF;
                    
                    SGBorderContour bc;
                    
                    bc.start = toVec3d(toSG(vertices->operator[](iStart)));
                    bc.end   = toVec3d(toSG(vertices->operator[](iEnd)));
                    border.push_back( bc );
                }
                segIt++;
            }
            
#if 0
            // debug out - requires GDAL
            //
            //
            //
            SGGeod  geodPos = SGGeod::fromCart(gbs_center);
            SGQuatd hlOr = SGQuatd::fromLonLat(geodPos)*SGQuatd::fromEulerDeg(0, 0, 180);

            for ( unsigned int i=0; i<border.size(); i++ ){
                // de-rotate and translate : todo - create a paralell vertex list so we just do this 
                // once per vertex, not for every triangle's use of the vertex
                SGVec3d sgVStart = hlOr.backTransform( border[i].start) + gbs_center;
                SGVec3d sgVEnd   = hlOr.backTransform( border[i].end)   + gbs_center;
                    
                // convert from cartesian to Geodetic, and save as a list of Geods for output
                SGGeod gStart = SGGeod::fromCart(sgVStart);
                SGGeod gEnd   = SGGeod::fromCart(sgVEnd);
                
                SGShapefile::FromSegment( gStart, gEnd, true, "./borders", mat->get_names()[0], "border" );
            }
#endif            
        }
    }
        
    // Random buildings API - get num triangles, then get a triangle at index
    unsigned int getNumTriangles( void ) const {
        unsigned int num_triangles = 0;

        if ( !geometries.empty() ) {
            int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
            if ( numPrimitiveSets > 0 ) {
                const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
                unsigned int numIndices = ps->getNumIndices();
                num_triangles = numIndices/3;
            }
        }
        
        return num_triangles;
    }
    
    void getTriangle(unsigned int i, std::vector<SGVec3f>& triVerts, std::vector<SGVec2f>& triTCs) const {
        const osg::Vec3Array* vertices  = dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());
        const osg::Vec2Array* texcoords = dynamic_cast<osg::Vec2Array*>(geometries[0]->getTexCoordArray(0));

        if ( !geometries.empty() ) {
            int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
            if ( numPrimitiveSets > 0 ) {
                const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
                int idxStart = i*3;
                
                triVerts.push_back( toSG(vertices->operator[](ps->index(idxStart+0))) );
                triVerts.push_back( toSG(vertices->operator[](ps->index(idxStart+1))) );
                triVerts.push_back( toSG(vertices->operator[](ps->index(idxStart+2))) );
                
                triTCs.push_back( toSG(texcoords->operator[](ps->index(idxStart+0))) );
                triTCs.push_back( toSG(texcoords->operator[](ps->index(idxStart+1))) );
                triTCs.push_back( toSG(texcoords->operator[](ps->index(idxStart+2))) );
            }
        }
    }
    
    // random lights and trees - just get a list of points on where to add the light / tree
    // TODO move this out - and handle in the random light / tree code
    // just use generic triangle API.
    void addRandomSurfacePoints(float coverage, float offset,
                                osg::Texture2D* object_mask,
                                std::vector<SGVec3f>& points)
    {
        if ( !geometries.empty() ) {
            const osg::Vec3Array* vertices  = dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());
            const osg::Vec2Array* texcoords = dynamic_cast<osg::Vec2Array*>(geometries[0]->getTexCoordArray(0));
            
            int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
            if ( numPrimitiveSets > 0 ) {
                const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
                unsigned int numIndices = ps->getNumIndices();
                
                for ( unsigned int i=2; i<numIndices; i+= 3 ) {                    
                    SGVec3f v0 = toSG(vertices->operator[](ps->index(i-2)));
                    SGVec3f v1 = toSG(vertices->operator[](ps->index(i-1)));
                    SGVec3f v2 = toSG(vertices->operator[](ps->index(i-0)));
                    
                    SGVec2f t0 = toSG(texcoords->operator[](ps->index(i-2)));
                    SGVec2f t1 = toSG(texcoords->operator[](ps->index(i-1)));
                    SGVec2f t2 = toSG(texcoords->operator[](ps->index(i-0)));
                    
                    SGVec3f normal = cross(v1 - v0, v2 - v0);
            
                    // Compute the area
                    float area = 0.5f*length(normal);
                    if (area <= SGLimitsf::min())
                        continue;
            
                    // For partial units of area, use a zombie door method to
                    // create the proper random chance of a light being created
                    // for this triangle
                    float unit = area + mt_rand(&seed)*coverage;
            
                    SGVec3f offsetVector = offset*normalize(normal);
                    // generate a light point for each unit of area
            
                    while ( coverage < unit ) {
                        float a = mt_rand(&seed);
                        float b = mt_rand(&seed);
                
                        if ( a + b > 1 ) {
                            a = 1 - a;
                            b = 1 - b;
                        }
                        float c = 1 - a - b;
                        SGVec3f randomPoint = offsetVector + a*v0 + b*v1 + c*v2;
                
                        if (object_mask != NULL) {
                            SGVec2f texCoord = a*t0 + b*t1 + c*t2;
                    
                            // Check this random point against the object mask
                            // red channel.
                            osg::Image* img = object_mask->getImage();            
                            unsigned int x = (int) (img->s() * texCoord.x()) % img->s();
                            unsigned int y = (int) (img->t() * texCoord.y()) % img->t();
                    
                            if (mt_rand(&seed) < img->getColor(x, y).r()) {                
                                points.push_back(randomPoint);        
                            }                    
                        } else {      
                            // No object mask, so simply place the object  
                            points.push_back(randomPoint);        
                        }
                        unit -= coverage;        
                    }
                }
            }
        }
    }
    
    void addRandomTreePoints(float wood_coverage, 
                             osg::Texture2D* object_mask,
                             float vegetation_density,
                             float cos_max_density_angle,
                             float cos_zero_density_angle,
                             std::vector<SGVec3f>& points)
    {
        if ( !geometries.empty() ) {
            const osg::Vec3Array* vertices  = dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());
            const osg::Vec2Array* texcoords = dynamic_cast<osg::Vec2Array*>(geometries[0]->getTexCoordArray(0));
            
            int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
            if ( numPrimitiveSets > 0 ) {
                const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
                unsigned int numIndices = ps->getNumIndices();
                
                for ( unsigned int i=2; i<numIndices; i+= 3 ) {                    
                    SGVec3f v0 = toSG(vertices->operator[](ps->index(i-2)));
                    SGVec3f v1 = toSG(vertices->operator[](ps->index(i-1)));
                    SGVec3f v2 = toSG(vertices->operator[](ps->index(i-0)));
                    
                    SGVec2f t0 = toSG(texcoords->operator[](ps->index(i-2)));
                    SGVec2f t1 = toSG(texcoords->operator[](ps->index(i-1)));
                    SGVec2f t2 = toSG(texcoords->operator[](ps->index(i-0)));
                    
                    SGVec3f normal = cross(v1 - v0, v2 - v0);
            
                    // Ensure the slope isn't too steep by checking the
                    // cos of the angle between the slope normal and the
                    // vertical (conveniently the z-component of the normalized
                    // normal) and values passed in.                   
                    float alpha = normalize(normal).z();
                    float slope_density = 1.0;
            
                    if (alpha < cos_zero_density_angle) 
                        continue; // Too steep for any vegetation      
                        
                    if (alpha < cos_max_density_angle) {
                        slope_density = 
                        (alpha - cos_zero_density_angle) / (cos_max_density_angle - cos_zero_density_angle);
                    }
                    
                    // Compute the area
                    float area = 0.5f*length(normal);
                    if (area <= SGLimitsf::min())
                        continue;
                
                    // Determine the number of trees, taking into account vegetation
                    // density (which is linear) and the slope density factor.
                    // Use a zombie door method to create the proper random chance 
                    // of a tree being created for partial values.
                    int woodcount = (int) (vegetation_density * vegetation_density * 
                    slope_density *
                    area / wood_coverage + mt_rand(&seed));
                    
                    for (int j = 0; j < woodcount; j++) {
                        float a = mt_rand(&seed);
                        float b = mt_rand(&seed);
                        
                        if ( a + b > 1.0f ) {
                            a = 1.0f - a;
                            b = 1.0f - b;
                        }
                        
                        float c = 1.0f - a - b;
                        
                        SGVec3f randomPoint = a*v0 + b*v1 + c*v2;
                        
                        if (object_mask != NULL) {
                            SGVec2f texCoord = a*t0 + b*t1 + c*t2;
                            
                            // Check this random point against the object mask
                            // green (for trees) channel. 
                            osg::Image* img = object_mask->getImage();            
                            unsigned int x = (int) (img->s() * texCoord.x()) % img->s();
                            unsigned int y = (int) (img->t() * texCoord.y()) % img->t();
                            
                            if (mt_rand(&seed) < img->getColor(x, y).g()) {  
                                // The red channel contains the rotation for this object                                  
                                points.push_back(randomPoint);
                            }
                        } else {
                            points.push_back(randomPoint);
                        }                
                    }
                }
            }
        }
    }
    
#if 0    
    // debug : this will save the tile as a shapefile that can be viewed in QGIS.
    // NOTE: this is really slow....
    // remember - we need to de-rotate the tile, then translate back to gbs_center.
    void dumpBorder() {
        //dump the first triangle only of the first geometry, for now...
        SG_LOG(SG_TERRAIN, SG_ALERT, "effect geode has " << geometries.size() << " geometries" );
        
        const osg::Vec3Array* vertices =  dynamic_cast<osg::Vec3Array*>(geometries[0]->getVertexArray());
        if ( vertices ) {
            SG_LOG(SG_TERRAIN, SG_ALERT, " geometry has " << vertices->getNumElements() << " vertices" );
        }
        
        if ( !geometries.empty() ) {
            int numPrimitiveSets = geometries[0]->getNumPrimitiveSets();
            SG_LOG(SG_TERRAIN, SG_ALERT, " geometry has " << numPrimitiveSets << " primitive sets" );
            
            if ( numPrimitiveSets > 0 ) {
                const osg::PrimitiveSet* ps = geometries[0]->getPrimitiveSet(0);
                unsigned int numIndices = ps->getNumIndices();

                // create the same quat we used to rotate here 
                // - use backTransform to go back to original node location
                SGGeod  geodPos = SGGeod::fromCart(gbs_center);
                SGQuatd hlOr = SGQuatd::fromLonLat(geodPos)*SGQuatd::fromEulerDeg(0, 0, 180);
                
                SG_LOG(SG_TERRAIN, SG_ALERT, "  primitive set has has " << numIndices << " indices" );                
                for ( unsigned int i=2; i<numIndices; i+= 3 ) {
                    if ( numIndices >= 3 ) {                    
                        unsigned int v0i = ps->index(i-2);
                        unsigned int v1i = ps->index(i-1);
                        unsigned int v2i = ps->index(i-0);
                    
                        const osg::Vec3 *v0 = &vertices->operator[](v0i);
                        const osg::Vec3 *v1 = &vertices->operator[](v1i);
                        const osg::Vec3 *v2 = &vertices->operator[](v2i);
                   
                        // de-rotate and translate : todo - create a paralell vertex list so we just do this 
                        // once per vertex, not for every triangle's use of the vertex
                        SGVec3d vec0 = hlOr.backTransform( toVec3d(toSG(*v0))) + gbs_center;
                        SGVec3d vec1 = hlOr.backTransform( toVec3d(toSG(*v1))) + gbs_center;
                        SGVec3d vec2 = hlOr.backTransform( toVec3d(toSG(*v2))) + gbs_center;
                    
                        // convert from cartesian to Geodetic, and save as a list of Geods for output
                        std::vector<SGGeod> triangle;
                        triangle.push_back( SGGeod::fromCart(vec0) );
                        triangle.push_back( SGGeod::fromCart(vec1) );
                        triangle.push_back( SGGeod::fromCart(vec2) );
                    
                        SGShapefile::FromGeodList( triangle, true, "./triangles", mat->get_names()[0], "tri" );
                    }
                }
            }            
            
        }
    }
#endif    
    
private:
    mt seed;
    SGMaterial* mat;
    SGVec3d gbs_center;
    std::vector<osg::Geometry*> geometries;
    std::vector<int> polygon_border;    // TODO
};

// This visitor will generate an SGTriangleInfo.
// currently, it looks like it could save multiple lists, which could be the case
// if multiple osg::geods are found with osg::Geometry.
// But right now, we store a single PrimitiveSet under a single EffectGeod.  
// so the traversal should only find a single EffectGeod - building a single SGTriangleInfo
class GetNodeTriangles : public osg::NodeVisitor
{
public:
    GetNodeTriangles(const SGVec3d& c, std::vector<SGTriangleInfo>* nt) : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ), center(c), nodeTris(nt) {}
    
    // This method gets called for every node in the scene
    //   graph. Check each node to see if it has user
    //   out target. If so, save the node's address.
    virtual void apply( osg::Node& node )
    {
        EffectGeode* eg = dynamic_cast<EffectGeode*>(&node);
        if ( eg ) {
            // get the material from the user info
            SGTriangleInfo triInfo( center );
            triInfo.setMaterial( eg->getMaterial() );

            // let's find the drawables for this node
            int numDrawables = eg->getNumDrawables();
            for ( int i=0; i<numDrawables; i++ ) {
                triInfo.addGeometry( eg->getDrawable(i)->asGeometry() );
            }
            
            nodeTris->push_back( triInfo );
        }
        
        // Keep traversing the rest of the scene graph.
        traverse( node );
    }
    
protected:
    SGVec3d                         center;
    std::vector<SGTriangleInfo>*    nodeTris;
};
