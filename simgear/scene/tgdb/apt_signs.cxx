// apt_signs.cxx -- build airport signs on the fly
//
// Written by Curtis Olson, started July 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <vector>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/StateSet>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include "apt_signs.hxx"

#define SIGN "OBJECT_SIGN: "

using std::vector;
using std::string;
using namespace simgear;

// for temporary storage of sign elements
struct element_info {
    element_info(SGMaterial *m, SGMaterialGlyph *g, double h, double c)
        : material(m), glyph(g), height(h), coverwidth(c)
    {
        scale = h * m->get_xsize()
                / (m->get_ysize() < 0.001 ? 1.0 : m->get_ysize());
        abswidth = c == 0 ? g->get_width() * scale : c;
    }
    SGMaterial *material;
    SGMaterialGlyph *glyph;
    double height;
    double scale;
    double abswidth;
    double coverwidth;
};

typedef std::vector<element_info*> ElementVec;

// Standard panel height sizes. The first value is unused to
// make sure the size value equals 1:1 the value from the apt.dat file:
const double HT[6] = {0.1, 0.460, 0.610, 0.760, 1.220, 0.760};

const double grounddist = 0.2;     // hard-code sign distance from surface for now
const double thick = 0.1;    // half the thickness of the 3D sign


// translation table for "command" to "glyph name"
struct pair {
    const char *keyword;
    const char *glyph_name;
} cmds[] = {
    {"@u",       "^u"},
    {"@d",       "^d"},
    {"@l",       "^l"},
    {"@lu",      "^lu"},
    {"@ld",      "^ld"},
    {"@r",       "^r"},
    {"@ru",      "^ru"},
    {"@rd",      "^rd"},
    {"r1",       "^I1"},
    {"r2",       "^I2"},
    {"r3",       "^I3"},
    {0, 0},
};

struct GlyphGeometry
{
  osg::DrawArrays* quads;
  osg::Vec2Array* uvs;
  osg::Vec3Array* vertices;
  osg::Vec3Array* normals;

  void addGlyph(SGMaterialGlyph* glyph, double x, double y, double width, double height, const osg::Matrix& xform)
  {

    vertices->push_back(xform.preMult(osg::Vec3(thick, x, y)));
    vertices->push_back(xform.preMult(osg::Vec3(thick, x + width, y)));
    vertices->push_back(xform.preMult(osg::Vec3(thick, x + width, y + height)));
    vertices->push_back(xform.preMult(osg::Vec3(thick, x, y + height)));

    // texture coordinates
    double xoffset = glyph->get_left();
    double texWidth = glyph->get_width();

    uvs->push_back(osg::Vec2(xoffset,         0));
    uvs->push_back(osg::Vec2(xoffset + texWidth, 0));
    uvs->push_back(osg::Vec2(xoffset + texWidth, 1));
    uvs->push_back(osg::Vec2(xoffset,         1));

    // normals
    for (int i=0; i<4; ++i)
      normals->push_back(xform.preMult(osg::Vec3(0, -1, 0)));

    quads->setCount(vertices->size());
  }

