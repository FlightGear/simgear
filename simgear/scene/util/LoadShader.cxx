// SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "LoadShader.hxx"

#include <osg/Shader>
#include <osgDB/Registry>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/io/sg_mmap.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/modellib.hxx>

namespace simgear {

bool loadShaderFromUTF8Path(osg::Shader* shader, const std::string& filename)
{
    if (!shader) {
        return false;
    }
    sg_ifstream inStream(SGPath::fromUtf8(filename), std::ios::in | std::ios::binary);
    if (!inStream.is_open()) {
        reportFailure(LoadFailure::BadData,
                      ErrorCode::LoadEffectsShaders,
                      "Failed to read shader source code",
                      SGPath::fromUtf8(filename));
        return false;
    }
    shader->setFileName(filename);
    shader->setShaderSource(inStream.read_all());

    // Defines are reset by setShaderSource()
    // Set some builtin shader defines that don't need to be imported
    shader->getShaderDefines().insert("FG_NUM_VIEWS");
    shader->getShaderDefines().insert("FG_VIEW_GLOBAL");
    switch (shader->getType()) {
    case osg::Shader::VERTEX:
        shader->getShaderDefines().insert("FG_VIEW_ID/*VERT*/");
        break;
    case osg::Shader::GEOMETRY:
        shader->getShaderDefines().insert("FG_VIEW_ID/*GEOM*/");
        break;
    case osg::Shader::FRAGMENT:
        shader->getShaderDefines().insert("FG_VIEW_ID/*FRAG*/");
        break;
    default:
        break;
    }
    return true;
}

bool loadShaderFromDataFile(osg::Shader* shader, const std::string& filename)
{
    std::string file = SGModelLib::findDataFile(filename);
    if (file.empty()) {
        reportFailure(LoadFailure::NotFound,
                      ErrorCode::LoadEffectsShaders,
                      "Could not locate shader: " + filename);
        return false;
    }
    return loadShaderFromUTF8Path(shader, file);
}

} // namespace simgear

using namespace simgear;

void *sgShaderVGShaderOpen(const char *filename, const char **buf, int *size)
{
    SGPath path("Shaders/ShaderVG/");
    path.append(filename);

    std::string file = SGModelLib::findDataFile(path);
    if (file.empty()) {
        reportFailure(LoadFailure::NotFound,
                      ErrorCode::LoadEffectsShaders,
                      "Could not find ShaderVG shader",
                      path);
        return NULL;
    }
    SGMMapFile *mmap = new SGMMapFile(file);
    if (!mmap->open(SG_IO_IN)) {
        reportFailure(LoadFailure::NotFound,
                      ErrorCode::LoadEffectsShaders,
                      "Failed to read ShaderVG shader source code",
                      path);
        return NULL;
    }
    *buf = mmap->get();
    *size = mmap->get_size();
    return mmap;
}

void sgShaderVGShaderClose(void *ptr)
{
    SGMMapFile *mmap = (SGMMapFile *)ptr;
    delete mmap;
}
