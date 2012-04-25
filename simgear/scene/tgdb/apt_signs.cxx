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

#include "apt_signs.hxx"

#define SIGN "OBJECT_SIGN: "

using std::vector;
using namespace simgear;

// for temporary storage of sign elements
struct element_info {
    element_info(SGMaterial *m, Effect *s, SGMaterialGlyph *g, double h, double c)
        : material(m), state(s), glyph(g), height(h), coverwidth(c)
    {
        scale = h * m->get_xsize()
                / (m->get_ysize() < 0.001 ? 1.0 : m->get_ysize());
        abswidth = c == 0 ? g->get_width() * scale : c;
    }
    SGMaterial *material;
    Effect *state;
    SGMaterialGlyph *glyph;
    double height;
    double scale;
    double abswidth;
    double coverwidth;
};

typedef std::vector<element_info*> ElementVec;

const double HT[5] = {0.460, 0.610, 0.760, 1.220, 0.760}; // standard panel height sizes
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
    {"@ul",      "^lu"},
    {"@ld",      "^ld"},
    {"@dl",      "^ld"},
    {"@r",       "^r"},
    {"@ru",      "^ru"},
    {"@ur",      "^ru"},
    {"@rd",      "^rd"},
    {"@dr",      "^rd"},
    {0, 0},
};

void SGMakeSignFace(osg::Group* object, const ElementVec& elements, double hpos, bool isBackside)
{
    std::map<Effect*, EffectGeode*> geodesByEffect;

    for (unsigned int i = 0; i < elements.size(); i++) {
        element_info *element = elements[i];

        double xoffset = element->glyph->get_left();
        double height = element->height;
        double width = element->glyph->get_width();

        osg::Vec3Array* vl = new osg::Vec3Array;
        osg::Vec2Array* tl = new osg::Vec2Array;

        // vertices
        vl->push_back(osg::Vec3(thick, hpos,            grounddist));
        vl->push_back(osg::Vec3(thick, hpos + element->abswidth, grounddist));
        vl->push_back(osg::Vec3(thick, hpos,            grounddist + height));
        vl->push_back(osg::Vec3(thick, hpos + element->abswidth, grounddist + height));

        // texture coordinates
        tl->push_back(osg::Vec2(xoffset,         0));
        tl->push_back(osg::Vec2(xoffset + width, 0));
        tl->push_back(osg::Vec2(xoffset,         1));
        tl->push_back(osg::Vec2(xoffset + width, 1));

        // normals
        osg::Vec3Array* nl = new osg::Vec3Array;
        nl->push_back(osg::Vec3(0, -1, 0));

        // colors
        osg::Vec4Array* cl = new osg::Vec4Array;
        cl->push_back(osg::Vec4(1, 1, 1, 1));

        osg::Geometry* geometry = new osg::Geometry;
        geometry->setVertexArray(vl);
        geometry->setNormalArray(nl);
        geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
        geometry->setColorArray(cl);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        geometry->setTexCoordArray(0, tl);
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl->size()));

        EffectGeode* geode = geodesByEffect[element->state];
        if (!geode) {
            geode = new EffectGeode();
            geode->setEffect(element->state);
            geodesByEffect[element->state] = geode;
        }

        geode->addDrawable(geometry);

        if (isBackside) {
            osg::MatrixTransform* transform = new osg::MatrixTransform();
            const double angle = 180;
            const osg::Vec3d axis(0, 0, 1);
            transform->setMatrix(osg::Matrix::rotate( osg::DegreesToRadians(angle), axis));
            transform->addChild(geode);
            object->addChild(transform);
        } else
            object->addChild(geode);

        hpos += element->abswidth;
        delete element;
    }
}


