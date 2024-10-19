// Copyright (C) 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <string>

namespace osg {
class Shader;
}

namespace simgear {

// Load a shader from an UTF-8 path.
// This is a workaround for osg::Shader::loadShaderFromSourceFile not respecting
// UTF-8 paths, even when OSG_USE_UTF8_FILENAME is set.
bool loadShaderFromUTF8File(osg::Shader *shader, const std::string &filename);
// Load a shader from a data file in $FG_ROOT.
bool loadShaderFromDataFile(osg::Shader *shader, const std::string &filename);

} // namespace simgear

extern "C" {

// Used by ShaderVG to find the shader source files in $FG_ROOT
void *simgearShaderOpen(const char *filename, const char **buf, int *size);
void  simgearShaderClose(void *ptr);

} // extern "C"
