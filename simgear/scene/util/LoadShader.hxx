// SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <string>

namespace osg {
class Shader;
}

namespace simgear {

/**
 * @brief Load a shader from an UTF-8 path.
 * This is a workaround for osg::Shader::loadShaderFromSourceFile() not
 * respecting UTF-8 paths, even when OSG_USE_UTF8_FILENAME is set.
 *
 * @param shader Must be a valid osg::Shader, otherwise loading will fail.
 * @param filename UTF-8 file path. This path must be absolute, i.e. not
 *                 relative to $FG_ROOT. Use loadShaderFromDataFile() if you
 *                 need to use a path relative to $FG_ROOT.
 * @return True if successful, false otherwise.
 */
bool loadShaderFromUTF8Path(osg::Shader* shader, const std::string& filename);

/**
 * @brief Load a shader from a data file in $FG_ROOT.
 *
 * @param shader Must be a valid osg::Shader, otherwise loading will fail.
 * @param filename UTF-8 file path relative to $FG_ROOT.
 * @return True if successful, false otherwise.
 */
bool loadShaderFromDataFile(osg::Shader* shader, const std::string& filename);

} // namespace simgear

// These functions are used by ShaderVG to retrieve the shader sources from
// files in $FG_ROOT.
extern "C" {
void *sgShaderVGShaderOpen(const char *filename, const char **buf, int *size);
void sgShaderVGShaderClose(void *ptr);
}
