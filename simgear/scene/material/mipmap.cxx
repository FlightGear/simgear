// Copyright (C) 2010  Frederic Bouvier
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "mipmap.hxx"
#include "EffectBuilder.hxx"

#include <limits>
#include <iomanip>

#include <osg/Image>
#include <osg/Vec4>

namespace simgear { namespace effect {

EffectNameValue<MipMapFunction> mipmapFunctionsInit[] =
{
    {"auto", AUTOMATIC},
    {"average", AVERAGE},
    {"sum", SUM},
    {"product", PRODUCT},
    {"min", MIN},
    {"max", MAX}
};
EffectPropertyMap<MipMapFunction> mipmapFunctions(mipmapFunctionsInit);

MipMapTuple makeMipMapTuple(Effect* effect, const SGPropertyNode* props,
                      const SGReaderWriterOptions* options)
{
    const SGPropertyNode* pMipmapR
        = getEffectPropertyChild(effect, props, "function-r");
    MipMapFunction mipmapR = AUTOMATIC;
    if (pMipmapR)
        findAttr(mipmapFunctions, pMipmapR, mipmapR);
    const SGPropertyNode* pMipmapG
        = getEffectPropertyChild(effect, props, "function-g");
    MipMapFunction mipmapG = AUTOMATIC;
    if (pMipmapG)
        findAttr(mipmapFunctions, pMipmapG, mipmapG);
    const SGPropertyNode* pMipmapB
        = getEffectPropertyChild(effect, props, "function-b");
    MipMapFunction mipmapB = AUTOMATIC;
    if (pMipmapB)
        findAttr(mipmapFunctions, pMipmapB, mipmapB);
    const SGPropertyNode* pMipmapA
        = getEffectPropertyChild(effect, props, "function-a");
    MipMapFunction mipmapA = AUTOMATIC;
    if (pMipmapA)
        findAttr(mipmapFunctions, pMipmapA, mipmapA);
    return MipMapTuple( mipmapR, mipmapG, mipmapB, mipmapA );
}


unsigned char* imageData(unsigned char* ptr, GLenum pixelFormat, GLenum dataType, int width, int height, int packing, int column, int row=0, int image=0)
{
    if (!ptr) return NULL;
    return ptr +
        ( column * osg::Image::computePixelSizeInBits( pixelFormat, dataType ) ) / 8 +
        row * osg::Image::computeRowWidthInBytes( width, pixelFormat, dataType, packing ) +
        image * height * osg::Image::computeRowWidthInBytes( width, pixelFormat, dataType, packing );
}

template <typename T>
void _writeColor(GLenum pixelFormat, T* data, float scale, osg::Vec4 value)
{
    switch(pixelFormat)
    {
        case(GL_DEPTH_COMPONENT):   //intentionally fall through and execute the code for GL_LUMINANCE
        case(GL_LUMINANCE):         { *data = value.r()*scale; break; }
        case(GL_ALPHA):             { *data = value.a()*scale; break; }
        case(GL_LUMINANCE_ALPHA):   { *data++ = value.r()*scale; *data = value.a()*scale; break; }
        case(GL_RGB):               { *data++ = value.r()*scale; *data++ = value.g()*scale; *data = value.b()*scale; break; }
        case(GL_RGBA):              { *data++ = value.r()*scale; *data++ = value.g()*scale; *data++ = value.b()*scale; *data = value.a()*scale; break; }
        case(GL_BGR):               { *data++ = value.b()*scale; *data++ = value.g()*scale; *data = value.r()*scale; break; }
        case(GL_BGRA):              { *data++ = value.b()*scale; *data++ = value.g()*scale; *data++ = value.r()*scale; *data = value.a()*scale; break; }
    }
}

void setColor(unsigned char *ptr, GLenum pixelFormat, GLenum dataType, osg::Vec4 color)
{
    switch(dataType)
    {
        case(GL_BYTE):              return _writeColor(pixelFormat, (char*)ptr,             128.0f, color);
        case(GL_UNSIGNED_BYTE):     return _writeColor(pixelFormat, (unsigned char*)ptr,    255.0f, color);
        case(GL_SHORT):             return _writeColor(pixelFormat, (short*)ptr,            32768.0f, color);
        case(GL_UNSIGNED_SHORT):    return _writeColor(pixelFormat, (unsigned short*)ptr,   65535.0f, color);
        case(GL_INT):               return _writeColor(pixelFormat, (int*)ptr,              2147483648.0f, color);
        case(GL_UNSIGNED_INT):      return _writeColor(pixelFormat, (unsigned int*)ptr,     4294967295.0f, color);
        case(GL_FLOAT):             return _writeColor(pixelFormat, (float*)ptr,            1.0f, color);
    }
}

template <typename T>    
osg::Vec4 _readColor(GLenum pixelFormat, T* data,float scale)
{
    switch(pixelFormat)
    {
        case(GL_DEPTH_COMPONENT):   //intentionally fall through and execute the code for GL_LUMINANCE
        case(GL_LUMINANCE):         { float l = float(*data++)*scale; return osg::Vec4(l, l, l, 1.0f); }
        case(GL_ALPHA):             { float a = float(*data++)*scale; return osg::Vec4(1.0f, 1.0f, 1.0f, a); }
        case(GL_LUMINANCE_ALPHA):   { float l = float(*data++)*scale; float a = float(*data++)*scale; return osg::Vec4(l,l,l,a); }
        case(GL_RGB):               { float r = float(*data++)*scale; float g = float(*data++)*scale; float b = float(*data++)*scale; return osg::Vec4(r,g,b,1.0f); }
        case(GL_RGBA):              { float r = float(*data++)*scale; float g = float(*data++)*scale; float b = float(*data++)*scale; float a = float(*data++)*scale; return osg::Vec4(r,g,b,a); }
        case(GL_BGR):               { float b = float(*data++)*scale; float g = float(*data++)*scale; float r = float(*data++)*scale; return osg::Vec4(r,g,b,1.0f); }
        case(GL_BGRA):              { float b = float(*data++)*scale; float g = float(*data++)*scale; float r = float(*data++)*scale; float a = float(*data++)*scale; return osg::Vec4(r,g,b,a); }
    }
    return osg::Vec4(1.0f,1.0f,1.0f,1.0f);
}

osg::Vec4 getColor(const unsigned char* ptr, GLenum pixelFormat, GLenum dataType)
{
    switch(dataType)
    {
        case(GL_BYTE):              return _readColor(pixelFormat, (char*)ptr,             1.0f/128.0f);
        case(GL_UNSIGNED_BYTE):     return _readColor(pixelFormat, (unsigned char*)ptr,    1.0f/255.0f);
        case(GL_SHORT):             return _readColor(pixelFormat, (short*)ptr,            1.0f/32768.0f);
        case(GL_UNSIGNED_SHORT):    return _readColor(pixelFormat, (unsigned short*)ptr,   1.0f/65535.0f);
        case(GL_INT):               return _readColor(pixelFormat, (int*)ptr,              1.0f/2147483648.0f);
        case(GL_UNSIGNED_INT):      return _readColor(pixelFormat, (unsigned int*)ptr,     1.0f/4294967295.0f);
        case(GL_FLOAT):             return _readColor(pixelFormat, (float*)ptr,            1.0f);
    }
    return osg::Vec4(1.0f,1.0f,1.0f,1.0f);
}

osg::Vec4::value_type computeAverage( int c, osg::Vec4 colors[2][2][2], bool colorValid[2][2][2] )
{
    osg::Vec4::value_type r = 0;
    osg::Vec4::value_type nb = 0;
    for ( int k = 0; k < 2; ++k ) for ( int j = 0; j < 2; ++j ) for ( int i = 0; i < 2; ++i )
    {
        if ( colorValid[i][j][k] )
        {
            r += colors[i][j][k][c];
            nb += 1;
        }
    }
    return r / nb;
}

osg::Vec4::value_type computeSum( int c, osg::Vec4 colors[2][2][2], bool colorValid[2][2][2] )
{
    osg::Vec4::value_type r = 0;
    for ( int k = 0; k < 2; ++k ) for ( int j = 0; j < 2; ++j ) for ( int i = 0; i < 2; ++i )
        if ( colorValid[i][j][k] )
        {
            r += colors[i][j][k][c];
        }
    return r;
}

osg::Vec4::value_type computeProduct( int c, osg::Vec4 colors[2][2][2], bool colorValid[2][2][2] )
{
    osg::Vec4::value_type r = 1;
    for ( int k = 0; k < 2; ++k ) for ( int j = 0; j < 2; ++j ) for ( int i = 0; i < 2; ++i )
        if ( colorValid[i][j][k] )
        {
            r *= colors[i][j][k][c];
        }
    return r;
}

osg::Vec4::value_type computeMin( int c, osg::Vec4 colors[2][2][2], bool colorValid[2][2][2] )
{
    osg::Vec4::value_type r = std::numeric_limits<osg::Vec4::value_type>::max();
    for ( int k = 0; k < 2; ++k ) for ( int j = 0; j < 2; ++j ) for ( int i = 0; i < 2; ++i )
        if ( colorValid[i][j][k] )
        {
            r = std::min( r, colors[i][j][k][c] );
        }
    return r;
}

osg::Vec4::value_type computeMax( int c, osg::Vec4 colors[2][2][2], bool colorValid[2][2][2] )
{
    osg::Vec4::value_type r = std::numeric_limits<osg::Vec4::value_type>::min();
    for ( int k = 0; k < 2; ++k ) for ( int j = 0; j < 2; ++j ) for ( int i = 0; i < 2; ++i )
        if ( colorValid[i][j][k] )
        {
            r = std::max( r, colors[i][j][k][c] );
        }
    return r;
}

osg::Vec4::value_type computeComponent( int c, osg::Vec4 colors[2][2][2], bool colorValid[2][2][2], MipMapFunction f )
{
    switch ( f )
    {
    case AVERAGE: return computeAverage( c, colors, colorValid );
    case SUM: return computeSum( c, colors, colorValid );
    case PRODUCT: return computeProduct( c, colors, colorValid );
    case MIN: return computeMin( c, colors, colorValid );
    case MAX: return computeMax( c, colors, colorValid );
    default: break;
    }
    return 0;
}

osg::Vec4 computeColor( osg::Vec4 colors[2][2][2], bool colorValid[2][2][2], MipMapTuple attrs, GLenum pixelFormat )
{
    osg::Vec4 result;
    unsigned int nbComponents = osg::Image::computeNumComponents( pixelFormat );
    result[0] = computeComponent( 0, colors, colorValid, std::get<0>(attrs) );
    if ( nbComponents >= 2 )
        result[1] = computeComponent( 1, colors, colorValid, std::get<1>(attrs) );
    if ( nbComponents >= 3 )
        result[2] = computeComponent( 2, colors, colorValid, std::get<2>(attrs) );
    if ( nbComponents == 4 )
        result[3] = computeComponent( 3, colors, colorValid, std::get<3>(attrs) );
    return result;
}

void dumpMipmap( std::string n, int s, int t, int r, int c, unsigned char *d, const osg::Image::MipmapDataType &o )
{
    std::ofstream ofs( (n + ".dump").c_str() );
    for ( osg::Image::MipmapDataType::size_type i = 0; i < o.size()+1; ++i )
    {
        ofs << s << " " << t << " " << r << std::endl;
        unsigned int offset = 0;
        if ( i > 0 )
            offset = o[i-1];
        unsigned char *p = &d[offset];
        for ( int l = 0; l < r; ++l )
        {
            for ( int k = 0; k < t; ++k )
            {
                for ( int j = 0; j < s; ++j )
                {
                    ofs << "(";
                    for ( int m = 0; m < c; ++m )
                    {
                        if ( m != 0 )
                            ofs << " ";
                        ofs << std::setw(3) << (unsigned int)*p++;
                    }
                    ofs << ")";
                }
                ofs << std::endl;
            }
            ofs << std::endl;
        }
        ofs << std::endl;
        s >>= 1; if ( s == 0 ) s = 1;
        t >>= 1; if ( t == 0 ) t = 1;
        r >>= 1; if ( r == 0 ) r = 1;
    }
}

osg::Image* computeMipmap( osg::Image* image, MipMapTuple attrs )
{
    bool computeMipmap = false;
    unsigned int nbComponents = osg::Image::computeNumComponents( image->getPixelFormat() );
    int s = image->s();
    int t = image->t();
    if ( (s & (s - 1)) || (t & (t - 1)) ) // power of two test
    {
        SG_LOG(SG_IO, SG_DEV_ALERT, "mipmapping: texture size not a power-of-two: " + image->getFileName());
    }
    else if ( std::get<0>(attrs) != AUTOMATIC &&
        ( std::get<1>(attrs) != AUTOMATIC || nbComponents < 2 ) &&
        ( std::get<2>(attrs) != AUTOMATIC || nbComponents < 3 ) &&
        ( std::get<3>(attrs) != AUTOMATIC || nbComponents < 4 ) )
    {
        computeMipmap = true;
    }
    else if ( std::get<0>(attrs) != AUTOMATIC ||
             ( std::get<1>(attrs) != AUTOMATIC && nbComponents >= 2 ) ||
             ( std::get<2>(attrs) != AUTOMATIC && nbComponents >= 3 ) ||
             ( std::get<3>(attrs) != AUTOMATIC && nbComponents == 4 ) )
    {
        throw BuilderException("invalid mipmap control function combination");
    }

    if ( computeMipmap )
    {
        osg::ref_ptr<osg::Image> mipmaps = new osg::Image();
        int r = image->r();
        int nb = osg::Image::computeNumberOfMipmapLevels(s, t, r);
        osg::Image::MipmapDataType mipmapOffsets;
        unsigned int offset = 0;
        for ( int i = 0; i < nb; ++i )
        {
            offset += t * r * osg::Image::computeRowWidthInBytes(s, image->getPixelFormat(), image->getDataType(), image->getPacking());
            mipmapOffsets.push_back( offset );
            s >>= 1; if ( s == 0 ) s = 1;
            t >>= 1; if ( t == 0 ) t = 1;
            r >>= 1; if ( r == 0 ) r = 1;
        }
        mipmapOffsets.pop_back();
        unsigned char *data = new unsigned char[offset];
        memcpy( data, image->data(), mipmapOffsets.front() );
        s = image->s();
        t = image->t();
        r = image->r();
        for ( int m = 0; m < nb-1; ++m )
        {
            unsigned char *src = data;
            if ( m > 0 )
                src += mipmapOffsets[m-1];

            unsigned char *dest = data + mipmapOffsets[m];

            int ns = s >> 1; if ( ns == 0 ) ns = 1;
            int nt = t >> 1; if ( nt == 0 ) nt = 1;
            int nr = r >> 1; if ( nr == 0 ) nr = 1;

            for ( int k = 0; k < r; k += 2 )
            {
                for ( int j = 0; j < t; j += 2 )
                {
                    for ( int i = 0; i < s; i += 2 )
                    {
                        osg::Vec4 colors[2][2][2];
                        bool colorValid[2][2][2];
                        colorValid[0][0][0] = false; colorValid[0][0][1] = false; colorValid[0][1][0] = false; colorValid[0][1][1] = false;
                        colorValid[1][0][0] = false; colorValid[1][0][1] = false; colorValid[1][1][0] = false; colorValid[1][1][1] = false;
                        if ( true )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i, j, k );
                            colors[0][0][0] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[0][0][0] = true;
                        }
                        if ( i + 1 < s )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i + 1, j, k );
                            colors[0][0][1] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[0][0][1] = true;
                        }
                        if ( j + 1 < t )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i, j + 1, k );
                            colors[0][1][0] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[0][1][0] = true;
                        }
                        if ( i + 1 < s && j + 1 < t )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i + 1, j + 1, k );
                            colors[0][1][1] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[0][1][1] = true;
                        }
                        if ( k + 1 < r )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i, j, k + 1 );
                            colors[1][0][0] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[1][0][0] = true;
                        }
                        if ( i + 1 < s && k + 1 < r )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i + 1, j, k + 1 );
                            colors[1][0][1] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[1][0][1] = true;
                        }
                        if ( j + 1 < t && k + 1 < r )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i, j + 1, k + 1 );
                            colors[1][1][0] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[1][1][0] = true;
                        }
                        if ( i + 1 < s && j + 1 < t && k + 1 < r )
                        {
                            unsigned char *ptr = imageData( src, image->getPixelFormat(), image->getDataType(), s, t, image->getPacking(), i + 1, j + 1, k + 1 );
                            colors[1][1][1] = getColor( ptr, image->getPixelFormat(), image->getDataType() );
                            colorValid[1][1][1] = true;
                        }

                        unsigned char *ptr = imageData( dest, image->getPixelFormat(), image->getDataType(), ns, nt, image->getPacking(), i/2, j/2, k/2 );
                        osg::Vec4 color = computeColor( colors, colorValid, attrs, image->getPixelFormat() );
                        setColor( ptr, image->getPixelFormat(), image->getDataType(), color );
                    }
                }
            }
            s = ns;
            t = nt;
            r = nr;
        }
        //dumpMipmap( image->getFileName(), image->s(), image->t(), image->r(), osg::Image::computeNumComponents(image->getPixelFormat()), data, mipmapOffsets );
        mipmaps->setImage( image->s(), image->t(), image->r(), 
            image->getInternalTextureFormat(), image->getPixelFormat(),
            image->getDataType(), data, osg::Image::USE_NEW_DELETE, image->getPacking() );
        mipmaps->setMipmapLevels( mipmapOffsets );
        mipmaps->setName(image->getName());
        mipmaps->setFileName(image->getFileName());

        return mipmaps.release();
    }
    return image;
}
} }