// see $FG_ROOT/Docs/README.scenery
//
osg::Node*
SGMakeSign(SGMaterialLib *matlib, const string& content)
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
    int size = -1;
    char oldtype = 0, newtype = 0;

    osg::Group* object = new osg::Group;
    object->setName(content);

    SGMaterial *material = 0;

    // Part I: parse & measure
    for (const char *s = content.data(); *s; s++) {
        string name;
        string value;

        if (*s == '{') {
            if (cmd)
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "Illegal taxiway sign syntax. Unexpected '{' in '" << content << "'.");
            cmd = true;
            continue;

        } else if (*s == '}') {
            if (!cmd)
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "Illegal taxiway sign syntax. Unexpected '}' in '" << content << "'.");
            cmd = false;
            continue;

        } else if (!cmd) {
            name = *s;

        } else {
            if (*s == ',')
                continue;

            for (; *s; s++) {
                name += *s;
                if (s[1] == ',' || s[1] == '}' || s[1] == '=')
                    break;
            }
            if (!*s) {
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "unclosed { in sign contents");
            } else if (s[1] == '=') {
                for (s += 2; *s; s++) {
                    value += *s;
                    if (s[1] == ',' || s[1] == '}')
                        break;
                }
                if (!*s)
                    SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "unclosed { in sign contents");
            }

            if (name == "@@") {
                isBackside = true;
                continue;
            }

            if (name == "@size") {
                sign_height = strtod(value.data(), 0);
                continue;
            }

            if (name.size() == 2 || name.size() == 3) {
                string n = name;
                if (n.size() == 3 && n[2] >= '1' && n[2] <= '5') {
                    size = n[2] - '1';
                    n = n.substr(0, 2);
                }
                if (n == "@Y") {
                    if (size < 3) {
                        sign_height = HT[size < 0 ? 2 : size];
                        newmat = "YellowSign";
                        newtype = 'Y';
                        continue;
                    }

                } else if (n == "@R") {
                    if (size < 3) {
                        sign_height = HT[size < 0 ? 2 : size];
                        newmat = "RedSign";
                        newtype = 'R';
                        continue;
                    }

                } else if (n == "@L") {
                    if (size < 3) {
                        sign_height = HT[size < 0 ? 2 : size];
                        newmat = "FramedSign";
                        newtype = 'L';
                        continue;
                    }

                } else if (n == "@B") {
                    if (size < 0 || size == 3 || size == 4) {
                        sign_height = HT[size < 0 ? 3 : size];
                        newmat = "BlackSign";
                        newtype = 'B';
                        continue;
                    }
                }
            }

            for (int i = 0; cmds[i].keyword; i++) {
                if (name == cmds[i].keyword) {
                    name = cmds[i].glyph_name;
                    break;
                }
            }

            if (name[0] == '@') {
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "ignoring unknown command `" << name << '\'');
                continue;
            }
        }

        if (newmat.size()) {
            material = matlib->find(newmat);
            newmat.clear();
        }
 
        SGMaterialGlyph *glyph = material->get_glyph(name);
        if (!glyph) {
            SG_LOG( SG_TERRAIN, SG_ALERT, SIGN "unsupported glyph '" << *s << '\'');
            continue;
        }

        // in managed mode push frame stop and frame start first
        Effect *state = material->get_effect();
        element_info *e1;
        element_info *e2;
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
                    close1 = new element_info(material, state, g, sign_height, 0);
                g = material->get_glyph("start-frame");
                if (g) {
                    e1 = new element_info(material, state, g, sign_height, 0);
                    elements1.push_back(e1);
                    total_width1 += e1->glyph->get_width() * e1->scale;
                }
            }
            // now the actually requested glyph (front)
            e1 = new element_info(material, state, glyph, sign_height, 0);
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
                    close2 = new element_info(material, state, g, sign_height, 0);
                g = material->get_glyph("start-frame");
                if (g) {
                    e2 = new element_info(material, state, g, sign_height, 0);
                    elements2.push_back(e2);
                    total_width2 += e2->glyph->get_width() * e2->scale;
                }
            }
            // now the actually requested glyph (back)
            e2 = new element_info(material, state, glyph, sign_height, 0);
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
    double hpos = -total_width1 / 2;
    double boxwidth = total_width1 > total_width2 ? total_width1/2 : total_width2/2;

    if (total_width1 < total_width2) {
        double ssize = (total_width2 - total_width1)/2;
        SGMaterial *mat = matlib->find("signcase");
        if (mat) {
            element_info* s1 = new element_info(mat, mat->get_effect(), mat->get_glyph("cover"), sign_height, ssize);
            element_info* s2 = new element_info(mat, mat->get_effect(), mat->get_glyph("cover"), sign_height, ssize);
            elements1.insert(elements1.begin(), s1);
            elements1.push_back(s2);
        }
        hpos = -total_width2 / 2;

    } else if (total_width2 < total_width1) {
        double ssize = (total_width1 - total_width2)/2;
        SGMaterial *mat = matlib->find("signcase");
        if (mat) {
            element_info* s1 = new element_info(mat, mat->get_effect(), mat->get_glyph("cover"), sign_height, ssize);
            element_info* s2 = new element_info(mat, mat->get_effect(), mat->get_glyph("cover"), sign_height, ssize);
            elements2.insert(elements2.begin(), s1);
            elements2.push_back(s2);
        }
        hpos = -total_width1 / 2;
    }

    // Create front side
    SGMakeSignFace(object, elements1, hpos, false);

    // Create back side
    SGMakeSignFace(object, elements2, hpos, true);


    // Add 3D case for signs
    osg::Vec3Array* vl = new osg::Vec3Array;
    osg::Vec2Array* tl = new osg::Vec2Array;
    osg::Vec3Array* nl = new osg::Vec3Array;

    //left side
    vl->push_back(osg::Vec3(-thick, -boxwidth,  grounddist));
    vl->push_back(osg::Vec3(thick, -boxwidth,  grounddist));
    vl->push_back(osg::Vec3(-thick, -boxwidth,  grounddist + sign_height));
    vl->push_back(osg::Vec3(thick, -boxwidth,  grounddist + sign_height));

    nl->push_back(osg::Vec3(-1, 0, 0));

    //top
    vl->push_back(osg::Vec3(-thick, -boxwidth,  grounddist + sign_height));
    vl->push_back(osg::Vec3(thick,  -boxwidth,  grounddist + sign_height));
    vl->push_back(osg::Vec3(-thick, boxwidth, grounddist + sign_height));
    vl->push_back(osg::Vec3(thick,  boxwidth, grounddist + sign_height));

    nl->push_back(osg::Vec3(0, 0, 1));

    //right side
    vl->push_back(osg::Vec3(-thick, boxwidth, grounddist + sign_height));
    vl->push_back(osg::Vec3(thick,  boxwidth, grounddist + sign_height));
    vl->push_back(osg::Vec3(-thick, boxwidth, grounddist));
    vl->push_back(osg::Vec3(thick,  boxwidth, grounddist));

    for (int i = 0; i < 4; ++i) {
        tl->push_back(osg::Vec2(1,    1));
        tl->push_back(osg::Vec2(0.75, 1));
        tl->push_back(osg::Vec2(1,    0));
        tl->push_back(osg::Vec2(0.75, 0));
    }

    nl->push_back(osg::Vec3(1, 0, 0));

    osg::Vec4Array* cl = new osg::Vec4Array;
    cl->push_back(osg::Vec4(1.0, 1.0, 1.0, 1.0));
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(vl);
    geometry->setNormalArray(nl);
    geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
    geometry->setTexCoordArray(0, tl);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl->size()));
    EffectGeode* geode = new EffectGeode;
    geode->addDrawable(geometry);
    SGMaterial *mat = matlib->find("signcase");
    if (mat)
      geode->setEffect(mat->get_effect());
    object->addChild(geode);

    return object;
}
