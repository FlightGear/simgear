// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Richard Kaszeta <bofh@me.umn.edu>

/**
 * @file
 * @brief Dump a copy of the OpenGL screen buffer to a file
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <osg/Image>
#include <osgDB/WriteFile>

#include "screen-dump.hxx"


// dump the screen buffer to a png file, returns true on success
bool sg_glDumpWindow(const char *filename, int win_width, int win_height) {
  osg::ref_ptr<osg::Image> img(new osg::Image);
  img->readPixels(0,0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE);
  return osgDB::writeImageFile(*img, filename);
}

