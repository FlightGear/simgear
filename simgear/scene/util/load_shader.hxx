// Copyright (C) 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <string>

namespace osg {
class Shader;
}

namespace simgear {

// Load a shader from a UTF-8 filename.
// This is a workaround for osg::Shader::loadShaderFromSourceFile not respecting
// UTF-8 paths, even when OSG_USE_UTF8_FILENAME is set.
bool loadShaderFromUTF8File(osg::Shader *shader, const std::string &filename);

} // namespace simgear
