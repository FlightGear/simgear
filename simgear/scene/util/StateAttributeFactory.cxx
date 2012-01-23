/* -*-c++-*-
 *
 * Copyright (C) 2007 Tim Moore
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
#include "StateAttributeFactory.hxx"

#include <osg/AlphaFunc>
#include <osg/Array>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/ShadeModel>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TexEnv>

#include <osg/Image>

#include <simgear/scene/material/Noise.hxx>

using namespace osg;

namespace simgear
{
StateAttributeFactory::StateAttributeFactory()
{
    _standardAlphaFunc = new AlphaFunc;
    _standardAlphaFunc->setFunction(osg::AlphaFunc::GREATER);
    _standardAlphaFunc->setReferenceValue(0.01f);
    _standardAlphaFunc->setDataVariance(Object::STATIC);
    _smooth = new ShadeModel;
    _smooth->setMode(ShadeModel::SMOOTH);
    _smooth->setDataVariance(Object::STATIC);
    _flat = new ShadeModel(ShadeModel::FLAT);
    _flat->setDataVariance(Object::STATIC);
    _standardBlendFunc = new BlendFunc;
    _standardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    _standardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    _standardBlendFunc->setDataVariance(Object::STATIC);
    _standardTexEnv = new TexEnv;
    _standardTexEnv->setMode(TexEnv::MODULATE);
    _standardTexEnv->setDataVariance(Object::STATIC);
    osg::Image *dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_LUMINANCE_ALPHA,
                              GL_UNSIGNED_BYTE);
    unsigned char* imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 255;
    imageBytes[1] = 255;
    _whiteTexture = new osg::Texture2D;
    _whiteTexture->setImage(dummyImage);
    _whiteTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _whiteTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _whiteTexture->setDataVariance(osg::Object::STATIC);
    // And now the transparent texture
    dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
    imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 255;
    imageBytes[1] = 0;
    _transparentTexture = new osg::Texture2D;
    _transparentTexture->setImage(dummyImage);
    _transparentTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _transparentTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _transparentTexture->setDataVariance(osg::Object::STATIC);
    _white = new Vec4Array(1);
    (*_white)[0].set(1.0f, 1.0f, 1.0f, 1.0f);
    _white->setDataVariance(Object::STATIC);
    _cullFaceFront = new CullFace(CullFace::FRONT);
    _cullFaceFront->setDataVariance(Object::STATIC);
    _cullFaceBack = new CullFace(CullFace::BACK);
    _cullFaceBack->setDataVariance(Object::STATIC);
    _depthWritesDisabled = new Depth(Depth::LESS, 0.0, 1.0, false);
    _depthWritesDisabled->setDataVariance(Object::STATIC);
}

osg::Image* make3DNoiseImage(int texSize)
{
    osg::Image* image = new osg::Image;
    image->setImage(texSize, texSize, texSize,
                    4, GL_RGBA, GL_UNSIGNED_BYTE,
                    new unsigned char[4 * texSize * texSize * texSize],
                    osg::Image::USE_NEW_DELETE);

    const int startFrequency = 4;
    const int numOctaves = 4;

    int f, i, j, k, inc;
    double ni[3];
    double inci, incj, inck;
    int frequency = startFrequency;
    GLubyte *ptr;
    double amp = 0.5;

    osg::notify(osg::WARN) << "creating 3D noise texture... ";

    for (f = 0, inc = 0; f < numOctaves; ++f, frequency *= 2, ++inc, amp *= 0.5)
    {
        SetNoiseFrequency(frequency);
        ptr = image->data();
        ni[0] = ni[1] = ni[2] = 0;

        inci = 1.0 / (texSize / frequency);
        for (i = 0; i < texSize; ++i, ni[0] += inci)
        {
            incj = 1.0 / (texSize / frequency);
            for (j = 0; j < texSize; ++j, ni[1] += incj)
            {
                inck = 1.0 / (texSize / frequency);
                for (k = 0; k < texSize; ++k, ni[2] += inck, ptr += 4)
                {
                    *(ptr+inc) = (GLubyte) (((noise3(ni) + 1.0) * amp) * 128.0);
                }
            }
        }
    }

    osg::notify(osg::WARN) << "DONE" << std::endl;
    return image;
}

osg::Texture3D* StateAttributeFactory::getNoiseTexture(int size)
{
    NoiseMap::iterator itr = _noises.find(size);
    if (itr != _noises.end())
        return itr->second.get();
    Texture3D* noiseTexture = new osg::Texture3D;
    noiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::REPEAT);
    noiseTexture->setImage( make3DNoiseImage(size) );
    _noises.insert(std::make_pair(size, noiseTexture));
    return noiseTexture;
}

static osg::Image* make2DNoiseNormal();
osg::Texture2D* StateAttributeFactory::getNoiseNormalMap()
{
    if (_noiseNormalTexture.valid())
        return _noiseNormalTexture.get();

    _noiseNormalTexture = new osg::Texture2D;
    _noiseNormalTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    _noiseNormalTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    _noiseNormalTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::REPEAT);
    _noiseNormalTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::REPEAT);
    _noiseNormalTexture->setImage( make2DNoiseNormal() );
    return _noiseNormalTexture;
}

// anchor the destructor into this file, to avoid ref_ptr warnings
StateAttributeFactory::~StateAttributeFactory()
{
  
}
  
/*  GIMP header image file format (RGB):  */

static unsigned int noiseNormalWidth = 64;
static unsigned int noiseNormalHeight = 64;

/*  Call this macro repeatedly.  After each use, the pixel data can be extracted  */

