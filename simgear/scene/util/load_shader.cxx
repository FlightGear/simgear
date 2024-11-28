// Copyright (C) 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "load_shader.hxx"

#include <osg/Shader>
#include <osgDB/Registry>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/io/sg_mmap.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

namespace simgear {

bool
loadShaderFromUTF8File(osg::Shader *shader, const std::string &filename)
{
    if (!shader) {
        return false;
    }
    sg_ifstream inStream(SGPath::fromUtf8(filename), std::ios::in | std::ios::binary);
    if (!inStream.is_open()) {
        return false;
    }
    shader->setFileName(filename);
    shader->setShaderSource(inStream.read_all());
    return true;
}

} // namespace simgear
