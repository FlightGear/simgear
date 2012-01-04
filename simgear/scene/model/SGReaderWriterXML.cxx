// Copyright (C) 2007 Tim Moore timoore@redhat.com
// Copyright (C) 2008 Till Busch buti@bux.at
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <algorithm>
//yuck
#include <cstring>
#include <cassert>

#include <boost/bind.hpp>

#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Switch>
#include <osgDB/FileNameUtils>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "modellib.hxx"
#include "SGReaderWriterXML.hxx"

#include "animation.hxx"
#include "particles.hxx"
#include "model.hxx"
#include "SGText.hxx"
#include "SGMaterialAnimation.hxx"

using namespace std;
using namespace simgear;
using namespace osg;

static osg::Node *
sgLoad3DModel_internal(const SGPath& path,
                       const osgDB::Options* options,
                       SGPropertyNode *overlay = 0);


SGReaderWriterXML::SGReaderWriterXML()
{
    supportsExtension("xml", "SimGear xml database format");
}

SGReaderWriterXML::~SGReaderWriterXML()
{
}

const char* SGReaderWriterXML::className() const
{
    return "XML database reader";
}

osgDB::ReaderWriter::ReadResult
SGReaderWriterXML::readNode(const std::string& fileName,
                            const osgDB::Options* options) const
{
    osg::Node *result=0;
    try {
        SGPath p = SGModelLib::findDataFile(fileName);
        if (!p.exists()) {
          return ReadResult::FILE_NOT_FOUND;
        }
        
        result=sgLoad3DModel_internal(p, options);
    } catch (const sg_exception &t) {
        SG_LOG(SG_INPUT, SG_ALERT, "Failed to load model: " << t.getFormattedMessage()
          << "\n\tfrom:" << fileName);
        result=new osg::Node;
    }
    if (result)
        return result;
    else
        return ReadResult::FILE_NOT_HANDLED;
}

class SGSwitchUpdateCallback : public osg::NodeCallback
{
public:
    SGSwitchUpdateCallback(SGCondition* condition) :
            mCondition(condition) {}
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
        assert(dynamic_cast<osg::Switch*>(node));
        osg::Switch* s = static_cast<osg::Switch*>(node);

        if (mCondition && mCondition->test()) {
            s->setAllChildrenOn();
            // note, callback is responsible for scenegraph traversal so
            // should always include call traverse(node,nv) to ensure
            // that the rest of cullbacks and the scene graph are traversed.
            traverse(node, nv);
        } else
            s->setAllChildrenOff();
    }

private:
    SGSharedPtr<SGCondition> mCondition;
};


// Little helper class that holds an extra reference to a
// loaded 3d model.
// Since we clone all structural nodes from our 3d models,
// the database pager will only see one single reference to
// top node of the model and expire it relatively fast.
// We attach that extra reference to every model cloned from
// a base model in the pager. When that cloned model is deleted
// this extra reference is deleted too. So if there are no
// cloned models left the model will expire.
namespace {
class SGDatabaseReference : public osg::Observer
{
public:
    SGDatabaseReference(osg::Referenced* referenced) :
        mReferenced(referenced)
    { }
    virtual void objectDeleted(void*)
    {
        mReferenced = 0;
    }
private:
    osg::ref_ptr<osg::Referenced> mReferenced;
};

void makeEffectAnimations(PropertyList& animation_nodes,
                          PropertyList& effect_nodes)
{
    for (PropertyList::iterator itr = animation_nodes.begin();
         itr != animation_nodes.end();
         ++itr) {
        SGPropertyNode_ptr effectProp;
        SGPropertyNode* animProp = itr->ptr();
        SGPropertyNode* typeProp = animProp->getChild("type");
        if (!typeProp)
            continue;
        const char* typeString = typeProp->getStringValue();
        if (!strcmp(typeString, "material")) {
            effectProp
                = SGMaterialAnimation::makeEffectProperties(animProp);
        } else if (!strcmp(typeString, "shader")) {
            
            SGPropertyNode* shaderProp = animProp->getChild("shader");
            if (!shaderProp || strcmp(shaderProp->getStringValue(), "chrome"))
                continue;
            *itr = 0;           // effect replaces animation
            SGPropertyNode* textureProp = animProp->getChild("texture");
            if (!textureProp)
                continue;
            effectProp = new SGPropertyNode();
            makeChild(effectProp.ptr(), "inherits-from")
                ->setValue("Effects/chrome");
            SGPropertyNode* paramsProp = makeChild(effectProp.get(), "parameters");
            makeChild(paramsProp, "chrome-texture")
                ->setValue(textureProp->getStringValue());
        }
        if (effectProp.valid()) {
            PropertyList objectNameNodes = animProp->getChildren("object-name");
            for (PropertyList::iterator objItr = objectNameNodes.begin(),
                     end = objectNameNodes.end();
                 objItr != end;
                 ++objItr)
                effectProp->addChild("object-name")
                    ->setStringValue((*objItr)->getStringValue());
            effect_nodes.push_back(effectProp);

        }
    }
    animation_nodes.erase(remove_if(animation_nodes.begin(),
                                    animation_nodes.end(),
                                    !boost::bind(&SGPropertyNode_ptr::valid,
                                                 _1)),
                          animation_nodes.end());
}
}

