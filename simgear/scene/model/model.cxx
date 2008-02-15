// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <osg/observer_ptr>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/NodeCallback>
#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osgDB/Archive>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osgDB/SharedStateManager>
#include <osgUtil/Optimizer>

#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGStateAttributeVisitor.hxx>
#include <simgear/scene/util/SGTextureStateAttributeVisitor.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

#include "animation.hxx"
#include "model.hxx"
#include "particles.hxx"

SG_USING_STD(vector);

using namespace simgear;

osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const std::string& path,
                const osgDB::ReaderWriter::Options* options,
                bool wrapu, bool wrapv, int)
{
  osg::Image* image;
  if (options)
    image = osgDB::readImageFile(path, options);
  else
    image = osgDB::readImageFile(path);
  osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
  texture->setImage(image);
  if (staticTexture)
    texture->setDataVariance(osg::Object::STATIC);
  if (wrapu)
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
  else
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
  if (wrapv)
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
  else
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);

  if (image) {
    int s = image->s();
    int t = image->t();

    if (s <= t && 32 <= s) {
      SGSceneFeatures::instance()->setTextureCompression(texture.get());
    } else if (t < s && 32 <= t) {
      SGSceneFeatures::instance()->setTextureCompression(texture.get());
    }
  }

  // Make sure the texture is shared if we already have the same texture
  // somewhere ...
  {
    osg::ref_ptr<osg::Node> tmpNode = new osg::Node;
    osg::StateSet* stateSet = tmpNode->getOrCreateStateSet();
    stateSet->setTextureAttribute(0, texture.get());

    // OSGFIXME: don't forget that mutex here
    osgDB::Registry* registry = osgDB::Registry::instance();
    registry->getSharedStateManager()->share(tmpNode.get(), 0);

    // should be the same, but be paranoid ...
    stateSet = tmpNode->getStateSet();
    osg::StateAttribute* stateAttr;
    stateAttr = stateSet->getTextureAttribute(0, osg::StateAttribute::TEXTURE);
    osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(stateAttr);
    if (texture2D)
      texture = texture2D;
  }

  return texture.release();
}

