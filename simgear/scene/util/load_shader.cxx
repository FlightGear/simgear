// Copyright (C) 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "load_shader.hxx"

#include <osg/Shader>
#include <osgDB/Registry>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/io/sg_mmap.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/modellib.hxx>

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

bool
loadShaderFromDataFile(osg::Shader *shader, const std::string &filename)
{
    std::string file = SGModelLib::findDataFile(filename);
    if (file.empty()) {
        return false;
    }
    return loadShaderFromUTF8File(shader, file);
}

} // namespace simgear

extern "C" {

using namespace simgear;

void *
simgearShaderOpen(const char *filename, const char **buf, int *size)
{
    SGPath path("Shaders/ShaderVG/");
    path.append(filename);

    std::string file = SGModelLib::findDataFile(path);
    if (file.empty()) {
        simgear::reportFailure(simgear::LoadFailure::NotFound,
                               simgear::ErrorCode::LoadEffectsShaders,
                               "Could not find ShaderVG shader",
                               path);
        return NULL;
    }
    SGMMapFile *mmap = new SGMMapFile(file);
    if (!mmap->open(SG_IO_IN)) {
        simgear::reportFailure(simgear::LoadFailure::NotFound,
                               simgear::ErrorCode::LoadEffectsShaders,
                               "Failed to read ShaderVG shader source code",
                               path);
        return NULL;
    }
    *buf = mmap->get();
    *size = mmap->get_size();
    return mmap;
}

void
simgearShaderClose(void *ptr)
{
    SGMMapFile *mmap = (SGMMapFile *)ptr;
    delete mmap;
}

} // extern "C"
