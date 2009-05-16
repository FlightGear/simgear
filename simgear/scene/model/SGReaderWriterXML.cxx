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

#include <osg/MatrixTransform>
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

#include "modellib.hxx"
#include "SGPagedLOD.hxx"
#include "SGReaderWriterXML.hxx"
#include "SGReaderWriterXMLOptions.hxx"

#include "animation.hxx"
#include "particles.hxx"
#include "model.hxx"

#include "SGReaderWriterXMLOptions.hxx"
#include "SGReaderWriterXML.hxx"

using namespace simgear;

static osg::Node *
sgLoad3DModel_internal(const string &path,
                       SGPropertyNode *prop_root,
                       SGModelData *data = 0,
                       osg::Node *(*load_panel)(SGPropertyNode *) = 0,
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
                            const osgDB::ReaderWriter::Options* options) const
{
    // SG_LOG(SG_GENERAL, SG_ALERT, "SGReaderWriterXML::readNode(" << fileName << ")");

    const SGReaderWriterXMLOptions* xmlOptions
    = dynamic_cast<const SGReaderWriterXMLOptions*>(options);

    string fg_root;
    SGPropertyNode *prop_root=0;
    osg::Node *(*load_panel)(SGPropertyNode *)=0;
    SGModelData *model_data=0;
    SGPath externalTexturePath;

    if (xmlOptions) {
        prop_root = xmlOptions->getPropRoot();
        load_panel = xmlOptions->getLoadPanel();
        model_data = xmlOptions->getModelData();
    }

    fg_root=osgDB::Registry::instance()->getDataFilePathList().front();

    osg::Node *result=0;

    try {
        result=sgLoad3DModel_internal(fileName, prop_root, model_data, load_panel);
    } catch (const sg_throwable &t) {
        SG_LOG(SG_INPUT, SG_ALERT, "Failed to load model: " << t.getFormattedMessage());
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

static osg::Node *
sgLoad3DModel_internal(const string &path,
                       SGPropertyNode *prop_root,
                       SGModelData *data,
                       osg::Node *(*load_panel)(SGPropertyNode *),
                       SGPropertyNode *overlay)
{
    if ( !prop_root ) {
        SG_LOG(SG_GENERAL, SG_ALERT, "prop_root NULL: " << path);
    }

    string fg_root=osgDB::Registry::instance()->getDataFilePathList().front();
    osg::ref_ptr<osg::Node> model;
    osg::ref_ptr<osg::Group> group;
    SGPropertyNode_ptr props = new SGPropertyNode;

    // Load the 3D object itself
    SGPath modelpath = path, texturepath = path;
    if ( !ulIsAbsolutePathName( path.c_str() ) ) {
        SGPath tmp = fg_root;
        tmp.append(modelpath.str());
        modelpath = texturepath = tmp;
    }

    // Check for an XML wrapper
    if (modelpath.str().substr(modelpath.str().size() - 4, 4) == ".xml") {
        try {
            readProperties(modelpath.str(), props);
        } catch (const sg_throwable &t) {
            SG_LOG(SG_INPUT, SG_ALERT, "Failed to load xml: " << t.getFormattedMessage());
            throw;
        }
        if (overlay)
            copyProperties(overlay, props);

        if (props->hasValue("/path")) {
            modelpath = modelpath.dir();
            modelpath.append(props->getStringValue("/path"));
            if (props->hasValue("/texture-path")) {
                texturepath = texturepath.dir();
                texturepath.append(props->getStringValue("/texture-path"));
            }
        } else {
            model = new osg::Node;
        }

        SGPropertyNode *mp = props->getNode("multiplay");
        if (mp && prop_root && prop_root->getParent())
            copyProperties(mp, prop_root);
    }

    osg::ref_ptr<SGReaderWriterXMLOptions> options
    = new SGReaderWriterXMLOptions(*osgDB::Registry::instance()->getOptions());

    // Assume that textures are in
    // the same location as the XML file.
    if (!model) {
        if (texturepath.extension() != "")
            texturepath = texturepath.dir();

        options->setDatabasePath(texturepath.str());
        model = osgDB::readNodeFile(modelpath.str(), options.get());
        if (model == 0)
            throw sg_io_exception("Failed to load 3D model",
                                  sg_location(modelpath.str()));
    }
    model->setName(modelpath.str());

    bool needTransform=false;
    // Set up the alignment node if needed
    SGPropertyNode *offsets = props->getNode("offsets", false);
    if (offsets) {
        needTransform=true;
        osg::MatrixTransform *alignmainmodel = new osg::MatrixTransform;
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
        string submodelFileName = sub_props->getStringValue("path");
        if ( submodelFileName.size() > 2 && submodelFileName.substr( 0, 2 ) == "./" ) {
            submodelpath = modelpath.dir();
            submodelpath.append( submodelFileName.substr( 2 ) );
        } else {
            submodelpath = submodelFileName;
        }
        try {
            submodel = sgLoad3DModel_internal(submodelpath.str(), prop_root, 0, load_panel,
                                              sub_props->getNode("overlay"));
        } catch (const sg_throwable &t) {
            SG_LOG(SG_INPUT, SG_ALERT, "Failed to load submodel: " << t.getFormattedMessage());
            throw;
        }

        osg::ref_ptr<osg::Node> submodel_final=submodel.get();
        SGPropertyNode *offs = sub_props->getNode("offsets", false);
        if (offs) {
            osg::Matrix res_matrix;
            osg::ref_ptr<osg::MatrixTransform> align = new osg::MatrixTransform;
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
            submodel_final=align.get();
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

    if ( load_panel ) {
        // Load panels
        vector<SGPropertyNode_ptr> panel_nodes = props->getChildren("panel");
        for (unsigned i = 0; i < panel_nodes.size(); i++) {
            SG_LOG(SG_INPUT, SG_DEBUG, "Loading a panel");
            osg::ref_ptr<osg::Node> panel = load_panel(panel_nodes[i]);
            if (panel_nodes[i]->hasValue("name"))
                panel->setName((char *)panel_nodes[i]->getStringValue("name"));
            group->addChild(panel.get());
        }
    }

    std::vector<SGPropertyNode_ptr> particle_nodes;
    particle_nodes = props->getChildren("particlesystem");
    for (unsigned i = 0; i < particle_nodes.size(); ++i) {
        if (i==0) {
            if (texturepath.extension() != "")
                texturepath = texturepath.dir();

            options->setDatabasePath(texturepath.str());
        }
        group->addChild(Particles::appendParticles(particle_nodes[i],
                        prop_root,
                        options.get()));
    }

    if (data) {
        data->setProps(props);
        options->setModelData(data);
    }

    std::vector<SGPropertyNode_ptr> animation_nodes;
    animation_nodes = props->getChildren("animation");
    for (unsigned i = 0; i < animation_nodes.size(); ++i)
        /// OSGFIXME: duh, why not only model?????
        SGAnimation::animate(group.get(), animation_nodes[i], prop_root,
                             options.get());

    if (props->hasChild("debug-outfile")) {
        std::string outputfile = props->getStringValue("debug-outfile",
                                 "debug-model.osg");
        osgDB::writeNodeFile(*group, outputfile);
    }
    if (!needTransform && group->getNumChildren() < 2) {
        model = group->getChild(0);
        group->removeChild(model.get());
        model->setUserData(group->getUserData());
        return model.release();
    }

    return group.release();
}