class SGSwitchUpdateCallback : public osg::NodeCallback {
public:
  SGSwitchUpdateCallback(SGCondition* condition) :
    mCondition(condition) {}
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  { 
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


////////////////////////////////////////////////////////////////////////
// Global functions.
////////////////////////////////////////////////////////////////////////

osg::Node *
sgLoad3DModel( const string &fg_root, const string &path,
               SGPropertyNode *prop_root,
               double sim_time_sec, osg::Node *(*load_panel)(SGPropertyNode *),
               SGModelData *data,
               const SGPath& externalTexturePath )
{
  osg::ref_ptr<osg::Node> model;
  SGPropertyNode props;

  // Load the 3D aircraft object itself
  SGPath modelpath = path, texturepath = path;
  if ( !ulIsAbsolutePathName( path.c_str() ) ) {
    SGPath tmp = fg_root;
    tmp.append(modelpath.str());
    modelpath = texturepath = tmp;
  }

  // Check for an XML wrapper
  if (modelpath.str().substr(modelpath.str().size() - 4, 4) == ".xml") {
    readProperties(modelpath.str(), &props);
    if (props.hasValue("/path")) {
      modelpath = modelpath.dir();
      modelpath.append(props.getStringValue("/path"));
      if (props.hasValue("/texture-path")) {
        texturepath = texturepath.dir();
        texturepath.append(props.getStringValue("/texture-path"));
      }
    } else {
      if (!model)
        model = new osg::Switch;
    }
  }

  osg::ref_ptr<osgDB::ReaderWriter::Options> options
      = new osgDB::ReaderWriter::Options(*osgDB::Registry::instance()
                                         ->getOptions());

  // Assume that textures are in
  // the same location as the XML file.
  if (!model) {
      if (texturepath.extension() != "")
          texturepath = texturepath.dir();

      options->setDatabasePath(texturepath.str());
      if (!externalTexturePath.str().empty())
          options->getDatabasePathList().push_back(externalTexturePath.str());

      model = osgDB::readNodeFile(modelpath.str(), options.get());
      if (model == 0)
          throw sg_io_exception("Failed to load 3D model", 
                                sg_location(modelpath.str()));
  }

  // Set up the alignment node
  osg::ref_ptr<osg::MatrixTransform> alignmainmodel = new osg::MatrixTransform;
  alignmainmodel->addChild(model.get());
  osg::Matrix res_matrix;
  res_matrix.makeRotate(
    props.getFloatValue("/offsets/pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
    osg::Vec3(0, 1, 0),
    props.getFloatValue("/offsets/roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
    osg::Vec3(1, 0, 0),
    props.getFloatValue("/offsets/heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
    osg::Vec3(0, 0, 1));

  osg::Matrix tmat;
  tmat.makeTranslate(props.getFloatValue("/offsets/x-m", 0.0),
                     props.getFloatValue("/offsets/y-m", 0.0),
                     props.getFloatValue("/offsets/z-m", 0.0));
  alignmainmodel->setMatrix(res_matrix*tmat);

  // Load sub-models
  vector<SGPropertyNode_ptr> model_nodes = props.getChildren("model");
  for (unsigned i = 0; i < model_nodes.size(); i++) {
    SGPropertyNode_ptr node = model_nodes[i];
    osg::ref_ptr<osg::MatrixTransform> align = new osg::MatrixTransform;
    res_matrix.makeIdentity();
    res_matrix.makeRotate(
      node->getDoubleValue("offsets/pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
      osg::Vec3(0, 1, 0),
      node->getDoubleValue("offsets/roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
      osg::Vec3(1, 0, 0),
      node->getDoubleValue("offsets/heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
      osg::Vec3(0, 0, 1));
    
    tmat.makeIdentity();
    tmat.makeTranslate(node->getDoubleValue("offsets/x-m", 0),
                       node->getDoubleValue("offsets/y-m", 0),
                       node->getDoubleValue("offsets/z-m", 0));
    align->setMatrix(res_matrix*tmat);

    osg::ref_ptr<osg::Node> kid;
    const char* submodel = node->getStringValue("path");
    try {
      kid = sgLoad3DModel( fg_root, submodel, prop_root, sim_time_sec, load_panel );

    } catch (const sg_throwable &t) {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to load submodel: " << t.getFormattedMessage());
      throw;
    }
    align->addChild(kid.get());

    align->setName(node->getStringValue("name", ""));

    SGPropertyNode *cond = node->getNode("condition", false);
    if (cond) {
      osg::ref_ptr<osg::Switch> sw = new osg::Switch;
      sw->setUpdateCallback(new SGSwitchUpdateCallback(sgReadCondition(prop_root, cond)));
      alignmainmodel->addChild(sw.get());
      sw->addChild(align.get());
      sw->setName("submodel condition switch");
    } else {
      alignmainmodel->addChild(align.get());
    }
  }

  if ( load_panel ) {
    // Load panels
    vector<SGPropertyNode_ptr> panel_nodes = props.getChildren("panel");
    for (unsigned i = 0; i < panel_nodes.size(); i++) {
        SG_LOG(SG_INPUT, SG_DEBUG, "Loading a panel");
        osg::ref_ptr<osg::Node> panel = load_panel(panel_nodes[i]);
        if (panel_nodes[i]->hasValue("name"))
            panel->setName((char *)panel_nodes[i]->getStringValue("name"));
        alignmainmodel->addChild(panel.get());
    }
  }

  std::vector<SGPropertyNode_ptr> particle_nodes;
  particle_nodes = props.getChildren("particlesystem");
  for (unsigned i = 0; i < particle_nodes.size(); ++i)
  {
    if(i==0)
    {
      if (texturepath.extension() != "")
          texturepath = texturepath.dir();

      options->setDatabasePath(texturepath.str());
      if (!externalTexturePath.str().empty())
          options->getDatabasePathList().push_back(externalTexturePath.str());
    }
    alignmainmodel.get()->addChild(Particles::appendParticles(particle_nodes[i],
                                                              prop_root,
                                                              options.get()));
  }

  if (data) {
    alignmainmodel->setUserData(data);
    data->modelLoaded(path, &props, alignmainmodel.get());
  }

  std::vector<SGPropertyNode_ptr> animation_nodes;
  animation_nodes = props.getChildren("animation");
  for (unsigned i = 0; i < animation_nodes.size(); ++i)
    /// OSGFIXME: duh, why not only model?????
    SGAnimation::animate(alignmainmodel.get(), animation_nodes[i], prop_root,
                         options.get());

  if (props.hasChild("debug-outfile")) {
    std::string outputfile = props.getStringValue("debug-outfile",
                                                  "debug-model.osg");
    osgDB::writeNodeFile(*alignmainmodel, outputfile);
  }

  return alignmainmodel.release();
}

// end of model.cxx
