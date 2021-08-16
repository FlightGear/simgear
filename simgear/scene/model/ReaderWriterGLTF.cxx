// Copyright (C) 2021  Fernando García Liñán <fernandogarcialinan@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "ReaderWriterGLTF.hxx"

#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>

#include <simgear/math/SGMath.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_USE_CPP14

#include "tiny_gltf.h"

namespace simgear {

struct GLTFBuilder {
    const tinygltf::Model &model;
    const SGReaderWriterOptions *opts;
    std::vector<osg::ref_ptr<osg::Array>> arrays;

    GLTFBuilder(const tinygltf::Model &model_,
                const SGReaderWriterOptions *opts_) :
        model(model_),
        opts(opts_) {
        extractArrays();
    }

    osg::Node *makeModel() const {
        osg::MatrixTransform* transform = new osg::MatrixTransform;
        // In glTF +Y is up, but we use +Z up
        transform->setMatrix(osg::Matrixd::rotate(osg::Vec3d(0.0, 1.0, 0.0),
                                                  osg::Vec3d(0.0, 0.0, 1.0)));

        for (const auto &scene : model.scenes) {
            for (const int nodeIndex : scene.nodes) {
                osg::Node *node = makeNode(model.nodes[nodeIndex]);
                if (node)
                    transform->addChild(node);
            }
        }

        return transform;
    }

    osg::Node *makeNode(const tinygltf::Node &node) const {
        osg::MatrixTransform* mt = new osg::MatrixTransform;
        // This is the name used later by animations
        mt->setName(node.name);

        if (node.matrix.size() == 16) {
            osg::Matrixd mat;
            mat.set(node.matrix.data());
            mt->setMatrix(mat);
        } else {
            osg::Matrixd scale, translation, rotation;
            if (node.scale.size() == 3) {
                scale = osg::Matrixd::scale(node.scale[0],
                                            node.scale[1],
                                            node.scale[2]);
            }
            if (node.rotation.size() == 4) {
                osg::Quat quat(node.rotation[0],
                               node.rotation[1],
                               node.rotation[2],
                               node.rotation[3]);
                rotation.makeRotate(quat);
            }
            if (node.translation.size() == 3) {
                translation = osg::Matrixd::translate(node.translation[0],
                                                      node.translation[1],
                                                      node.translation[2]);
            }
            mt->setMatrix(scale * rotation * translation);
        }

        if (node.mesh >= 0) {
            osg::Group *mesh = makeMesh(model.meshes[node.mesh]);
            if (mesh)
                mt->addChild(mesh);
        }

        for (const int nodeIndex : node.children) {
            osg::Node *child = makeNode(model.nodes[nodeIndex]);
            if (child)
                mt->addChild(child);
        }

        return mt;
    }