  void addSignCase(double caseWidth, double caseHeight, const osg::Matrix& xform)
  {
    int last = vertices->size();
    double texsize = caseWidth / caseHeight;

    //left
    vertices->push_back(osg::Vec3(-thick, -caseWidth,  grounddist));
    vertices->push_back(osg::Vec3(thick, -caseWidth,  grounddist));
    vertices->push_back(osg::Vec3(thick, -caseWidth,  grounddist + caseHeight));
    vertices->push_back(osg::Vec3(-thick, -caseWidth,  grounddist + caseHeight));

    uvs->push_back(osg::Vec2(1,    1));
    uvs->push_back(osg::Vec2(0.75, 1));
    uvs->push_back(osg::Vec2(0.75, 0));
    uvs->push_back(osg::Vec2(1,    0));

    for (int i=0; i<4; ++i)
      normals->push_back(osg::Vec3(-1, 0.0, 0));

    //top
    vertices->push_back(osg::Vec3(-thick, -caseWidth,  grounddist + caseHeight));
    vertices->push_back(osg::Vec3(thick,  -caseWidth,  grounddist + caseHeight));
    vertices->push_back(osg::Vec3(thick,  caseWidth, grounddist + caseHeight));
    vertices->push_back(osg::Vec3(-thick, caseWidth, grounddist + caseHeight));

    uvs->push_back(osg::Vec2(1,    texsize));
    uvs->push_back(osg::Vec2(0.75, texsize));
    uvs->push_back(osg::Vec2(0.75, 0));
    uvs->push_back(osg::Vec2(1,    0));

    for (int i=0; i<4; ++i)
      normals->push_back(osg::Vec3(0, 0, 1));

    //right
    vertices->push_back(osg::Vec3(-thick, caseWidth, grounddist + caseHeight));
    vertices->push_back(osg::Vec3(thick,  caseWidth, grounddist + caseHeight));
    vertices->push_back(osg::Vec3(thick,  caseWidth, grounddist));
    vertices->push_back(osg::Vec3(-thick, caseWidth, grounddist));

    uvs->push_back(osg::Vec2(1,    1));
    uvs->push_back(osg::Vec2(0.75, 1));
    uvs->push_back(osg::Vec2(0.75, 0));
    uvs->push_back(osg::Vec2(1,    0));

    for (int i=0; i<4; ++i)
      normals->push_back(osg::Vec3(1, 0.0, 0));


  // transform all the newly added vertices and normals by the matrix
    for (unsigned int i=last; i<vertices->size(); ++i) {
      (*vertices)[i]= xform.preMult((*vertices)[i]);
      (*normals)[i] = xform.preMult((*normals)[i]);
    }

    quads->setCount(vertices->size());
  }
};

typedef std::map<Effect*, GlyphGeometry*> EffectGeometryMap;

GlyphGeometry* makeGeometry(Effect* eff, osg::Group* group)
{
  GlyphGeometry* gg = new GlyphGeometry;

  EffectGeode* geode = new EffectGeode;
  geode->setEffect(eff);

  gg->vertices = new osg::Vec3Array;
  gg->normals = new osg::Vec3Array;
  gg->uvs = new osg::Vec2Array;

  osg::Vec4Array* cl = new osg::Vec4Array;
  cl->push_back(osg::Vec4(1, 1, 1, 1));

  osg::Geometry* geometry = new osg::Geometry;
  geometry->setVertexArray(gg->vertices);
  geometry->setNormalArray(gg->normals);
  geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  geometry->setColorArray(cl);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->setTexCoordArray(0, gg->uvs);

  gg->quads = new osg::DrawArrays(GL_QUADS, 0, gg->vertices->size());
  geometry->addPrimitiveSet(gg->quads);
  geode->addDrawable(geometry);
  group->addChild(geode);
  return gg;
}

// see $FG_ROOT/Docs/README.scenery

namespace simgear
{

class AirportSignBuilder::AirportSignBuilderPrivate
{
public:
    SGMaterialLib* materials;
    EffectGeometryMap geometries;
    osg::MatrixTransform* signsGroup;
    GlyphGeometry* signCaseGeometry;

    GlyphGeometry* getGeometry(Effect* eff)
    {
        EffectGeometryMap::iterator it = geometries.find(eff);
        if (it != geometries.end()) {
            return it->second;
        }

        GlyphGeometry* gg = makeGeometry(eff, signsGroup);
        geometries[eff] = gg;
        return gg;
    }

