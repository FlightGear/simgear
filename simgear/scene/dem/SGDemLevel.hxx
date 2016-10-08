#ifndef __SG_DEM_LEVEL_HXX__
#define __SG_DEM_LEVEL_HXX__

#include <cstring>
#include <cstdio>

#include <simgear/misc/sg_dir.hxx>

class SGDemLevel
{
public:
    SGDemLevel( int l, const SGPath& p, int w, int h, int x, int y, int o, const unsigned long* e, std::string ext, bool r )
    {
        level = l;
        path = p;
        width = w;
        height = h;
        xres = x;
        yres = y;
        overlap = o;
        extension = ext;
        ready = r;

        std::fprintf( stderr, "copying extent fle size %lu\n", sizeof(extent) );
        std::memcpy( (void *)&extent[0][0], (const void *)e, sizeof(extent) );
    };

    SGDemLevel( int l, const SGPath& p, int w, int h, int x, int y, int o, std::string ext, bool r )
    {
        level = l;
        path = p;
        width = w;
        height = h;
        xres = x;
        yres = y;
        overlap = o;
        extension = ext;
        ready = r;

        std::fprintf( stderr, "setting extent fle size %lu\n", sizeof(extent) );
        memset( (unsigned char*)extent, 0, sizeof(extent) );
    };

    int getLevel( void ) const {
        return level;
    }
    const SGPath& getLevelDir( void ) const {
        return path;
    }
    bool isReady( void ) const {
        return ready;
    }
    bool isValid( unsigned int wo, unsigned int so, unsigned int eo, unsigned int no ) const;

    void addExtent( unsigned int wo, unsigned int so, unsigned int eo, unsigned int no );
    void close( void );

    int getWidth( void ) const {
        return width;
    }
    int getHeight( void ) const {
        return height;
    }
    int getResX( void ) const {
        return xres;
    }
    int getResY( void ) const {
        return yres;
    }
    int getOverlap( void ) const {
        return overlap;
    }

private:
    SGPath path;
    std::string extension;
    bool ready;

    int level;
    int width;
    int height;
    int xres;
    int yres;
    int overlap;

    // bitmap of tiles (1x1 degree)
    unsigned char extent[180][360/8];
};

#endif /* __SG_DEM_LEVEL_HXX__ */
