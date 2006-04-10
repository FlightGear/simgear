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

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/scene/tgdb/leaf.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>

#include "apt_signs.hxx"

#define TAXI "OBJECT_TAXI_SIGN: "
#define RWY "OBJECT_RUNWAY_SIGN: "


// for temporary storage of sign elements
struct element_info {
    element_info(SGMaterial *m, SGMaterialGlyph *g) : material(m), glyph(g) {
        scale = m->get_xsize() / (m->get_ysize() < 0.001 ? 1.0 : m->get_ysize());
    }
    SGMaterial *material;
    SGMaterialGlyph *glyph;
    double scale;
};


// translation table for 'command' to glyph name
struct pair {
    const char *keyword;
    const char *glyph_name;
} cmds[] = {
    {"l",       "left"},
    {"lu",      "up-left"},
    {"ld",      "down-left"},
    {"r",       "right"},
    {"ru",      "up-right"},
    {"rd",      "down-right"},
    {"u",       "up"},
    {"ul",      "up-left"},
    {"ur",      "up-right"},
    {"d",       "down"},
    {"dl",      "down-left"},
    {"dr",      "down-right"},
    {"no-exit", "no-exit"},
    {0, 0},
};


// Y ... black on yellow            -> direction, destination, boundary
// R ... white on red               -> mandatory instruction
// L ... yellow on black with frame -> runway/taxiway location
// B ... white on black             -> runway distance remaining
//
// u/d/l/r     ... up/down/left/right arrow or two-letter combinations
//                 thereof (ur, dl, ...)
//
// Example: {l}E|{L}[T]|{Y,ur}L|E{r}
//
ssgBranch *sgMakeTaxiSign( SGMaterialLib *matlib,
                           const string path, const string content )
{
    const double sign_height = 1.0;   // meter     TODO make configurable

    vector<element_info *> elements;
    double total_width = 0.0;
    bool cmd = false;
    char *newmat = "YellowSign";

    ssgBranch *object = new ssgBranch();
    object->setName((char *)content.c_str());
    SGMaterial *material = matlib->find(newmat);

    for (const char *s = content.data(); *s; s++) {
        string name;
        const char *newmat = 0;

        if (*s == '{') {
            if (cmd)
                SG_LOG(SG_TERRAIN, SG_ALERT, TAXI "unexpected { in sign contents");
            cmd = true;
            continue;
        } else if (*s == '}') {
            if (!cmd)
                SG_LOG(SG_TERRAIN, SG_ALERT, TAXI "unexpected } in sign contents");
            cmd = false;
            continue;
        } else if (cmd) {
            if (*s == ',')
                continue;
            else if (*s == 'Y')
                newmat = "YellowSign";
            else if (*s == 'R')
                newmat = "RedSign";
            else if (*s == 'L')
                newmat = "FramedSign";
            else if (*s == 'B')
                newmat = "BlackSign";
            else {
                // find longest match of cmds[]
                int maxlen = 0, len;
                for (int i = 0; cmds[i].keyword; i++) {
                    len = strlen(cmds[i].keyword);
                    if (!strncmp(s, cmds[i].keyword, len)) {
                        maxlen = len;
                        name = cmds[i].glyph_name;
                    }
                }
                if (maxlen)
                    s += maxlen - 1;
            }
            if (s[1] != ',' && s[1] != '}')
                SG_LOG(SG_TERRAIN, SG_ALERT, TAXI "garbage after command `" << s << '\'');

            if (newmat) {
                material = matlib->find(newmat);
                continue;
            }

            if (name.empty()) {
                SG_LOG(SG_TERRAIN, SG_ALERT, TAXI "unknown command `" << s << '\'');
                continue;
            }
        } else
            name = *s;

        if (!material) {
            SG_LOG( SG_TERRAIN, SG_ALERT, TAXI "material doesn't exit");
            continue;
        }

        SGMaterialGlyph *glyph = material->get_glyph(name);
        if (!glyph) {
            SG_LOG( SG_TERRAIN, SG_ALERT, TAXI "unsupported character `" << *s << '\'');
            continue;
        }

        element_info *e = new element_info(material, glyph);
        elements.push_back(e);
        total_width += glyph->get_width() * e->scale;
    }

    double hpos = -total_width / 2;

    sgVec3 normal;
    sgSetVec3(normal, 0, -1, 0);

    sgVec4 color;
    sgSetVec4(color, 1.0, 1.0, 1.0, 1.0);

    for (unsigned int i = 0; i < elements.size(); i++) {
        element_info *element = elements[i];

        double xoffset = element->glyph->get_left();
        double width = element->glyph->get_width();
        double abswidth = width * element->scale;

        // vertices
        ssgVertexArray *vl = new ssgVertexArray(4);
        vl->add(hpos,            0, 0);
        vl->add(hpos + abswidth, 0, 0);
        vl->add(hpos,            0, sign_height);
        vl->add(hpos + abswidth, 0, sign_height);

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
        ssgSimpleState *state = element->material->get_state();
        //if (!lighted)
        //    state->setMaterial(GL_EMISSION, 0, 0, 0, 1);
        leaf->setState(state);

        object->addKid(leaf);
        hpos += abswidth;
        delete element;
    }

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