    void makeFace(const ElementVec& elements, double hpos, const osg::Matrix& xform)
    {
        for (auto element : elements) {
            GlyphGeometry* gg = getGeometry(element->material->get_effect());
            gg->addGlyph(element->glyph, hpos, grounddist, element->abswidth, element->height, xform);
            hpos += element->abswidth;
            delete element;
        }
    }

};

AirportSignBuilder::AirportSignBuilder(SGMaterialLib* mats, const SGGeod& center) :
    d(new AirportSignBuilderPrivate)
{
    d->signsGroup = new osg::MatrixTransform;
    d->signsGroup->setMatrix(makeZUpFrame(center));

    assert(mats);
    d->materials = mats;
    d->signCaseGeometry = d->getGeometry(d->materials->find("signcase", center)->get_effect());
}

osg::Node* AirportSignBuilder::getSignsGroup()
{
    if (0 == d->signsGroup->getNumChildren())
        return 0;
    return d->signsGroup;
}

AirportSignBuilder::~AirportSignBuilder()
{
    EffectGeometryMap::iterator it;
    for (it = d->geometries.begin(); it != d->geometries.end(); ++it) {
        delete it->second;
    }
}

void AirportSignBuilder::addSign(const SGGeod& pos, double heading, const std::string& content, int size)
{
    double sign_height = 1.0;  // meter
    string newmat = "BlackSign";
    ElementVec elements1, elements2;
    element_info *close1 = 0;
    element_info *close2 = 0;
    double total_width1 = 0.0;
    double total_width2 = 0.0;
    bool cmd = false;
    bool isBackside = false;
    char oldtype = 0, newtype = 0;
    SGMaterial *material = 0;

    if (size < -1 || size > 5){
        SG_LOG(SG_TERRAIN, SG_INFO, SIGN "Found illegal sign size value of '" << size << "' for " << content << ".");
        size = -1;
    }

    // Part I: parse & measure
    for (const char *s = content.data(); *s; s++) {
        string name;
        string value;

        if (*s == '{') {
            if (cmd)
                SG_LOG(SG_TERRAIN, SG_INFO, SIGN "Illegal taxiway sign syntax. Unexpected '{' in '" << content << "'.");
            cmd = true;
            continue;

        } else if (*s == '}') {
            if (!cmd)
                SG_LOG(SG_TERRAIN, SG_INFO, SIGN "Illegal taxiway sign syntax. Unexpected '}' in '" << content << "'.");
            cmd = false;
            continue;

        } else if (!cmd) {
            name = *s;

        } else {
            if (*s == ',')
                continue;

            for (; *s; s++) {
                name += *s;
                if (s[1] == ',' || s[1] == '}' )
                    break;
            }

            if (!*s) {
                SG_LOG(SG_TERRAIN, SG_INFO, SIGN "unclosed { in sign contents");
            } else if (s[1] == '=') {
                for (s += 2; *s; s++) {
                    value += *s;
                    if (s[1] == ',' || s[1] == '}')
                        break;
                }
                if (!*s)
                    SG_LOG(SG_TERRAIN, SG_INFO, SIGN "unclosed { in sign contents");
            }

            if (name == "no-entry") {
                sign_height = HT[size < 0 ? 3 : size];
                newmat = "RedSign";
                newtype = 'R';
            }

            else if (name == "critical") {
                sign_height = HT[size < 0 ? 3 : size];
                newmat = "SpecialSign";
                newtype = 'S';
            }

            else if (name == "safety") {
                sign_height = HT[size < 0 ? 3 : size];
                newmat = "SpecialSign";
                newtype = 'S';
            }

            else if (name == "hazard") {
                sign_height = HT[size < 0 ? 3 : size];
                newmat = "SpecialSign";
                newtype = 'S';
            }

            if (name == "@@") {
                isBackside = true;
                continue;
            }

            if (name.size() == 2 || name.size() == 3) {
                string n = name;
                if (n.size() == 3 && n[2] >= '1' && n[2] <= '5') {
                    size = n[2];
                    n = n.substr(0, 2);
                }
                if (n == "@Y") {
                    if (size > 3) {
                        size = -1;
                        SG_LOG(SG_TERRAIN, SG_INFO, SIGN << content << " has wrong size. Allowed values are 1 to 3");
                    }
                    sign_height = HT[size < 0 ? 3 : size];
                    newmat = "YellowSign";
                    newtype = 'Y';
                    continue;

                } else if (n == "@R") {
                    if (size > 3) {
                        size = -1;
                        SG_LOG(SG_TERRAIN, SG_INFO, SIGN << content << " has wrong size. Allowed values are 1 to 3");
                    }
                    sign_height = HT[size < 0 ? 3 : size];
                    newmat = "RedSign";
                    newtype = 'R';
                    continue;

                } else if (n == "@L") {
                    if (size > 3) {
                        size = -1;
                        SG_LOG(SG_TERRAIN, SG_INFO, SIGN << content << " has wrong size. Allowed values are 1 to 3");
                    }
                    sign_height = HT[size < 0 ? 3 : size];
                    newmat = "FramedSign";
                    newtype = 'L';
                    continue;

                } else if (n == "@B") {
                    if ( (size != -1) && (size != 4) && (size != 5) ) {
                        size = -1;
                        SG_LOG(SG_TERRAIN, SG_INFO, SIGN << content << " has wrong size. Allowed values are 4 or 5");
                    }
                    sign_height = HT[size < 0 ? 4 : size];
                    newmat = "BlackSign";
                    newtype = 'B';
                    continue;
                }
            }

            for (int i = 0; cmds[i].keyword; i++) {
                if (name == cmds[i].keyword) {
                    name = cmds[i].glyph_name;
                    break;
                }
            }

            if (name[0] == '@') {
                SG_LOG(SG_TERRAIN, SG_INFO, SIGN "ignoring unknown command `" << name << '\'');
                continue;
            }
        }

        if (! newmat.empty()) {
            material = d->materials->find(newmat, pos);
            newmat.clear();
        }

        SGMaterialGlyph *glyph = material->get_glyph(name);
        if (!glyph) {
            SG_LOG( SG_TERRAIN, SG_INFO, SIGN "unsupported glyph '" << *s << '\'');
            continue;
        }

    // in managed mode push frame stop and frame start first
        if (!isBackside) {
            if (newtype && newtype != oldtype) {
                if (close1) {
                    elements1.push_back(close1);
                    total_width1 += close1->glyph->get_width() * close1->scale;
                    close1 = 0;
                }
                oldtype = newtype;
                SGMaterialGlyph *g = material->get_glyph("stop-frame");
                if (g)
                    close1 = new element_info(material, g, sign_height, 0);
                g = material->get_glyph("start-frame");
                if (g) {
                    element_info* e1 = new element_info(material, g, sign_height, 0);
                    elements1.push_back(e1);
                    total_width1 += e1->glyph->get_width() * e1->scale;
                }
            }
            // now the actually requested glyph (front)
            element_info* e1 = new element_info(material, glyph, sign_height, 0);
            elements1.push_back(e1);
            total_width1 += e1->glyph->get_width() * e1->scale;
        } else {
            if (newtype && newtype != oldtype) {
                if (close2) {
                    elements2.push_back(close2);
                    total_width2 += close2->glyph->get_width() * close2->scale;
                    close2 = 0;
                }
                oldtype = newtype;
                SGMaterialGlyph *g = material->get_glyph("stop-frame");
                if (g)
                    close2 = new element_info(material, g, sign_height, 0);
                g = material->get_glyph("start-frame");
                if (g) {
                    element_info* e2 = new element_info(material, g, sign_height, 0);
                    elements2.push_back(e2);
                    total_width2 += e2->glyph->get_width() * e2->scale;
                }
            }
            // now the actually requested glyph (back)
            element_info* e2 = new element_info(material, glyph, sign_height, 0);
            elements2.push_back(e2);
            total_width2 += e2->glyph->get_width() * e2->scale;
        }
    }

    // in managed mode close frame
    if (close1) {
        elements1.push_back(close1);
        total_width1 += close1->glyph->get_width() * close1->scale;
        close1 = 0;
    }

    if (close2) {
        elements2.push_back(close2);
        total_width2 += close2->glyph->get_width() * close2->scale;
        close2 = 0;
    }

  // Part II: typeset
    double boxwidth = std::max(total_width1, total_width2) * 0.5;
    double hpos = -boxwidth;
    SGMaterial *mat = d->materials->find("signcase", pos);

    double coverSize = fabs(total_width1 - total_width2) * 0.5;
    element_info* s1 = new element_info(mat, mat->get_glyph("cover1"), sign_height, coverSize);
    element_info* s2 = new element_info(mat, mat->get_glyph("cover2"), sign_height, coverSize);

    if (total_width1 < total_width2) {
        elements1.insert(elements1.begin(), s1);
        elements1.push_back(s2);
    } else if (total_width2 < total_width1) {
        elements2.insert(elements2.begin(), s1);
        elements2.push_back(s2);
    } else {
        delete s1;
        delete s2;
    }

// position the sign
    const osg::Vec3 Z_AXIS(0, 0, 1);
    osg::Matrix m(makeZUpFrame(pos));
    m.preMultRotate(osg::Quat(SGMiscd::deg2rad(heading), Z_AXIS));

    // apply the inverse of the group transform, so sign vertices
    // are relative to the tile center, and hence have a magnitude which
    // fits in a float with sufficent precision.
    m.postMult(d->signsGroup->getInverseMatrix());

    d->makeFace(elements1, hpos, m);
// Create back side
    osg::Matrix back(m);
    back.preMultRotate(osg::Quat(M_PI, Z_AXIS));
    d->makeFace(elements2, hpos, back);

    d->signCaseGeometry->addSignCase(boxwidth, sign_height, m);
}

} // of namespace simgear
