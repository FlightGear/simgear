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

#include <osg/StateSet>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>

#include "apt_signs.hxx"

#define SIGN "OBJECT_SIGN: "
#define RWY "OBJECT_RUNWAY_SIGN: "

using std::vector;
using namespace simgear;

// for temporary storage of sign elements
struct element_info {
    element_info(SGMaterial *m, Effect *s, SGMaterialGlyph *g, double h)
        : material(m), state(s), glyph(g), height(h)
    {
        scale = h * m->get_xsize()
                / (m->get_ysize() < 0.001 ? 1.0 : m->get_ysize());
    }
    SGMaterial *material;
    Effect *state;
    SGMaterialGlyph *glyph;
    double height;
    double scale;
};


// standard panel height sizes
const double HT[5] = {0.460, 0.610, 0.760, 1.220, 0.760};


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


// see $FG_ROOT/Docs/README.scenery
//
osg::Node*
SGMakeSign(SGMaterialLib *matlib, const string& path, const string& content)
{
    double sign_height = 1.0;  // meter
    bool lighted = true;
    string newmat = "BlackSign";

    vector<element_info *> elements;
    element_info *close = 0;
    double total_width = 0.0;
    bool cmd = false;
    int size = -1;
    char oldtype = 0, newtype = 0;

    osg::Group* object = new osg::Group;
    object->setName(content);

    SGMaterial *material = 0;
    Effect *lighted_state = 0;
    Effect *unlighted_state = 0;

    // Part I: parse & measure
    for (const char *s = content.data(); *s; s++) {
        string name;
        string value;

        if (*s == '{') {
            if (cmd)
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "unexpected { in sign contents");
            cmd = true;
            continue;

        } else if (*s == '}') {
            if (!cmd)
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "unexpected } in sign contents");
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

            if (name == "@size") {
                sign_height = strtod(value.data(), 0);
                continue;
            }

            if (name == "@light") {
                lighted = (value != "0" && value != "no" && value != "off" && value != "false");
                continue;
            }

            if (name == "@material") {
                newmat = value.data();
                continue;

            } else if (name.size() == 2 || name.size() == 3) {
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
            SGMaterial *m = matlib->find(newmat);
            if (!m) {
                // log error, but keep using previous material to at least show something
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "ignoring unknown material `" << newmat << '\'');
            } else {
                material = m;
                // set material states (lighted & unlighted)
                lighted_state = material->get_effect();
                newmat.append(".unlighted");

                m = matlib->find(newmat);
                if (m) {
                    unlighted_state = m->get_effect();
                } else {
                    SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "ignoring unknown material `" << newmat << '\'');
                    unlighted_state = lighted_state;
                }
            }
            newmat.clear();
        }

        // This can only happen if the default material is missing.
        // Error has been already logged in the block above.
        if (!material) continue;
 
        SGMaterialGlyph *glyph = material->get_glyph(name);
        if (!glyph) {
            SG_LOG( SG_TERRAIN, SG_ALERT, SIGN "unsupported glyph `" << *s << '\'');
            continue;
        }

        // in managed mode push frame stop and frame start first
        Effect *state = lighted ? lighted_state : unlighted_state;
        element_info *e;
        if (newtype && newtype != oldtype) {
            if (close) {
                elements.push_back(close);
                total_width += close->glyph->get_width() * close->scale;
                close = 0;
            }
            oldtype = newtype;
            SGMaterialGlyph *g = material->get_glyph("stop-frame");
            if (g)
                close = new element_info(material, state, g, sign_height);
            g = material->get_glyph("start-frame");
            if (g) {
                e = new element_info(material, state, g, sign_height);
                elements.push_back(e);
                total_width += e->glyph->get_width() * e->scale;
            }
        }
        // now the actually requested glyph
        e = new element_info(material, state, glyph, sign_height);
        elements.push_back(e);
        total_width += e->glyph->get_width() * e->scale;
    }

    // in managed mode close frame
    if (close) {
        elements.push_back(close);
        total_width += close->glyph->get_width() * close->scale;
        close = 0;
    }


    // Part II: typeset
    double hpos = -total_width / 2;
    const double dist = 0.25;        // hard-code distance from surface for now

    for (unsigned int i = 0; i < elements.size(); i++) {
        element_info *element = elements[i];

        double xoffset = element->glyph->get_left();
        double height = element->height;
        double width = element->glyph->get_width();
        double abswidth = width * element->scale;

        // vertices
        osg::Vec3Array* vl = new osg::Vec3Array;
        vl->push_back(osg::Vec3(0, hpos,            dist));
        vl->push_back(osg::Vec3(0, hpos + abswidth, dist));
        vl->push_back(osg::Vec3(0, hpos,            dist + height));
        vl->push_back(osg::Vec3(0, hpos + abswidth, dist + height));

        // texture coordinates
        osg::Vec2Array* tl = new osg::Vec2Array;
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
        EffectGeode* geode = new EffectGeode;
        geode->addDrawable(geometry);
        geode->setEffect(element->state);

        object->addChild(geode);
        hpos += abswidth;
        delete element;
    }


    // minimalistic backside
    osg::Vec3Array* vl = new osg::Vec3Array;
    vl->push_back(osg::Vec3(0, hpos,               dist));
    vl->push_back(osg::Vec3(0, hpos - total_width, dist));
    vl->push_back(osg::Vec3(0, hpos,               dist + sign_height));
    vl->push_back(osg::Vec3(0, hpos - total_width, dist + sign_height));

    osg::Vec2Array* tl = new osg::Vec2Array;
    for (int i = 0; i < 4; ++i)
        tl->push_back(osg::Vec2(0.0, 0.0));
    osg::Vec3Array* nl = new osg::Vec3Array;
    nl->push_back(osg::Vec3(0, 1, 0));
    osg::Vec4Array* cl = new osg::Vec4Array;
    cl->push_back(osg::Vec4(0.0, 0.0, 0.0, 1.0));
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
    SGMaterial *mat = matlib->find("BlackSign");
    if (mat)
      geode->setEffect(mat->get_effect());
    object->addChild(geode);

    return object;
}

osg::Node*
SGMakeRunwaySign(SGMaterialLib *matlib, const string& path, const string& name)
{
    // for demo purposes we assume each element (letter) is 1x1 meter.
    // Sign is placed 0.25 meters above the ground

    float width = name.length() / 3.0;

    osg::Vec3 corner(-width, 0, 0.25f);
    osg::Vec3 widthVec(2*width + 1, 0, 0);
    osg::Vec3 heightVec(0, 0, 1);
    osg::Geometry* geometry;
    geometry = osg::createTexturedQuadGeometry(corner, widthVec, heightVec);
    EffectGeode* geode = new EffectGeode;
    geode->setName(name);
    geode->addDrawable(geometry);
    SGMaterial *mat = matlib->find(name);
    if (mat)
      geode->setEffect(mat->get_effect());

    return geode;
}