#define NOISE_NORMAL_PIXEL(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
data += 4; \
}
static char *noiseNormalData =
	"@Y4`CI$`BXX`>XP`>HH_AH<`B8H`?X\\`AHT`B8\\`B8\\`@H`^>I$\\>Y(^@(T_AHD`"
	"?)$]B),`DI0`BI0`?90`>),^@HX^C8@^AHP`A8<`B(<`BHD`AHD\\@HD]A(L_A(D`"
	"DXD`@8L`=HX\\?9(_CI(`D8X`AY,`?9H`?9,\\@9(_A(X_A(H^@H@\\@8L\\?X`]?)$]"
	"@I<`AY,`@I,`>9<]?Y8_CY$`D(T`@HX^@HH[B(``BY,`A90]@9<^A9P`AYT`AI<`"
	">XPZAHD^A8H`?(P_?(X`AXT`B(X`@)(`A9$`B),`BY,`AY4`@)<`@9@`A)4`AY$`"
	">H\\\\A(X_B8P_@HP]>8`\\>I(^A(``D(T`@(T`@(H]A8L]AXT_@XT^@H`_AY,`B9,`"
	"BXH`@(H]>XT[@9(`BI(`BX\\`@X\\^?I,`@9(_A)(`A8\\`A(L_@XH^@XT^@Y$`@9(_"
	"BI8`B)(`A9,`@I@`AY@`BY,`A8T^>8HX@(X]A9$`B),`A),^@),]@),_@8\\]?XDZ"
	">XDX@H@Z@XL\\?X`^@9,`B),`AY(_?Y4^AI@^B98`B90`A9$`@)(`?Y0`@90`@9(]"
	"?9(_A(X_AXH]@(H[>8`\\?)0`AY,`D(``?Y0`@9(_AY,`B)<`@I,`@),_A)4`AI0`"
	"@(P^?XL]@(P\\AI$`B),`A(``@HX^@X\\`AY$`AY,`AI(`A(``@HX`@X\\`A(``A8\\`"
	"CY(`@X\\_?XT[AX\\_C)(`AI0`?I,`?9(`@Y0`AI0`B)0`AY,`@Y$_?XT\\?H@[?X0["
	"A(`_A8`_A8`_A)4`AI<`A98`A)8^?Y4\\AYD^AY0^@XT^?X@`?8@_?HL_?H\\]@)$\\"
	"@)(`AI(`B9$`A(``?I,`?I4`A)(`BXX`@IH`A9@`C)L`BYT`A)<`?Y(\\@)$^@HX]"
	">Y,`@H`_BHT`BY$`A9,`?Y$_@HX`B(L`AHP`@XT`@HX`?X\\`?X\\`@X\\`A8\\`B(X`"
	"BXX`>HPZ>8LYB8L]CHX`@I,`?I4`A94`AY4`B),`BI(`B9,`A9(`@(T_@(L`A8P`"
	"C9@`AY4`AI4`B98`B9@`A)<_@)8]@Y8^AY(]@XX]@8@^?X4_@84`@XH`@X\\`A)(`"
	"?X`^A9$`B)(`A9$`?Y(^?9(^@Y$_AX\\_A9D^A9<_BY@`BYH`@Y0_@)$\\A9(^B(`^"
	">9@`@Y0`CH``C9$`A)(`?H`^@8T_AXH_@(4\\?8<Z?8H\\?8X\\?X`^@H`_A8\\`B8\\`"
	"@XT`>9$]?90_D)0`DX``@HX]>XT[@XT`BI(`AXT^B(L^AHT`@8X`?(X`?8\\`@Y,`"
	"CI8`A)4`@90^B90`B90`?Y4^?Y0`B)0`B(X`@HP_@(L`@HL`AHL`AXP`AHT`@X\\_"
	"?H\\]A(``AI(`@Y$_@)$\\@90^A9<_B)<`BID`B),`B),`AY(`@(X\\@(X\\AY(_C),`"
	"=),_?H``AXT_BHX_A8`_@9(`A)(`B(\\`A(X`@HX`@8\\^@9(_@I,`@Y$_A(`_A8`_"
	"@Y0`@)<`@YH`BY@`BY(`@HT\\@(P[A8\\`BI(`AXT^AXH]@XH^?HL]>(T\\>X`_@9,`"
	"B9$`?9(^?9(^BY,`BI(`?)$^>X`_B(\\`B(X`@HX`?)$^?Y0`AI0`AI$`A(\\^@8\\]"
	"@Y$_A9,`AI,_A),^@Y4]AI@`B)H`B9L`B94`A8`_AHX_@HX`?(T[?H\\]AI(`BI(`"
	"=8\\_>H\\^@8T]AXT_B(X_AY(`A9,`@Y0`B)4`AI,`A9,`AY8`AY8`A90_A),^A),^"
	"AY4`B98`A98`?Y<]?Y(ZA(\\\\AY0`A9H`@90`@Y$_A(X_@XT`@(T_?X\\`@)$_A)(`"
	"B(X`>X`\\?I,_C)(`BH``>8X]>8H_AXD`B(X`?X`^>)0[>YD]@YD^B)<^B98^AI@`"
	"A90_AI4`AI4`AI4^B)<^B)H`AIH^@Y<[@HX^?XD\\@8L^@8X`?H``@)(`AI0`B90`"
	"?9,`?Y,`@X``AX\\`BX\\`B(X_@8\\]>H\\[?H``?8\\]@8\\]A90]AY8_AY8_@Y8^A)<`"
	"B(`^CH``B)4_?)H^>YD]AI4^AY8`?9L`=Y0]?),_@9(`A(``A(``A9$`AY(`BI$_"
	"A(L_B8P`BHL`B(L`@XT`@9$`@)$`?X\\`AXP`A(H^AHP^A(`_@),[@9D]AIT`B)L]"
	"@)P`?YH`?Y8`A(``C8T`DHT`BHT`?XH_?XDZ@(X]@H`_@HX^AHX_B(X`AHX_@(P\\"
	"@(TYAI<`AYP`@Y@`@Y$`A(``@X\\_?XL[@9(_@Y$_A9$`AI(`AY,`A9,`@90`?Y(^"
	"A9$`A(X_@XL\\A(H\\AHP^AX\\`@HX^?HP[>X<`@XD`D(P`B)(`>Y@`B9@`CY8`>9H["
	"?H`^A(``AHX_A(L_@8L^@(T_@8X`@(T`A9$`A8`_BI(`B94`A98`A98`AY,`AX\\_"
	"?X`]?I,_@),_@HX^B(H]AX@^?H@[=H@ZA8\\`A)(`@9(`@H`_A(X_AX\\`A(X_@8T]"
	"@8P[@H`^@),_?H`^@(X]A8\\`A9$`@X``B)8`B)0`AY,`AY(`A9$`A9$`@I,`@Y0`"
	"@8\\^@8T]@(H[@XL\\A8T^@XT^@8T]?HP[>(T\\@8X`C8X`A8\\`>(`\\@XT^C(P`>X`\\"
	"?Y@`@)<`@Y0`@H`^@8T]@HX^@8\\^@8\\^A)8\\A9(\\B90`BI4`A9$`A8\\`B(H`AH0`"
	"@(@X@HX]A)(`AH``B8\\`B(X`@9$`>I4`BI8`AY4`@9(`?X`^@X\\_A8\\`A(``@(X]"
	"BI(`@X\\_?8X\\?8X\\@HX`A(X`@X\\`@8X`A8\\`A8\\`B(``AX\\`A8\\`AH``AY,`B94`"
	"?H`^?X`^@HX^A8\\`AY$`AH``A(``@H`_?),\\@)4`C)(`AX\\`>(T\\@HD]CHD`@HT`"
	"@ID`@9@`@)8_@I,^A)(`A9,`@90`?I,_A90_A(\\^AHX^A8T^?XD^?X@`AH@`AX4`"
	"@H@Y@(P[@)$_A(``B8\\`BH``AI,`@9<`BI0`A9,`?H`^?H\\]@H`_A(``A(``?X`^"
	"C9,`A8\\`@8X`@X``AY$`A8\\`@8T_?HX_@8DZ@XL\\AHX_AX\\`AHX_AHX_AX\\`AY$`"
	">X`]?H`^@9(`AI0`AY,`AI(`A9$`A)(`@),_?I4`BI(`BXX`?8\\]A(X_D(\\`B)(`"
	"AY,`@9(_?I$]@9(_B)4`B)<`?Y8_=I8\\AHT`A(D`A(H^@8@\\?(@Z?HL]A8\\`B8\\`"
	"B94`@Y0`?I,`@)$_AX\\`BHT`AXX`@X``B(``@8\\^?(X\\?H`^A)(`AI(`A(``@H`_"
	"B(X`@8L^?XP^A(``AY$`A8\\`@X\\`@9$`A8T^AX\\`B(``AX\\`A8T^A(P]A(P]A8T^"
	">X`]>X`]?Y$_@Y0`A)(`@(X]?XT\\@(X]@8L`>8T_A8P`BXL_?X`]@Y0_C)0`AI4^"
	"BXX`@HP]?8X\\@Y$`C)0`BI4`@)4`=I8^@XP`@XH`AXP`A8\\`@(X]@9(_AY8`B90_"
	"C9L`@I<`?),_?Y$_B(``C(X`B8P_@XT^AHP^@8T]?H\\]@)(`AI0`AY,`A9$`@Y$`"
	"@XT`@(P^?HL]@8T_@HP]@HP]@H`_@I0`B9,`B)(`AI(`A9$`@X\\_A(X_B(``BI(`"
	"@Y8`@I4`@I4`A98`@Y0`@8\\]@(X\\@8\\]@XH`=XT^@(P^BXL_@)$^?Y<]AY0^@Y(Y"
	"B(L`@8L`?(P]A(X`C(T`C(X`@X\\_?),_?Y$_@)$_AY4`B)8`@Y0_@Y0_B)4`BI$_"
	"AX\\`?8X\\=HTY?(X\\A(X_AXT_A(P]@(P\\B8P_@XT^@)$_@Y0`AI0`AH``A(X_@X\\_"
	"A)$`@)$_?H\\]?XT\\@HP]@XX]@Y$_@90`B)0`AI0`A9,`@I,`A)(`AY,`B90`BY8`"
	"AY@`@Y0`@Y0`AI<`A98`@I,`A)(`AI0`A9,`=I<`@I4`CX\\`A)(`?I4`AI(`AX\\_"
	"@H<`?(D_>XL^@(H_B84_BX,\\A88\\@(T_>YD[?I@[A)H`AID`@90^@Y$_B(``BHP^"
	"BHP_@HP]?8\\]@9,`B)0`B9$`@X\\_?X`^BXT`AHX_@Y$`@Y0`A)(`@XT^@8L\\@8T]"
	"@Y0`@9,`@)(`@Y$`AY(`BI4`AY4`@I4_AY4`A)4`@Y0`@I4`@Y0`A9,`A9$`A8`_"
	"@H`^?XT[?HPZ@)$^@I,`@9(_@9(_A)4`@YL_=)\\_@IL`D9,`A9$`?I,`B8``C8P`"
	">Y$`@8X`AHX_BHX_B(X_@X\\^@8\\^?XT\\=X\\]@(P^BHD`B8H`@8T]?8\\]@8\\^AHP]"
	"@)(`@HP]@HP]@9,`B)8`C)(`A9$`=Y4[AY(_B9$_AY(_A90_@)<`?)@_?98^?90]"
	"@HX`@(X]?X`]A90_C)D`C9@`AY8`?Y(^BXT_B8\\`AI$`@H`^@)$^@9(_@Y$_@H`_"
	"A(\\`@HT`@8X`@H\\`@8T_?XDZA8L]BH``>Y$`?)$`@)$^A9(^B),^AY0^A),\\@)$\\"
	"@I<`@I0`@Y$_AI$^BY,`BI4`A98`@)4`=X\\]@(P^B(D_AXH_@8T]?Y$_A9$`C)(`"
	"A98`B(``@8T]>8X[?8X[B(X_BI(`@9@`AI0`AY,`AY$`A9$`@I,`@I0`AI<`BY<`"
	"AY,`A9,`@I,`A9,`AY(_AY(`@Y$_?I$]AHP`@XH^@8T_@X\\`A(``A(X`B(X`C8``"
	"?HL]@(T_@HX`@HX`@8T]@XT^A8\\`AH``@9$`A)$`AI(`B90`B98`B)<`@Y8`@)4`"
	"@Y0`?),^>I,[@90\\C98`D)D`B)D`?9D`?)0`@I(`BH``BH``A9$`@I,`AI(`BY$`"
	"A(``BH``A9$`>I,]>X`\\AXT^BX\\`@I,`@I(`A9$`AHT`@XH^?H@[?H@[A(L_BHT`"
	"@HX`@H\\`@Y$`@X\\^A8T]A(P\\@HX^@)$_B9(`A8X`@XX`AY$`AHT`@X8[A80[C8H`"
	"?H\\]A)(`AI(`A8\\`A8\\`AY,`A9,`?X`]@XT`A(X`B(``B9$`AY(`A)(`@9,`?Y0`"
	"@XT`>X`]>),]@90`C),`CY$`@X\\^=X`Z>I(^@9,`B9,`BI0`AY,`A)(`A(X_AHP^"
	"@HH[BXX`AI0`>I4_?90_BH``BXT_@8P[?X\\`@X\\`AHT`A(L_@(H]?8D[?H@Y@8DZ"
	"?HD^?XP^@8\\^@X\\_AHX_AHP^@HX`?X\\`@8H`@(H_@8P`@X\\`A(X_A(P]B8T^CI(`"
	"?9(^@I,`AI(`B(``B9$`B)0`A)4`@),_@(P^@8T_@XT^A8T^@XL\\@8L^@8P`@8X`"
	"AHL`@X``@)4`A9,`B8\\`B(H]@(<[>(@Y=XXY?8`\\A)(`AI0`A9,`A(``A(X_A8T^"
	"B8\\`C(``@H`^=8`Z>I$\\BY$`CH``A(X_@HX^A(X_B8\\`BI(`AY4`A9@`A)<`A)4`"
	"@9$`@9$`A)(`AH``BH``B8\\`@X\\`?8\\`?X8Z@8L\\@HX^@(X\\@8`[@Y0_AI@`A9<_"
	"?I$]?8X[@8T\\A8`_AI$`A9$`A)(`@I4`?Y$`@)(`@H\\`@XT^@XL\\A(H^AXH`B8P`"
	"?X@`@XT`AH``A8\\`@HP]@XH^A(L_AXT`@90`@I4`@I4`@I,`@Y$_@X\\_A(``A9$`"
	"BY$`BX\\`@8T\\=8`Z?),^BY,`BY$`@8\\^@8T]A(P]AXH]AHX^A9,`A9@`@YD`@YD`"
	"A)8`@I0`@)$^@X\\^B(``B(``@X``?H``B(``BI4`B98`A),^@I,^AID`A)H`@)8]"
	"A)4`@)$^@8\\]A9$`A(``@HP]@HP]AH``?Y,`?Y0`@9$`@X\\_A8T^AHP^BHL`BXP`"
	">(HZ@HP]BX\\`B(``?Y(\\?9(^AY(`D9$`AY@`AID`@Y@`@Y8`@Y$_@H`_@Y$`@I,`"
	"B9$`BHX_A(\\^?I4`@I<`B9,`A8\\`>(\\[?XP`@8L`AHD^AHD^@XL\\@8T]@HX]@X\\^"
	"?Y8`?I,_?I$]@H\\[AX\\_B(``@Y$`?)$`A9$`AI(`AI(`A9$`B)0`BYD`BYD`B)8`"
	"AI0`AI0`@Y0`@I,`@H\\`@HP_A8L_BHL`@8X`@8X`@8X`@X\\_A(`_A8`_AX\\`AHX_"
	">),]AI,_DY0`CI<`?IH]>9H]A90]DHX^A),^@I4_@98`@98`@I,`@Y$`@9(`@9,`"
	"BI4`B8\\`A(\\^?I4`@I<`AY$`@H`_=I,]?(T`@HT`BHP`BXP`B8H`AXH_B(L`BXP`"
	"?I4`?I4^@)8_A90_B9$_A8`_?H\\]>(X]B)D`AI0`@X\\_A8\\`AX\\`AXT`A(H^A(<\\"
	"@(H[@Y$`@9,`?H``?HX_A(X`C(T`CXH`@HD`@HD_@HP_@X\\_A9,`@Y0`@9(_?X`]"
	">)4^@Y8`C)<`CI8`AY0`@)(`?H``@8\\^A8L]A(L_@(L`?HL`?XP`@8T]AX\\]BI$["
	"AY4`A9$`A8`_AX\\`A(P]@8@\\@8L^AI(`@)<`@I4_AI4\\AI@]@IH^@)8_A8\\`C(H`"
	"AH<`AXD`AX\\`AY8`A9@`@90`@8X`A(H`C(X`AHX_@8X`@X\\`AXH_A(4]?X8\\>X@\\"
	"A(X`A(L`A8L_@8L^?(T[?Y0`A9@`@I,^@X`\\A)<`@IH`@)8`A)$`B),`B)4_A9<]"
	"?Y8`AI4`BY0_AY(_?X\\`>X\\`@(``AY(`B9$`AH``@XX`@8X`@H\\`A9$`BI(`C90`"
	"@I,`@8\\^@8T]@X\\_@X\\_?XT\\?XP^@9$`@Y8`B)<`BY@`BIP`A)H`?Y0`@X\\`B8P`"
	"?(<\\?XDZA(\\^A90]@I4]?I0]@I,`AY,`AHX^@8\\]?Y(^@Y0`BI0`C9,`BI0`AY@`"
	"@X\\`@(H]A(L_A8\\`@9(`@I4`A)4`A)(`BY8`A98`?90_>H\\\\?XLZAHX\\AY(]AI4^"
	"@H\\`AI$`B8`^@HT\\>8L[=XP[@8\\^BY$`AY,`A9$`@8X`?HX`@H\\`A9$`AY(`B9$_"
	"@I(`@8X`@8X`A)$`@Y0`@)$_?H`^?)$^@H\\[A8`]AY(`A9,`?I$]>XXZ?HHY@XLY"
	"?)0Z@I4]B)<`AY8`@I,^?9,\\@I4_B)D`B)D`A9@`@I@`A9@`B9@`B94`AI0`@Y8`"
	"@Y0`@8T]A(X_B94`AI0`@9(`@H`_A(X_A8T^@(P[?8XY@)$\\B90_C)0`BI4`B94`"
	"@HD_AXT_BX\\`A(`_>H\\^>8X]@HX^C8\\_@X``@8X`?8T^?8T^@8\\^@X\\_A(\\^A8T]"
	"@HT`@8T_@HX`A)$`A9,`@I,`@9,`@I4`BI,^B9$`A8\\`@HX`?X\\`@)$^AI4^BI<_"
	"?IL\\@YD^B98`AY$`@8T]?(TZ@)$\\AI4^@I4`@I4`@I4`@Y0`A)(`@X\\^@H`^@H`^"
	"@)(`@H`_AY,`B)0`@Y$`@8\\^A(``AHT`AHT`@HP_A8`_BY8`C9@`AY(`@8L^?XD^"
	"@HD_BHT`C9$`B),`@)(`?Y$`AH``CH``@H\\`@8X`@8\\^@H`_A)(`AI(`A(``@XT^"
	"@(<[@HD]@HP_@X\\_@X\\_@Y$_A9,`B)8`CI4`B),`@X\\_@8X`@X``B)8`C)L`BYT`"
	"@9L^A9@`B)(`B(T`A8P`A(X`AY$`B90`?X\\`@X``A9$`@X\\`@(P^?XL]@8L\\@HP]"
	"?(X\\A)(`BI4`A9$`?HPZ?X`^AI$`AXX`A(T`A(L`A8P`AX\\`A(X_?XL]?8@_?XH`"
	"@HT`B(X`BX\\`AX\\`@8X`?HX`A8\\`C8\\`@HT`@HX`A(``AI(`AY,`AY,`A(``@HX`"
	"B(X_B(``AY(`AI$`@X\\^@HX]@X\\^AY(_AY(_@Y$_?H\\]?H\\]A(``B),`B)4_@Y4["
	"?Y0`@Y$`@XT`@XH`@8H`A(X`AXX`AXT_@HT`B(\\`BX``AXX`@HX`@(T_@XT`AHP`"
	"@H\\`B9,`C94`A(\\\\>HLX?8\\]@9$`?HD`?8@]@HD_A8L_A8P`@(T_?8T`@(``AI,`"
	"@9,`A8`_B(P[AHD\\?HD^?HL`@8P`AXT`@XH`A8P`A8`_AI$^A9(^A(`_@8X`?XP`"
	"BY8`BY@`BI<`AY8`A90_@Y(]A)$]A8`]A9,`@Y8`?Y8`@I4`B),`BY,`B),`@9(]"
	">HP\\?8H\\?HH\\>X@Z>HH[?8H\\?XD\\@H4Z?HH\\A(L_B(L`AHX_A9,`A9<`B9<`BI4`"
	"A8\\`B(\\`BY$`A8`]?H\\Z?I,`?Y$`>(@[@9,[B90`CI0`BY,`@I,`?),_?9(^@)$^"
	"@ID`AY,`BHX]B(H]@XH`?XP`@8P`A8P`A(D`AHP`AX\\_AX\\]A(\\\\@8T\\?XP^?HL`"
	"A),Z@Y4[@I4]@Y8^A)<_AI<`AY8`AY0`@I,`@)<`?Y@`@)8]AI$\\BI$_AY(`@9(`"
	"?8\\`@9$`@Y0`@)4`@)<`@Y@`B94`BY$`A9,`B9$`BHX_AHX^@90^@)<`@),]@(TW"
	"@H<^@80YA8DZAI$^A98`A)L`@9<`>X\\`@I<VBI@^CI4`AY(`@90`@)D`A)P`AIH^"
	"A)<_?9@`>)0`>X``AH\\`B(\\`@90^?9X_A9@`A)4`@9(`@8\\^@HX`@XT`@XT`@HP_"
	"C(`_B(H\\B(@\\B8P`AXX`@HX`@XT`B(``BI(`?X`^>8\\^?8T`AXH`D(T`C8``@8T]"
	"?(``?I0`?Y8`@90`A90]BI4`B98^@Y4[@9H`@I4`@XX]AHD\\B(H]AXT`@8X`>HX`"
	"?8X\\@XL\\BXL^C(X`A(\\^?(X\\?)$`A)8`D)0`B98`@90`?H\\]@HH[AXH]AX\\`A)(`"
	"A9(^?9(_>(X]?8H^B8H`C(D^B(P[AI,]AI,_AY0`AY8`A9,`@9(_?Y(^@)(`@9,`"
	"B9$`B(P]AX@^B(L`A8P`@(P^@8L^AXT_BXT`?XD\\>(T\\>H``?8T^@8@\\A(<Z@X4X"
	"@X8YA(P]@X\\^@Y$_@Y0_AYH`@YP`?IP`A)L`A9@`AI0`AY$`B(``A(``?HX_>8T_"
	"@9(`B(``CH``D9,`AY(`?8\\]>H\\^@)(`BY(`A9$`?X`^@(X]A8P`B8P_AHX_A(`_"
	"A9$`?I,`>9(\\>X`\\B(``C(P_B(H\\AHP]A8PXB)$\\B90_AY8`@90^?),\\?98^?IH`"
	"AY,`A8\\`AHP`AXT`@HP`@(H]@HD]B8P`D8X`AXH_@X``@IH`>Y<`>H\\\\A(H[D(<]"
	"A(`_@Y0`@I4`@I,`A9$`BI(`AY(`@8\\]?9(^@),_@I4`@Y4`A)4`@I(`@(``?8\\_"
	"A)4`B)(`CI(`CY,`AY(`?H\\]>XT]?H``AHP^@(H[?HL]@H\\`B8``BH``BH``BI(`"
	"@)(`?94`>)@^?)T`AI``AYH`@I,`@9,`A8L\\AX\\_AY(`AI(`@Y$_?Y(^@)4`A)D`"
	"@Y4`A9(`AH``A8\\`@HX`@8L^A(L_BHT`A8P`A8@]B8P`AI,`>90`=Y(]A)$]DHX`"
	"AYD`A)H`@I<`@9(`AX\\`BHT`BHL`AHD\\?HHY?HPZ?H`^?Y0`?I,`?Y$`@8X`@X\\`"
	"AI0`A9$`B(``AX\\_A(\\^@(X]?X`^@9,`A(L_?XD\\?XP^@X``AY$`AX\\`B(X_BY(`"
	"@HP`@)$_?90]?IH]@Y\\`A)T`@)D`@9D`AY$`AY$`A8\\`A(X`@XT`A(X`AH``B)(`"
	"?I,`@9$`A9$`A9$`@Y$`@H`_AH``C)(`>X`_@8L^B8@_AHD^?8LZ?)$]AI<`CI<`"
	"D(X]BHX]@XT^@8T_@X\\`AI(`AI,`@I0`B8\\`A8\\`@Y$`@9,`?H`^?8X\\@8L^AHT`"
	"AI(`A9$`A(`_A8`]A8`_A(`_@9,`@98`A8\\`@X\\`A(``A)$`@8X`?HHZ@XDZBHP\\"
	"BXH`BHP_A8L\\@HTZ@Y([?X`[?H\\\\@9(`AY0`A8``@8P`@(H_@HD_A(H^AXH_AXH_"
	">H\\^?H\\]@X\\^A)(`@Y0`A)4`B)8`BY8`?9@`@I,`B(X`B8H`A(H\\AI(`C)L`B9H`"
	"C(``A(X_?XP^?HL_@(L`AHT`AX\\`A8`_C)(`B(``A(``@9(`?X`^?HP[@8L\\AX\\`"
	"BI(`B),`AY8`B98`BI4`B)0`@Y4`?94`A)(`AI0`B94`A)4`?H`^?H\\\\AX\\_D)$`"
	"A8\\`B8\\`B8P_AXH]A(P]@(LZ?HHY@XX]A)$`@H\\`@(T_@8T_@HX^@XT^@XT^@HP]"
	"?H\\]@8\\]A)$]A90]A)<_@YD`A9@`A98`A)<`?9(^?8\\]@8\\^A8L]B8P_B(``@X`\\"
	"A)0`?I(`>8\\`?(T`@HP`B8H`C(D]C(@XA8\\`@HX^@8\\^@9(`@9(`?H\\]@8\\^A)(`"
	"CY$`BY,`AY8`B)<`B98`B)0`?Y0`=Y0^@),[AI<`BI@`@Y8`?90_@98`BY8`E)4`"
	"<Y,\\?),_@I(`AI$`AY0`@I,`@9(_B)<`@8\\^@H`_A)(`A)4`A98`A)<`@Y8`@Y8`"
	"@I,`AI(`AY0`AY8_A)H_@YL`@IH`@I4_C94`>X`\\=)0]?9@`AY,`BHP_B8D\\A8DZ"
	"B(D`@HD`?8D`?HT`A)$`B9,`B90`B)4]@9(`?X`^@)$_A)4`@Y4`?Y$_?H`^?Y0`"
	"D(``BI(`A),^@Y(]A9(^@Y$_>Y(^<Y,\\?Y4ZB)H`BID`@90^>Y0^?Y8`B90`CH\\_"
	"@X`\\@Y0_@I<`@I<`A9@`A)H`?YL`>9P\\>I$\\AHX^B8`^@9<`@)4`AHX^@XT^?)0`"
	"?XT\\A8T^BXX`B8\\`A(``@X\\_AXT_C(P`?Y4^?)0Z?I@[@I\\`AI``@YD^@Y(]@XX]"
	"AXH]@HP]@)(`@I@`AIH_B98^B),`B)(`?8X`@8@^B(<^BH``A)<`>I@^?98\\@Y<\\"
	"BY(`B(L`AX8]BX@_BXP`AXX`@9,`?Y<`AI@^BID`AY,`@(P\\?HP[A(``B8\\`B8D\\"
	"BY,`B9,`A9$`@HX`A(X`AY$`A)4`@9<`>Y8`AI0`AY(`?9(^?Y(^B8\\`B8\\`?H``"
	"A9,`B9$`C(\\`B(X`@HX]@(X\\A(P\\B(P]B)(`A(``@I,`A9@`A9@`@),]?(X\\?8T^"
	"C(``A9$`@9,`@)4`@Y0_A8`]A8T^A8P`>XT]?8D[@8@\\A(L_@8T_?XP^@8X`AY$`"
	"A(X_A(P]AXH]B(H]AXH_@HD]?HL]?(X^?Y(\\@9(]@(X\\?(HY>XL\\@H`_A8\\`B8T^"
	"B(L`@X@_?X8^@((\\A80_B8@`B8P`AHT`>I<`A)8`A9,`?8`\\?X`]BH``BXX`?XL]"
	"@Y$_AI$`B8\\`B(``A9$`A)(`A9$`B9$`AH@`A8<`A(H^AHX_@X\\_?H\\]>H`_?)0`"
	"C90`AI(`@)(`@)(`@Y$`A8T^A(P]A8P`@Y$_?X`^?H\\]@8T_A8@_A80_B(@`BX``"
	"@(X\\@Y$_A9$`A(X_A(P]A(P]AHT`AHT`?Y$_?X`^?XP^?8T^?H``@)(`A9,`B),`"
	"?XD^?(<\\?(8[@88]AX@`C(@`B(D_A(<\\>X`_@(``@Y$`@H`^@Y$`AH``@HX`?8H\\"
	"@8XZ@XX[AI$^B90`B94`B)8`AY4`AI0`?H<`@8@`AHD`B(L`AHX_@8\\^?I4`?YH`"
	"B94]@Y$_?8\\`?XX`A(\\`AXX`B(``B8``BY,`@I0`?94`@)(`B(L`BH<^BHL`B)(`"
	"@),[A)<`@Y8`@)$^@8T]AX\\`CH\\`CXX`@I(`@HT`@XT`A(``@Y0`@9,`A9,`B90`"
	"@9,`?I,`@)(`AI0`C)0`C8``AXT_@(H[@XH^@(P^@X\\_AY(`AI(`?Y$_>Y(^?I,`"
	"AI,_AI,_AI(`B),`B90`AI(`@)$_?8\\]?(``@9$`AY$`BH``B(``@X\\^@90`@9@`"
	"AI,[@8\\]?(T`?HT`@XX`AXX`AH``A9$`BXX`@9(`>Y,_?I4`AI,_BI$]AY,[@Y<\\"
	"@I@_@YD`@)8_?8`\\?XT\\A8T^B8H`AX8]@8P`@8@^@H<^AXT`AH``@H`^A(`_BI(`"
	"?Y8`?94`?I4`@Y8`B98`B90`A9$`@)$^BHL`@HP]@HX]BY$`AY0`>Y0^>I<`@YL`"
	"BI8`AI0`@Y$_@HX^A8T^A8P`?XP^>(T\\>Y(^@)(`A9,`AY(`AX\\_A(\\\\@I$\\@I,^"
	"AY0`@)$_?8\\`?X\\`A(``@X\\^@(X\\?(X\\A(H^@(P\\?)$]?I4^A9D^B9D^B)L]AILZ"
	"@98`@)4`?Y0`@)(`A)(`AY$`AHT`@XD]?8<\\?X0[@X8[B(L^B(``A8`_A8`_BH``"
	"?HX_>XT[?(\\[@)$^AI$^AI$`A(`_@I,`C(X`@H`^@9(_BI$_BI,^?98^>YD_A9@`"
	"B)0`@Y$_?H\\]@8L\\AXH_B(L`@HP`>XT_@(P\\@HP]A(X_AX\\`AHX_A(X_@X\\^A(`_"
	"@H`_?HX_?8\\_@I,`AY8_AI4^?9,\\>9$]@X\\^A8`_A9$`@Y0_@9<`A9@`BY8`D94`"
	"?Y0`?)$^?X`^A)(`AY0`AY0`A9(`@X``?XP^@8L^AXT_B8\\`B),`AY0`AY(`B(``"
	"@XT`@8P`@8X`AH``BH``B(X`A(X`@X``AHX_?I,_?Y4^BI$]C),_@9D_?I<]@H\\["
	"AI(`@)$_?H\\]@XT`BXP`BXP`A8P`>XP`BXL_C(P`BXX`B(X`AXX`A8\\`A8\\`A(``"
	"?8H^>XL^?8\\_A98`C)P`BIP`@ID`?)@`@Y8^B90`C),`A9$`?9(_?Y$_BXX`EHL`"
	"?I,`>XT]?(D[@(P^@HX`?XP^?(X^?)$`@)4`AY4`BI4`BI4`B)4`AI4`A(`_@HP]"
	"@(P\\@(P\\@HT\\@X\\^AI(`@Y0`@)(`?)$^A9$`?X`]?H\\\\B8\\`DH\\`C8\\`?Y0`=9T`"
	"@YD^@)@^?Y8_@Y8`AI0`A9(^@)$\\?I0[@9(]B)4`BI4`B9$`B9$`B9,`A9$`>XPZ"
	"?HX`@HX`AHT`AXT_AXT_A(X`@9$`?Y,`@I,^A9,`A(``A8P`AHL`AXP`A8H`@H@\\"
	"?X@`>XT_?Y$`AY$`A9(`?)0`?I,_BH``@I4_A),^AY0^C)4`BY,`A(`_?8X\\>XT]"
	"@)8`@Y@`AY@`B)8`AI$^A8T]A(P\\A8T^B)(`A)(`@H`_AHX_C(D^BHH^@H`_>I4`"
	"A)<`@90`@)(`@X``AH``B(``A9$`A)4`?X`[@Y(]A(\\^A(P\\A8T]AH``@Y$`?H\\]"
	"?X`^@X\\_B(``B8\\`B(X_A(X_@8\\^?H``@)$^@Y$`A8\\`AHT`AXT`B8\\`B9$`B(``"
	"A(L`>XT]>XT]@HP_@HX^?)$^?Y(^BX\\`@9(_@Y$_A9$`AY(`B(``AHX_AHT`AH``"
	"?Y$`@I(`AI0`AI(`AI$^A8`]A8`_AI(`AHX^A)(`@Y0`A9$`BXP`C8H`B8P`@HT`"
	"@H\\`?8T`>XL`?XH`A(L`AHP`B(``B)(`@I,`A)(`A(`_A8`_AI$`AI(`A9,`@I,`"
	"?X`]@Y(]AI$^BI$_B8`^AX\\_@X\\_@8X`@H`_A(``A8\\`A8T^A8T^AX\\_B(`^AY(]"
	"AH``>X`]>8X[@HP]A(X_@)(`@)$_B8P_AI(`AI(`AY,`AI(`A(X_@XH^A(D`B8L`"
	"A88^A(4[A(<ZA8T]AY8`A9L`@9H`?9@`A9(^A98`@9<`@),_A(X_C(X`BXP`B(D_"
	"?(P`>HL`>8H_?8H^@XH^AHP`AX\\`AI$`@)$^@8\\]@X\\_A9$`A9$`@HX^?XT\\?(T["
	"@Y0`A9,`AY(`B(``B(X_AHX_@XT`@8P`A(``@X\\`@XT^A(\\^A8`]AY(]AY,[AY,["
	"AY,`?98`>I4_AI0`B90`@Y4`@)(`A(X_BH``BI(`B)0`A98`@)(`?8X\\?XL]@8L^"
	"C(L`BHL`B8P_B9$`B98`AID`?I<`>90^C)L`A9L`?98\\=X`V?(\\YA8`_BX\\`B(H]"
	"?8T`>X`_?)(`@9,`A9,`B),`AY(_A9(^@H`_@H`_A(``AY,`AI(`@X\\_@8\\^@H`_"
	"A94`A9(`A(``@XT^A(L_@8L`@(L`@(L`@X\\`@HX^@HX]A9$`BI<`C9@`C)<`BI4`"
	"AY4`?)D`>IH`A)<`B)0`@)4`?)0`@)(`C8`]B8`\\A9(^@Y8`@)<`?98`?90_?Y0`"
	"@Y0`AI<`C)@`BY8`AI$`A(P\\@HP]@XT^AY8]@Y<\\?Y4\\?I8\\@9@`AY@`AY(_AHP]"
	"@8\\^@)(`?Y8`@I@`AI<`B98`AY0`@Y(]A9$`AI(`AI(`AI(`AY,`B)0`BI8`BY<`"
	"?HX`?HX`?XP^@(P\\@HX^@X\\`@X``A)$`AI(`@X\\^@H`^A90_BI<`BI<`B),`B(``"
	"AY$`>I4`=I8_?Y$_A(X_?Y$_?)0`?Y8`D)4_BI$]A8`]@9(_?Y0`?Y8`?Y@`@)D`"
	"?I,_@Y8`B9@`B),`A8T]@XD[A8L_B(X`@HT\\@HT\\@X\\^AI<`AIL`AI@`A)(`@HP]"
	"A8T^@Y$`@90`@9(]A),^AY(`A9$`@8\\^@8L^A(X`A(X`@8L^@8L\\A(X_AH``A8\\`"
	">XL\\?(P]@(X]@Y$_AY8`AI<`A9@`A)<`AY8`A90_A90_A90_A),^@XT^@XH^A(D`"
	"BHL`>H\\^=(\\\\@(P^B8P_A(``?Y0`?Y8`BY@`B90`AH``A(X`@HP`@(L`?HX_@9$`"
	"AX\\]AX\\]AX\\]AX\\]A(\\^A(``A)$`A)0`BH\\`B(L`@X@_@HP`?HX_>HP\\>XL\\@(P^"
	"B(L`AX\\`@X\\_@(X\\@XX]AX\\`A8\\`@HT`@XH^B(\\`B8``@XT`@8L\\A8T^A(P]@(@X"
	"@)$_@I,`A98`B)H`BIP`AYL_@9@[?9,XAY8_A9<_A9<_A)4`@Y$_@8L^A(D`B8L`"
	"CH8`?(D]=HH\\A8L_D8X`C9,`A)4`@I4`?Y0`@I(`A8\\`B(H`AH8`@H(^@H0^@X@`"
	"BY@`BY(^CH``B9,`?Y8`?I8`A)(`B(H]@HD`>XP`@(L`C8@`C8@`?HX`?)(`AI(`"
	"A),^?8`Z>X`\\@9(`B9$`B9$`B)<`AIT`AYH`A)H`@9<^@9(]AI,_B94`A98`?I4`"
	"BI$]BY,`B90`A(``@(P^?8D[?XL]@8T_@I$\\A)8^A)<_?Y@^>I@\\?I<_A9,`B8\\`"
	"@(X\\?9(^?98^?Y@`A9@`B)0`A8T^@(<[C(T`B8P_A8L]@XX]A9$`A)4`@Y8^@)8]"
	"A)<_A)$]B(X_@X\\_>X`]?)$^A8\\`BHH^@XH`?(P_?HL]B(<^AX@^?8T^?9(_B)(`"
	"BID`AYH`@ID`@)4`A(`_AI$^A),^?Y4\\@YH`@9H`?Y8_@),]AI,_BI4`BI8`A)4`"
	"@)8_@Y8`A)<`@I,`@HX`A(L_B(L^BXT`BH``C)(`BY,`A9,`@)4`@)<`@I0`A(``"
	"B9,`AY4`@Y8`@90`@Y0`A9,`@X``?8T^?8T^?8X\\?8\\]?I,`@9@`@IL`@)P`?YL`"
	"?Y<]@)$\\A(`_@H`_>8X[?(X\\@XT^B8D]AXT_@H`_@)$_AX\\`A(\\^?Y(^@I,`BY,`"
	"AY0^AYH`@YH`?I4^@)$\\A9(^AI$\\@X`Z?90_?)4_>I,]?)$]A)$]AY(_AI$^@H\\["
	"@90`@90`@9,`@9$`@H\\`@X\\_AHX_B(X_B(<`BH8`BH<^AXH]A(``@)4`?Y0`?9(`"
	"BY$`B)(`AY,`AI(`AY$`AY,`@)(`>9$`A(X_@HP]@(H[@(H[@8T]@X\\_A9$`A9$`"
	"@Y0_A)(`B)0`AY4`?I,`?)$^@H`_A(P]AHX\\A9(^A98`AI<`A98`A)4`A90_B9$_"
	"@8P[@8\\]?Y$_?)$^?X`^AH``BX\\`BHP^@(X]?H`^?9(_@90`A9,`AI,_@8`[?(TX"
	"B8L]AXH]@XT^@H\\`?I,`?I4`?I<`?I<_@X@_AH<]B88]B(@\\A8T^@9(`?I,`?)(`"
	"@8L`@8T_@8L^@XL\\A8L]AHT`@9$`>Y$`B9$`B(``AHX_A8L]AXH_B8H`B(D_B(D_"
	"B9$`B(``BY,`B)4`@9,`?9,`?Y0`?I,`A(\\\\AY(_AY8`@YD`@9<`A90_AI$^AI$^"
	"A(L`@(H_?HD^@(T_@H\\`A(L`AH<_BH4_C(T`B(\\`A9,`A98`AI<`A98`@I4`?I,_"
	"BHP^AHP^@XT^@(``@)4`@9@`@)D`?IH`@98`AY,`CI(`CH``B(`^@H`^?Y(^?I,`"
	"?8T`?(P]>X@Z?84V@80WA8@]A(X`@H\\`?90`?Y0`@I<`AI@`AY<`AY0`A9$`@XT`"
	"C(``AXT_AX\\`@X\\`?HX_?Y$`@)<`?I<`A90_BY,`B90`?Y0`?I,_AX\\_BH``AI(`"
	"A(T`@HD`@HD_A(X`A(\\`@8L`A8<`BH8`DHH`BHL`@XT^@H`_@Y$`@Y$`@I,`@Y4`"
	"A9@`A)<`@9,`@)(`@(``@8\\^@XT^A(P\\?Y@`@Y8`B90`C),`AI$^@(X\\?H\\]?X`^"
	"?H``@)(`@9,`A(``AXT_B8P_AHP^@8L^?X`^?X`^@8\\^@HX^@HX^@XT^A8T^AHX_"
	"B9$`@XT^@8T_@8T_@(P^@X\\`A9,`@90`@Y8`C)(`BY$`?Y$_?H\\]B8P_BXX`@I(`"
	">X\\`@8X`AXX`AH``@Y$`?X\\`@X\\`B8X`CXH`B(H]@8L\\?XT\\@8T_@XH^@XH^A8P`"
	"@YH`@9@`?Y0`?H`^?XP^@(H]@XD[AHD\\?Y(^@HX]AHX^B(``A8\\`?XP^?8H^?HL`"
	"@HP_A9(`AI@`B)D`BI8`B90`A8`_@(P\\AXT`A8L]@XD[@8<Y@X<XA8DZB(P[BHX]"
	"AY4`@9(`@9$`@X``A8P`B(X`B(``@XX]>Y(^B8\\`BHT`@(X]?HL]B(<^B(D_?(X`"
	"=I$\\@Y8`C)<`AI,_?I$[?),^?Y0`A9,`CX\\`B(``A)(`A9(`AH``AXH_AX8_BH8`"
	"A(\\ZA(\\\\@HX]@(X]?X\\`@9$`AI,`B)4`BI(`B8\\`BHT`BH``AI$`?X\\`>XH`>X<`"
	"BXH`AX\\`@9(`?I$]?I$[@9(]@Y0`@Y0`?HX_@9(`A98`B)D`B9H`B)D`AID`A9@`"
	"";

static osg::Image* make2DNoiseNormal() {
    osg::Image* image = new osg::Image;
    image->setImage(noiseNormalWidth, noiseNormalHeight, 1,
                    GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE,
                    new unsigned char[3 * noiseNormalWidth * noiseNormalHeight],
                    osg::Image::USE_NEW_DELETE);

    char* data = noiseNormalData;
    for (unsigned int i=0; i<noiseNormalHeight; ++i) {
        for (unsigned int j=0; j<noiseNormalWidth; ++j) {
            GLubyte *ptr = image->data(i,j);
            NOISE_NORMAL_PIXEL(data,ptr);
        }
    }

    return image;
}
}