static osg::Node *
sgLoad3DModel_internal(const SGPath& path,
                       const osgDB::Options* dbOptions,
                       SGPropertyNode *overlay)
{
    if (!path.exists()) {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to load file: \"" << path.str() << "\"");
      return NULL;
    }

    osg::ref_ptr<SGReaderWriterOptions> options;
    options = SGReaderWriterOptions::copyOrCreate(dbOptions);
    
    SGPath modelpath(path);
    SGPath texturepath(path);
    SGPath modelDir(modelpath.dir());
    
    SGSharedPtr<SGPropertyNode> prop_root = options->getPropertyNode();
    if (!prop_root.valid())
        prop_root = new SGPropertyNode;
    osg::ref_ptr<SGModelData> data = options->getModelData();
    
    osg::ref_ptr<osg::Node> model;
    osg::ref_ptr<osg::Group> group;
    SGPropertyNode_ptr props = new SGPropertyNode;

    // Check for an XML wrapper
    if (modelpath.extension() == "xml") {
       try {
            readProperties(modelpath.str(), props);
        } catch (const sg_exception &t) {
            SG_LOG(SG_INPUT, SG_ALERT, "Failed to load xml: "
                   << t.getFormattedMessage());
            throw;
        }
        if (overlay)
            copyProperties(overlay, props);

        if (props->hasValue("/path")) {
            string modelPathStr = props->getStringValue("/path");
            modelpath = SGModelLib::findDataFile(modelPathStr, NULL, modelDir);
            if (modelpath.isNull())
                throw sg_io_exception("Model file not found: '" + modelPathStr + "'",
                        path.str());

            if (props->hasValue("/texture-path")) {
                string texturePathStr = props->getStringValue("/texture-path");
                texturepath = SGModelLib::findDataFile(texturePathStr, NULL, modelDir);
                if (texturepath.isNull())
                    throw sg_io_exception("Texture file not found: '" + texturePathStr + "'",
                            path.str());
            }
        } else {
            model = new osg::Node;
        }

        SGPropertyNode *mp = props->getNode("multiplay");
        if (mp && prop_root && prop_root->getParent())
            copyProperties(mp, prop_root);
    } else {
        // model without wrapper
    }

    options->setPropertyNode(prop_root);
    
    // Assume that textures are in
    // the same location as the XML file.
    if (!model) {
        if (!texturepath.extension().empty())
            texturepath = texturepath.dir();

        options->setDatabasePath(texturepath.str());
        osgDB::ReaderWriter::ReadResult modelResult;
        modelResult = osgDB::readNodeFile(modelpath.str(), options.get());
        if (!modelResult.validNode())
            throw sg_io_exception("Failed to load 3D model:" + modelResult.message(),
                                  modelpath.str());
        model = copyModel(modelResult.getNode());
        // Add an extra reference to the model stored in the database.
        // That is to avoid expiring the object from the cache even if
        // it is still in use. Note that the object cache will think
        // that a model is unused if the reference count is 1. If we
        // clone all structural nodes here we need that extra
        // reference to the original object
        SGDatabaseReference* databaseReference;
        databaseReference = new SGDatabaseReference(modelResult.getNode());
        model->addObserver(databaseReference);

        // Update liveries
        TextureUpdateVisitor liveryUpdate(options->getDatabasePathList());
        model->accept(liveryUpdate);

        // Copy the userdata fields, still sharing the boundingvolumes,
        // but introducing new data for velocities.
        UserDataCopyVisitor userDataCopyVisitor;
        model->accept(userDataCopyVisitor);
    }
    model->setName(modelpath.str());

    bool needTransform=false;
    // Set up the alignment node if needed
    SGPropertyNode *offsets = props->getNode("offsets", false);
    if (offsets) {
        needTransform=true;
        osg::MatrixTransform *alignmainmodel = new osg::MatrixTransform;
        alignmainmodel->setDataVariance(osg::Object::STATIC);
        osg::Matrix res_matrix;
        res_matrix.makeRotate(
            offsets->getFloatValue("pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
            osg::Vec3(0, 1, 0),
            offsets->getFloatValue("roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
            osg::Vec3(1, 0, 0),
            offsets->getFloatValue("heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
            osg::Vec3(0, 0, 1));

        osg::Matrix tmat;
        tmat.makeTranslate(offsets->getFloatValue("x-m", 0.0),
                           offsets->getFloatValue("y-m", 0.0),
                           offsets->getFloatValue("z-m", 0.0));
        alignmainmodel->setMatrix(res_matrix*tmat);
        group = alignmainmodel;
    }
    if (!group) {
        group = new osg::Group;
    }
    group->addChild(model.get());

    // Load sub-models
    vector<SGPropertyNode_ptr> model_nodes = props->getChildren("model");
    for (unsigned i = 0; i < model_nodes.size(); i++) {
        SGPropertyNode_ptr sub_props = model_nodes[i];

        SGPath submodelpath;
        osg::ref_ptr<osg::Node> submodel;
        
        string subPathStr = sub_props->getStringValue("path");
        SGPath submodelPath = SGModelLib::findDataFile(subPathStr, 
          NULL, modelDir);

        if (submodelPath.isNull()) {
          SG_LOG(SG_INPUT, SG_ALERT, "Failed to load file: \"" << subPathStr << "\"");
          continue;
        }

        try {
            submodel = sgLoad3DModel_internal(submodelPath, options.get(),
                                              sub_props->getNode("overlay"));
        } catch (const sg_exception &t) {
            SG_LOG(SG_INPUT, SG_ALERT, "Failed to load submodel: " << t.getFormattedMessage()
              << "\n\tfrom:" << t.getOrigin());
            continue;
        }

        osg::ref_ptr<osg::Node> submodel_final = submodel;
        SGPropertyNode *offs = sub_props->getNode("offsets", false);
        if (offs) {
            osg::Matrix res_matrix;
            osg::ref_ptr<osg::MatrixTransform> align = new osg::MatrixTransform;
            align->setDataVariance(osg::Object::STATIC);
            res_matrix.makeIdentity();
            res_matrix.makeRotate(
                offs->getDoubleValue("pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
                osg::Vec3(0, 1, 0),
                offs->getDoubleValue("roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
                osg::Vec3(1, 0, 0),
                offs->getDoubleValue("heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
                osg::Vec3(0, 0, 1));

            osg::Matrix tmat;
            tmat.makeIdentity();
            tmat.makeTranslate(offs->getDoubleValue("x-m", 0),
                               offs->getDoubleValue("y-m", 0),
                               offs->getDoubleValue("z-m", 0));
            align->setMatrix(res_matrix*tmat);
            align->addChild(submodel.get());
            submodel_final = align;
        }
        submodel_final->setName(sub_props->getStringValue("name", ""));

        SGPropertyNode *cond = sub_props->getNode("condition", false);
        if (cond) {
            osg::ref_ptr<osg::Switch> sw = new osg::Switch;
            sw->setUpdateCallback(new SGSwitchUpdateCallback(sgReadCondition(prop_root, cond)));
            group->addChild(sw.get());
            sw->addChild(submodel_final.get());
            sw->setName("submodel condition switch");
        } else {
            group->addChild(submodel_final.get());
        }
    } // end of submodel loading

    osg::Node *(*load_panel)(SGPropertyNode *) = options->getLoadPanel();
    if ( load_panel ) {
        // Load panels
        vector<SGPropertyNode_ptr> panel_nodes = props->getChildren("panel");
        for (unsigned i = 0; i < panel_nodes.size(); i++) {
            SG_LOG(SG_INPUT, SG_DEBUG, "Loading a panel");
            osg::ref_ptr<osg::Node> panel = load_panel(panel_nodes[i]);
            if (panel_nodes[i]->hasValue("name"))
                panel->setName(panel_nodes[i]->getStringValue("name"));
            group->addChild(panel.get());
        }
    }

    std::vector<SGPropertyNode_ptr> particle_nodes;
    particle_nodes = props->getChildren("particlesystem");
    for (unsigned i = 0; i < particle_nodes.size(); ++i) {
        osg::ref_ptr<SGReaderWriterOptions> options2;
        options2 = new SGReaderWriterOptions(*options);
        if (i==0) {
            if (!texturepath.extension().empty())
                texturepath = texturepath.dir();

            options2->setDatabasePath(texturepath.str());
        }
        group->addChild(Particles::appendParticles(particle_nodes[i],
                        prop_root,
                        options2.get()));
    }

    std::vector<SGPropertyNode_ptr> text_nodes;
    text_nodes = props->getChildren("text");
    for (unsigned i = 0; i < text_nodes.size(); ++i) {
        group->addChild(SGText::appendText(text_nodes[i],
                        prop_root,
                        options.get()));
    }
    PropertyList effect_nodes = props->getChildren("effect");
    PropertyList animation_nodes = props->getChildren("animation");
    PropertyList light_nodes = props->getChildren("light");
    // Some material animations (eventually all) are actually effects.
    makeEffectAnimations(animation_nodes, effect_nodes);
    {
        ref_ptr<Node> modelWithEffects
            = instantiateEffects(group.get(), effect_nodes, options.get());
        group = static_cast<Group*>(modelWithEffects.get());
    }
    for (unsigned i = 0; i < animation_nodes.size(); ++i)
        /// OSGFIXME: duh, why not only model?????
        SGAnimation::animate(group.get(), animation_nodes[i], prop_root,
                             options.get(), path.str(), i);

    if (!needTransform && group->getNumChildren() < 2) {
        model = group->getChild(0);
        group->removeChild(model.get());
        if (data.valid())
            data->modelLoaded(modelpath.str(), props, model.get());
        return model.release();
    }
    if (data.valid())
        data->modelLoaded(modelpath.str(), props, group.get());
    if (props->hasChild("debug-outfile")) {
        std::string outputfile = props->getStringValue("debug-outfile",
                                 "debug-model.osg");
        osgDB::writeNodeFile(*group, outputfile);
    }

    return group.release();
}