    osg::Group *makeMesh(const tinygltf::Mesh &mesh) const {
        osg::Group *group = new osg::Group;

        for (const auto &primitive : mesh.primitives) {
            if (primitive.indices < 0) {
                // Currently we don't support primitives without index data.
                // The glTF spec says that we should be using drawArrays to
                // render geometry without index data.
                continue;
            }

            EffectGeode *geode = new EffectGeode;
            group->addChild(geode);

            SGPropertyNode_ptr effectRoot = new SGPropertyNode;
            makeChild(effectRoot, "inherits-from")->setStringValue("Effects/model-pbr");
            if (primitive.material >= 0) {
                // We have a material assigned to the primitive, add all the
                // required material info as parameters to the Effect.
                makeMaterialParameters(makeChild(effectRoot, "parameters"),
                                       model.materials[primitive.material]);
            }
            Effect *effect = makeEffect(effectRoot, true, opts);
            if (effect)
                geode->setEffect(effect);

            osg::Geometry *geom = new osg::Geometry;
            geode->addDrawable(geom);
            geom->setDataVariance(osg::Object::STATIC);
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);

            for (const auto &attr : primitive.attributes) {
                if (attr.first == "POSITION") {
                    geom->setVertexArray(arrays[attr.second].get());
                } else if (attr.first == "NORMAL") {
                    geom->setNormalArray(arrays[attr.second].get());
                } else if (attr.first == "TEXCOORD_0") {
                    geom->setTexCoordArray(0, arrays[attr.second].get());
                } else if (attr.first == "TEXCOORD_1") {
                    geom->setTexCoordArray(1, arrays[attr.second].get());
                } else if (attr.first == "COLOR_0") {
                    geom->setColorArray(arrays[attr.second].get());
                }
            }

            int mode = -1;
            switch (primitive.mode) {
            case TINYGLTF_MODE_TRIANGLES:      mode = GL_TRIANGLES;      break;
            case TINYGLTF_MODE_TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
            case TINYGLTF_MODE_TRIANGLE_FAN:   mode = GL_TRIANGLE_FAN;   break;
            case TINYGLTF_MODE_POINTS:         mode = GL_POINTS;         break;
            case TINYGLTF_MODE_LINE:           mode = GL_LINES;          break;
            case TINYGLTF_MODE_LINE_LOOP:      mode = GL_LINE_LOOP;      break;
            default: break;
            }

            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                auto *indices = static_cast<osg::UShortArray *>(
                    arrays[primitive.indices].get());
                auto *drawElements = new osg::DrawElementsUShort(
                    mode, indices->begin(), indices->end());
                geom->addPrimitiveSet(drawElements);
            } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                auto *indices = static_cast<osg::UIntArray *>(
                    arrays[primitive.indices].get());
                auto *drawElements = new osg::DrawElementsUInt(
                    mode, indices->begin(), indices->end());
                geom->addPrimitiveSet(drawElements);
            } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                auto *indices = static_cast<osg::UByteArray *>(
                    arrays[primitive.indices].get());
                auto *drawElements = new osg::DrawElementsUByte(
                    mode, indexAccessor.count);
                std::copy(indices->begin(), indices->end(), drawElements->begin());
                geom->addPrimitiveSet(drawElements);
            }

            // Compute the normals if the file doesn't already contain them
            if (!geom->getNormalArray()) {
                osgUtil::SmoothingVisitor sv;
                geode->accept(sv);
            }

            // Generate tangent vectors etc if needed
            geode->runGenerators(geom);
        }

        return group;
    }

    bool makeMaterialParameters(SGPropertyNode *params,
                                const tinygltf::Material &material) const {
        const tinygltf::PbrMetallicRoughness &pbr = material.pbrMetallicRoughness;

        makeChild(params, "base-color-factor")->setValue(
            SGVec4d(pbr.baseColorFactor[0],
                    pbr.baseColorFactor[1],
                    pbr.baseColorFactor[2],
                    pbr.baseColorFactor[3]));
        makeChild(params, "metallic-factor")->setValue(pbr.metallicFactor);
        makeChild(params, "roughness-factor")->setValue(pbr.roughnessFactor);
        makeChild(params, "emissive-factor")->setValue(
            SGVec3d(material.emissiveFactor[0],
                    material.emissiveFactor[1],
                    material.emissiveFactor[2]));

        SGPropertyNode *blendNode = makeChild(params, "blend");
        if (material.alphaMode == "OPAQUE") {
            makeChild(blendNode, "active")->setValue(false);
            makeChild(params, "rendering-hint")->setStringValue("opaque");
            makeChild(params, "alpha-cutoff")->setValue(-1.0);
        } else if (material.alphaMode == "MASK") {
            makeChild(blendNode, "active")->setValue(false);
            makeChild(params, "rendering-hint")->setStringValue("opaque");
            makeChild(params, "alpha-cutoff")->setValue(material.alphaCutoff);
        } else if (material.alphaMode == "BLEND") {
            makeChild(blendNode, "active")->setValue(true);
            makeChild(params, "rendering-hint")->setStringValue("transparent");
            makeChild(params, "alpha-cutoff")->setValue(-1.0);
        }

        std::string cullFaceString;
        if (material.doubleSided) {
            cullFaceString = "off";
        } else {
            cullFaceString = "back";
        }
        makeChild(params, "cull-face")->setStringValue(cullFaceString);

        SGPropertyNode *baseColorTexNode = makeChild(params, "texture", 0);
        if (!makeTextureParameters(baseColorTexNode, pbr.baseColorTexture.index))
            makeChild(baseColorTexNode, "type")->setValue("white");
        SGPropertyNode *normalTexNode = makeChild(params, "texture", 1);
        if (!makeTextureParameters(normalTexNode, material.normalTexture.index))
            makeChild(normalTexNode, "type")->setValue("null-normalmap");
        SGPropertyNode *metRoughTexNode = makeChild(params, "texture", 2);
        if (!makeTextureParameters(metRoughTexNode, pbr.metallicRoughnessTexture.index))
            makeChild(metRoughTexNode, "type")->setValue("white");
        SGPropertyNode *occlusionTexNode = makeChild(params, "texture", 3);
        if (!makeTextureParameters(occlusionTexNode, material.occlusionTexture.index))
            makeChild(occlusionTexNode, "type")->setValue("white");
        SGPropertyNode *emissiveTexNode = makeChild(params, "texture", 4);
        if (!makeTextureParameters(emissiveTexNode,  material.emissiveTexture.index))
            makeChild(emissiveTexNode, "type")->setValue("white");

        makeChild(params, "flip-vertically")->setValue(true);

        return true;
    }

    bool makeTextureParameters(SGPropertyNode *texNode, int textureIndex) const {
        if (textureIndex < 0) {
            // The material doesn't define this texture
            return false;
        }

        const tinygltf::Texture &texture = model.textures[textureIndex];
        const tinygltf::Image &image = model.images[texture.source];

        if (tinygltf::IsDataURI(image.uri) || image.image.size() > 0) {
            // This is an embedded image
            // Since we rely on the Effects framework to load the images from
            // a file for us, we can't support these for now.
            SG_LOG(SG_INPUT, SG_DEV_ALERT, "glTF loader: embedded images are not supported");
            return false;
        }

        // This is an URI to a external image
        std::string absFileName = osgDB::findDataFile(image.uri, opts);
        if (absFileName.empty()) {
            SG_LOG(SG_INPUT, SG_ALERT, "glTF loader: could not find external texture '"
                   << image.uri << "'");
            return false;
        }

        makeChild(texNode, "type")->setValue("2d");
        makeChild(texNode, "image")->setStringValue(absFileName);
        // Use default sampler settings
        makeChild(texNode, "filter")->setStringValue("linear-mipmap-linear");
        makeChild(texNode, "mag-filter")->setStringValue("linear");
        makeChild(texNode, "wrap-s")->setStringValue("repeat");
        makeChild(texNode, "wrap-t")->setStringValue("repeat");

        return true;
    }

    // Create an OSG array from a glTF accessor
    // Copied from osgEarth's glTF reader plugin
    template<typename OSGArray, int ComponentType, int AccessorType>
    class ArrayBuilder {
    public:
        static OSGArray* makeArray(unsigned int size) {
            return new OSGArray(size);
        }

        static void copyData(OSGArray* dest, const unsigned char* src, size_t viewOffset,
                             size_t byteStride,  size_t accessorOffset, size_t count) {
            int32_t componentSize = tinygltf::GetComponentSizeInBytes(ComponentType);
            int32_t numComponents = tinygltf::GetNumComponentsInType(AccessorType);
            if (byteStride == 0) {
                memcpy(&(*dest)[0], src + accessorOffset + viewOffset, componentSize * numComponents * count);
            } else {
                const unsigned char* ptr = src + accessorOffset + viewOffset;
                for (size_t i = 0; i < count; ++i, ptr += byteStride) {
                    memcpy(&(*dest)[i], ptr, componentSize * numComponents);
                }
            }
        }

        static void copyData(OSGArray* dest, const tinygltf::Buffer& buffer, const tinygltf::BufferView& bufferView,
                             const tinygltf::Accessor& accessor) {
            copyData(dest, &buffer.data.at(0), bufferView.byteOffset,
                     bufferView.byteStride, accessor.byteOffset, accessor.count);
        }

        static OSGArray* makeArray(const tinygltf::Buffer& buffer, const tinygltf::BufferView& bufferView,
                                   const tinygltf::Accessor& accessor) {
            OSGArray* result = new OSGArray(accessor.count);
            copyData(result, buffer, bufferView, accessor);
            return result;
        }
    };

    // Take all of the accessors and turn them into arrays
    // Copied from osgEarth's glTF reader plugin
    void extractArrays() {
        for (const auto &accessor : model.accessors) {
            const auto &bufferView = model.bufferViews[accessor.bufferView];
            const auto &buffer = model.buffers[bufferView.buffer];

            osg::ref_ptr<osg::Array> osgArray;

            switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::ByteArray,
                                            TINYGLTF_COMPONENT_TYPE_BYTE,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2bArray,
                                            TINYGLTF_COMPONENT_TYPE_BYTE,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3bArray,
                                            TINYGLTF_COMPONENT_TYPE_BYTE,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4bArray,
                                            TINYGLTF_COMPONENT_TYPE_BYTE,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::UByteArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2ubArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3ubArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4ubArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::ShortArray,
                                            TINYGLTF_COMPONENT_TYPE_SHORT,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2sArray,
                                            TINYGLTF_COMPONENT_TYPE_SHORT,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3sArray,
                                            TINYGLTF_COMPONENT_TYPE_SHORT,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4sArray,
                                            TINYGLTF_COMPONENT_TYPE_SHORT,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::UShortArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2usArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3usArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4usArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_INT:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::IntArray,
                                            TINYGLTF_COMPONENT_TYPE_INT,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2uiArray,
                                            TINYGLTF_COMPONENT_TYPE_INT,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3uiArray,
                                            TINYGLTF_COMPONENT_TYPE_INT,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4uiArray,
                                            TINYGLTF_COMPONENT_TYPE_INT,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::UIntArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2iArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3iArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4iArray,
                                            TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                switch (accessor.type) {
                case TINYGLTF_TYPE_SCALAR:
                    osgArray = ArrayBuilder<osg::FloatArray,
                                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                                            TINYGLTF_TYPE_SCALAR>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC2:
                    osgArray = ArrayBuilder<osg::Vec2Array,
                                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                                            TINYGLTF_TYPE_VEC2>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC3:
                    osgArray = ArrayBuilder<osg::Vec3Array,
                                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                                            TINYGLTF_TYPE_VEC3>::makeArray(buffer, bufferView, accessor);
                    break;
                case TINYGLTF_TYPE_VEC4:
                    osgArray = ArrayBuilder<osg::Vec4Array,
                                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                                            TINYGLTF_TYPE_VEC4>::makeArray(buffer, bufferView, accessor);
                    break;
                default:
                    break;
                }
            default:
                break;
            }
            if (osgArray.valid()) {
                osgArray->setBinding(osg::Array::BIND_PER_VERTEX);
                osgArray->setNormalize(accessor.normalized);
            }
            arrays.push_back(osgArray);
        }
    }
};

