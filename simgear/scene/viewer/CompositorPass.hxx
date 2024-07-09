// Copyright (C) 2018 - 2023 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <unordered_map>

#include <osg/Camera>
#include <osg/Vec2f>
#include <osg/Vec2i>
#include <osg/View>

#include <simgear/props/condition.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/structure/Singleton.hxx>
#include <simgear/props/props.hxx>

namespace simgear {

class SGReaderWriterOptions;

namespace compositor {

class Compositor;

/**
 * A Pass encapsulates a single render operation. In an OSG context, this is
 * best represented as a Camera attached to the Viewer as a slave camera.
 *
 * Passes can render directly to the framebuffer or to a texture via FBOs. Also,
 * the OpenGL state can be modified via the Effects framework and by exposing RTT
 * textures from previous passes.
 *
 * Every pass can be enabled and disabled via a property tree conditional
 * expression. This allows dynamic rendering pipelines where features can be
 * enabled or disabled in a coherent way by the user.
 */
struct Pass : public osg::Referenced {
    Pass() :
        collect_lights(false),
        useMastersSceneData(true),
        cull_mask(0xffffff),
        inherit_cull_mask(false),
        render_once(false),
        viewport_x_scale(0.0f),
        viewport_y_scale(0.0f),
        viewport_width_scale(0.0f),
        viewport_height_scale(0.0f) {}

    int                              render_order;
    std::string                      name;
    std::string                      type;
    bool                             collect_lights;
    std::string                      effect_scheme;
    osg::ref_ptr<osg::Camera>        camera;
    bool                             useMastersSceneData;
    osg::Node::NodeMask              cull_mask;
    /** Whether the cull mask is ANDed with the view master camera cull mask. */
    bool                             inherit_cull_mask;
    bool                             render_once;
    float                            viewport_x_scale;
    float                            viewport_y_scale;
    float                            viewport_width_scale;
    float                            viewport_height_scale;
    SGSharedPtr<SGCondition>         render_condition;
    std::string                      multiview;

    osg::ref_ptr<osg::Drawable>      compute_node;
    osg::Vec2i                       compute_wg_size;
    osg::Vec2f                       compute_global_scale;

    struct PassUpdateCallback : public virtual osg::Referenced {
    public:
        virtual void updatePass(Pass &pass,
                                const osg::Matrix &view_matrix,
                                const osg::Matrix &proj_matrix) = 0;
    };

    osg::ref_ptr<PassUpdateCallback> update_callback;
};

class PassBuilder : public SGReferenced {
public:
    virtual ~PassBuilder() {}

    /**
     * \brief Build a pass.
     *
     * By default, this function implements commonly used features such as
     * input/output buffers, conditional support etc., but can be safely ignored
     * and overrided for more special passes.
     *
     * @param compositor The Compositor instance that owns the pass.
     * @param root The root node of the pass property tree.
     * @return A Pass or a null pointer if an error occurred.
     */
    virtual Pass *build(Compositor *compositor, const SGPropertyNode *root,
                        const SGReaderWriterOptions *options);

    static PassBuilder *find(const std::string &type) {
        auto itr = PassBuilderMapSingleton::instance()->_map.find(type);
        if (itr == PassBuilderMapSingleton::instance()->_map.end())
            return 0;
        return itr->second.ptr();
    }
protected:
    typedef std::unordered_map<std::string, SGSharedPtr<PassBuilder>> PassBuilderMap;
    struct PassBuilderMapSingleton : public Singleton<PassBuilderMapSingleton> {
        PassBuilderMap _map;
    };
    template <typename T>
    friend struct RegisterPassBuilder;
};

/**
 * An instance of this type registers a new pass type T with a name.
 * A global instance of this class must be created in CompositorPass.cxx to
 * register a new pass type.
 */
template <typename T>
struct RegisterPassBuilder {
    RegisterPassBuilder(const std::string &name) {
        PassBuilder::PassBuilderMapSingleton::instance()->
            _map.insert(std::make_pair(name, new T));
    }
};

/**
 * \brief Create a pass from a property tree definition.
 *
 * @param comp The Compositor instance that owns the pass.
 * @param node The root node of the pass property tree.
 * @return A Pass or a null pointer if an error occurred.
 */
Pass *buildPass(Compositor *compositor, const SGPropertyNode *root,
                const SGReaderWriterOptions *options);

} // namespace compositor
} // namespace simgear
