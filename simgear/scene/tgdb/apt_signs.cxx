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

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/scene/tgdb/leaf.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>

#include "apt_signs.hxx"

#define SIGN "OBJECT_SIGN: "
#define RWY "OBJECT_RUNWAY_SIGN: "

SG_USING_STD(vector);

// for temporary storage of sign elements
struct element_info {
    element_info(SGMaterial *m, ssgSimpleState *s, SGMaterialGlyph *g, double h)
        : material(m), state(s), glyph(g), height(h)
    {
        scale = h * m->get_xsize()
                / (m->get_ysize() < 0.001 ? 1.0 : m->get_ysize());
    }
    SGMaterial *material;
    ssgSimpleState *state;
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
    {"@u",       "up"},
    {"@d",       "down"},
    {"@l",       "left"},
    {"@lu",      "left-up"},
    {"@ul",      "left-up"},
    {"@ld",      "left-down"},
    {"@dl",      "left-down"},
    {"@r",       "right"},
    {"@ru",      "right-up"},
    {"@ur",      "right-up"},
    {"@rd",      "right-down"},
    {"@dr",      "right-down"},
    {0, 0},
};


// see $FG_ROOT/Docs/README.scenery
//
ssgBranch *sgMakeSign(SGMaterialLib *matlib, const string path, const string content)
{
    double sign_height = 1.0;  // meter
    bool lighted = true;
    const char *newmat = "BlackSign";

    vector<element_info *> elements;
    element_info *close = 0;
    double total_width = 0.0;
    bool cmd = false;
    int size = -1;
    char oldtype = 0, newtype = 0;

    ssgBranch *object = new ssgBranch();
    object->setName((char *)content.c_str());

    SGMaterial *material;
    ssgSimpleState *lighted_state;
    ssgSimpleState *unlighted_state;

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

        if (newmat) {
            material = matlib->find(newmat);
            if (!material) {
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "ignoring unknown material `" << newmat << '\'');
                continue;
            }

            // set material states (lighted & unlighted)
            lighted_state = material->get_state();
            string u = string(newmat) + ".unlighted";

            SGMaterial *m = matlib->find(u);
            if (m) {
                unlighted_state = (ssgSimpleState *)m->get_state()->clone(SSG_CLONE_STATE);
                unlighted_state->setTexture(lighted_state->getTexture());
            } else {
                SG_LOG(SG_TERRAIN, SG_ALERT, SIGN "ignoring unknown material `" << u << '\'');
                unlighted_state = lighted_state;
            }
            newmat = 0;
        }

        SGMaterialGlyph *glyph = material->get_glyph(name);
        if (!glyph) {
            SG_LOG( SG_TERRAIN, SG_ALERT, SIGN "unsupported glyph `" << *s << '\'');
            continue;
        }

        // in managed mode push frame stop and frame start first
        ssgSimpleState *state = lighted ? lighted_state : unlighted_state;
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

    sgVec3 normal;
    sgSetVec3(normal, 0, -1, 0);

    sgVec4 color;
    sgSetVec4(color, 1.0, 1.0, 1.0, 1.0);

    for (unsigned int i = 0; i < elements.size(); i++) {
        element_info *element = elements[i];

        double xoffset = element->glyph->get_left();
        double height = element->height;
        double width = element->glyph->get_width();
        double abswidth = width * element->scale;

        // vertices
        ssgVertexArray *vl = new ssgVertexArray(4);
        vl->add(0, hpos,            dist);
        vl->add(0, hpos + abswidth, dist);
        vl->add(0, hpos,            dist + height);
        vl->add(0, hpos + abswidth, dist + height);

        // texture coordinates
        ssgTexCoordArray *tl = new ssgTexCoordArray(4);
        tl->add(xoffset,         0);
        tl->add(xoffset + width, 0);
        tl->add(xoffset,         1);
        tl->add(xoffset + width, 1);

        // normals
        ssgNormalArray *nl = new ssgNormalArray(1);
        nl->add(normal);

        // colors
        ssgColourArray *cl = new ssgColourArray(1);
        cl->add(color);

        ssgLeaf *leaf = new ssgVtxTable(GL_TRIANGLE_STRIP, vl, nl, tl, cl);
        leaf->setState(element->state);

        object->addKid(leaf);
        hpos += abswidth;
        delete element;
    }


    // minimalistic backside
    ssgVertexArray *vl = new ssgVertexArray(4);
    vl->add(0, hpos,               dist);
    vl->add(0, hpos - total_width, dist);
    vl->add(0, hpos,               dist + sign_height);
    vl->add(0, hpos - total_width, dist + sign_height);

    ssgNormalArray *nl = new ssgNormalArray(1);
    nl->add(0, 1, 0);

    ssgLeaf *leaf = new ssgVtxTable(GL_TRIANGLE_STRIP, vl, nl, 0, 0);
    SGMaterial *mat = matlib->find("BlackSign");
    if (mat)
        leaf->setState(mat->get_state());
    object->addKid(leaf);

    return object;
}





ssgBranch *sgMakeRunwaySign( SGMaterialLib *matlib,
                             const string path, const string name )
{
    // for demo purposes we assume each element (letter) is 1x1 meter.
    // Sign is placed 0.25 meters above the ground

    ssgBranch *object = new ssgBranch();
    object->setName( (char *)name.c_str() );

    double width = name.length() / 3.0;

    string material = name;

    point_list nodes;
    point_list normals;
    point_list texcoords;
    int_list vertex_index;
    int_list normal_index;
    int_list tex_index;

    nodes.push_back( Point3D( -width, 0, 0.25 ) );
    nodes.push_back( Point3D( width + 1, 0, 0.25 ) );
    nodes.push_back( Point3D( -width, 0, 1.25 ) );
    nodes.push_back( Point3D( width + 1, 0, 1.25 ) );

    normals.push_back( Point3D( 0, -1, 0 ) );

    texcoords.push_back( Point3D( 0, 0, 0 ) );
    texcoords.push_back( Point3D( 1, 0, 0 ) );
    texcoords.push_back( Point3D( 0, 1, 0 ) );
    texcoords.push_back( Point3D( 1, 1, 0 ) );

    vertex_index.push_back( 0 );
    vertex_index.push_back( 1 );
    vertex_index.push_back( 2 );
    vertex_index.push_back( 3 );

    normal_index.push_back( 0 );
    normal_index.push_back( 0 );
    normal_index.push_back( 0 );
    normal_index.push_back( 0 );

    tex_index.push_back( 0 );
    tex_index.push_back( 1 );
    tex_index.push_back( 2 );
    tex_index.push_back( 3 );

    ssgLeaf *leaf = sgMakeLeaf( path, GL_TRIANGLE_STRIP, matlib, material,
                                nodes, normals, texcoords,
                                vertex_index, normal_index, tex_index,
                                false, NULL );

    object->addKid( leaf );

    return object;
}