ReaderWriterGLTF::ReaderWriterGLTF()
{
    supportsExtension("gltf", "glTF ASCII loader");
    supportsExtension("glb", "glTF binary loader");
}

ReaderWriterGLTF::~ReaderWriterGLTF()
{
}

const char *ReaderWriterGLTF::className() const
{
    return "glTF loader";
}

osgDB::ReaderWriter::ReadResult
ReaderWriterGLTF::readNode(const std::string &location,
                           const osgDB::Options *options) const
{
    std::string ext = osgDB::getFileExtension(location);
    if (!acceptsExtension(ext))
        return ReadResult::FILE_NOT_HANDLED;

    const auto sgOpts = dynamic_cast<const SGReaderWriterOptions *>(options);
    if (!sgOpts) {
        // We need a SGReaderWriterOptions for Effects, so we can't parse this
        // properly. I'm not sure if this can happen at all though.
        return ReadResult::FILE_NOT_FOUND;
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ret;

    if (ext == "gltf")
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, location);
    else if (ext == "glb")
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, location);
    else
        return ReadResult::FILE_NOT_HANDLED;

    if (!warn.empty())
        SG_LOG(SG_INPUT, SG_WARN, "glTF loader: " << warn);
    if (!err.empty())
        SG_LOG(SG_INPUT, SG_ALERT, "glTF loader: " << err);
    if (!ret)
        return osgDB::ReaderWriter::ReadResult::ERROR_IN_READING_FILE;

    GLTFBuilder builder(model, sgOpts);
    return builder.makeModel();
}

} // namespace simgear
