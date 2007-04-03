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

#include <simgear/scene/util/SGStateAttributeVisitor.hxx>
#include <simgear/scene/util/SGTextureStateAttributeVisitor.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

#include "animation.hxx"
#include "model.hxx"

SG_USING_STD(vector);
SG_USING_STD(set);

// Little helper class that holds an extra reference to a
// loaded 3d model.
// Since we clone all structural nodes from our 3d models,
// the database pager will only see one single reference to
// top node of the model and expire it relatively fast.
// We attach that extra reference to every model cloned from
// a base model in the pager. When that cloned model is deleted
// this extra reference is deleted too. So if there are no
// cloned models left the model will expire.
class SGDatabaseReference : public osg::Observer {
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

// Visitor for 
class SGTextureUpdateVisitor : public SGTextureStateAttributeVisitor {
public:
  SGTextureUpdateVisitor(const osgDB::FilePathList& pathList) :
    mPathList(pathList)
  { }
  osg::Texture2D* textureReplace(int unit,
                                 osg::StateSet::RefAttributePair& refAttr)
  {
    osg::Texture2D* texture;
    texture = dynamic_cast<osg::Texture2D*>(refAttr.first.get());
    if (!texture)
      return 0;
    
    osg::ref_ptr<osg::Image> image = texture->getImage(0);
    if (!image)
      return 0;

    // The currently loaded file name
    std::string fullFilePath = image->getFileName();
    // The short name
    std::string fileName = osgDB::getSimpleFileName(fullFilePath);
    // The name that should be found with the current database path
    std::string fullLiveryFile = osgDB::findFileInPath(fileName, mPathList);
    // If they are identical then there is nothing to do
    if (fullLiveryFile == fullFilePath)
      return 0;

    image = osgDB::readImageFile(fullLiveryFile);
    if (!image)
      return 0;

    osg::CopyOp copyOp(osg::CopyOp::DEEP_COPY_ALL &
                       ~osg::CopyOp::DEEP_COPY_IMAGES);
    texture = static_cast<osg::Texture2D*>(copyOp(texture));
    if (!texture)
      return 0;
    texture->setImage(image.get());
    return texture;
  }
  virtual void apply(osg::StateSet* stateSet)
  {
    if (!stateSet)
      return;

    // get a copy that we can safely modify the statesets values.
    osg::StateSet::TextureAttributeList attrList;
    attrList = stateSet->getTextureAttributeList();
    for (unsigned unit = 0; unit < attrList.size(); ++unit) {
      osg::StateSet::AttributeList::iterator i;
      i = attrList[unit].begin();
      while (i != attrList[unit].end()) {
        osg::Texture2D* texture = textureReplace(unit, i->second);
        if (texture) {
          stateSet->removeTextureAttribute(unit, i->second.first.get());
          stateSet->setTextureAttribute(unit, texture, i->second.second);
          stateSet->setTextureMode(unit, GL_TEXTURE_2D,
                                   osg::StateAttribute::ON);
        }
        ++i;
      }
    }
  }

private:
  osgDB::FilePathList mPathList;
};

class SGTexCompressionVisitor : public SGTextureStateAttributeVisitor {
public:
  SGTexCompressionVisitor(osg::Texture::InternalFormatMode formatMode) :
    mFormatMode(formatMode)
  { }

  virtual void apply(int, osg::StateSet::RefAttributePair& refAttr)
  {
    osg::Texture2D* texture;
    texture = dynamic_cast<osg::Texture2D*>(refAttr.first.get());
    if (!texture)
      return;
    
    osg::Image* image = texture->getImage(0);
    if (!image)
      return;

    int s = image->s();
    int t = image->t();
    if (s <= t && 32 <= s) {
      texture->setInternalFormatMode(mFormatMode);
    } else if (t < s && 32 <= t) {
      texture->setInternalFormatMode(mFormatMode);
    }
  }

private:
  osg::Texture::InternalFormatMode mFormatMode;
};

class SGAcMaterialCrippleVisitor : public SGStateAttributeVisitor {
public:
  virtual void apply(osg::StateSet::RefAttributePair& refAttr)
  {
    osg::Material* material;
    material = dynamic_cast<osg::Material*>(refAttr.first.get());
    if (!material)
      return;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
  }
};

class SGReadFileCallback :
  public osgDB::Registry::ReadFileCallback {
public:
  virtual osgDB::ReaderWriter::ReadResult
  readImage(const std::string& fileName,
            const osgDB::ReaderWriter::Options* opt)
  {
    std::string absFileName = osgDB::findDataFile(fileName);
    if (!osgDB::fileExists(absFileName)) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot find image file \""
             << fileName << "\"");
      return osgDB::ReaderWriter::ReadResult::FILE_NOT_FOUND;
    }

    osgDB::Registry* registry = osgDB::Registry::instance();
    osgDB::ReaderWriter::ReadResult res;
    res = registry->readImageImplementation(absFileName, opt);
    if (res.loadedFromCache())
      SG_LOG(SG_IO, SG_INFO, "Returning cached image \""
             << res.getImage()->getFileName() << "\"");
    else
      SG_LOG(SG_IO, SG_INFO, "Reading image \""
             << res.getImage()->getFileName() << "\"");

    return res;
  }

  virtual osgDB::ReaderWriter::ReadResult
  readNode(const std::string& fileName,
           const osgDB::ReaderWriter::Options* opt)
  {
    std::string absFileName = osgDB::findDataFile(fileName);
    if (!osgDB::fileExists(absFileName)) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot find model file \""
             << fileName << "\"");
      return osgDB::ReaderWriter::ReadResult::FILE_NOT_FOUND;
    }

    osgDB::Registry* registry = osgDB::Registry::instance();
    osgDB::ReaderWriter::ReadResult res;
    res = registry->readNodeImplementation(absFileName, opt);
    if (!res.validNode())
      return res;

    if (res.loadedFromCache()) {
      SG_LOG(SG_IO, SG_INFO, "Returning cached model \""
             << absFileName << "\"");
    } else {
      SG_LOG(SG_IO, SG_INFO, "Reading model \""
             << absFileName << "\"");

      bool needTristrip = true;
      if (osgDB::getLowerCaseFileExtension(absFileName) == "ac") {
        // we get optimal geometry from the loader.
        needTristrip = false;
        osg::Matrix m(1, 0, 0, 0,
                      0, 0, 1, 0,
                      0, -1, 0, 0,
                      0, 0, 0, 1);
        
        osg::ref_ptr<osg::Group> root = new osg::Group;
        osg::MatrixTransform* transform = new osg::MatrixTransform;
        root->addChild(transform);
        
        transform->setDataVariance(osg::Object::STATIC);
        transform->setMatrix(m);
        transform->addChild(res.getNode());
        
        res = osgDB::ReaderWriter::ReadResult(0);
        
        osgUtil::Optimizer optimizer;
        unsigned opts = osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS;
        optimizer.optimize(root.get(), opts);

        // strip away unneeded groups
        if (root->getNumChildren() == 1 && root->getName().empty()) {
          res = osgDB::ReaderWriter::ReadResult(root->getChild(0));
        } else
          res = osgDB::ReaderWriter::ReadResult(root.get());
        
        // Ok, this step is questionable.
        // It is there to have the same visual appearance of ac objects for the
        // first cut. Osg's ac3d loader will correctly set materials from the
        // ac file. But the old plib loader used GL_AMBIENT_AND_DIFFUSE for the
        // materials that in effect igored the ambient part specified in the
        // file. We emulate that for the first cut here by changing all
        // ac models here. But in the long term we should use the
        // unchanged model and fix the input files instead ...
        SGAcMaterialCrippleVisitor matCriple;
        res.getNode()->accept(matCriple);
      }
      
      osgUtil::Optimizer optimizer;
      unsigned opts = 0;
      // Don't use this one. It will break animation names ...
      // opts |= osgUtil::Optimizer::REMOVE_REDUNDANT_NODES;

      // opts |= osgUtil::Optimizer::REMOVE_LOADED_PROXY_NODES;
      // opts |= osgUtil::Optimizer::COMBINE_ADJACENT_LODS;
      // opts |= osgUtil::Optimizer::SHARE_DUPLICATE_STATE;
      opts |= osgUtil::Optimizer::MERGE_GEOMETRY;
      // opts |= osgUtil::Optimizer::CHECK_GEOMETRY;
      // opts |= osgUtil::Optimizer::SPATIALIZE_GROUPS;
      // opts |= osgUtil::Optimizer::COPY_SHARED_NODES;
      opts |= osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS;
      if (needTristrip)
        opts |= osgUtil::Optimizer::TRISTRIP_GEOMETRY;
      // opts |= osgUtil::Optimizer::TESSELATE_GEOMETRY;
      opts |= osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS;
      optimizer.optimize(res.getNode(), opts);

      // OSGFIXME
      registry->getSharedStateManager()->share(res.getNode());
      
      // OSGFIXME: guard that with a flag
      // OSGFIXME: in the long term it is unclear if we have an OpenGL
      // context here...
      osg::Texture::Extensions* e = osg::Texture::getExtensions(0, true);
      if (e->isTextureCompressionARBSupported()) {
        SGTexCompressionVisitor texComp(osg::Texture::USE_ARB_COMPRESSION);
        res.getNode()->accept(texComp);
      } else if (e->isTextureCompressionS3TCSupported()) {
        SGTexCompressionVisitor texComp(osg::Texture::USE_S3TC_DXT5_COMPRESSION);
        res.getNode()->accept(texComp);
      }
    }

    // Add an extra reference to the model stored in the database.
    // That it to avoid expiring the object from the cache even if it is still
    // in use. Note that the object cache will think that a model is unused
    // if the reference count is 1. If we clone all structural nodes here
    // we need that extra reference to the original object
    SGDatabaseReference* databaseReference;
    databaseReference = new SGDatabaseReference(res.getNode());
    osg::CopyOp::CopyFlags flags = osg::CopyOp::DEEP_COPY_ALL;
    flags &= ~osg::CopyOp::DEEP_COPY_TEXTURES;
    flags &= ~osg::CopyOp::DEEP_COPY_IMAGES;
    flags &= ~osg::CopyOp::DEEP_COPY_ARRAYS;
    flags &= ~osg::CopyOp::DEEP_COPY_PRIMITIVES;
    // This will safe display lists ...
    flags &= ~osg::CopyOp::DEEP_COPY_DRAWABLES;
    flags &= ~osg::CopyOp::DEEP_COPY_SHAPES;
    res = osgDB::ReaderWriter::ReadResult(osg::CopyOp(flags)(res.getNode()));
    res.getNode()->addObserver(databaseReference);

    SGTextureUpdateVisitor liveryUpdate(osgDB::getDataFilePathList());
    res.getNode()->accept(liveryUpdate);

    // OSGFIXME: don't forget that mutex here
    registry->getOrCreateSharedStateManager()->share(res.getNode(), 0);

    return res;
  }
};

class SGReadCallbackInstaller {
public:
  SGReadCallbackInstaller()
  {
    osg::Referenced::setThreadSafeReferenceCounting(true);

    osgDB::Registry* registry = osgDB::Registry::instance();
    osgDB::ReaderWriter::Options* options = new osgDB::ReaderWriter::Options;
    options->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_ALL);
    registry->setOptions(options);
    registry->getOrCreateSharedStateManager()->setShareMode(osgDB::SharedStateManager::SHARE_TEXTURES);
    registry->setReadFileCallback(new SGReadFileCallback);
  }
};

static SGReadCallbackInstaller readCallbackInstaller;

osg::Texture2D*
SGLoadTexture2D(const std::string& path, bool wrapu, bool wrapv, int)
{
  osg::Image* image = osgDB::readImageFile(path);
  osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
  texture->setImage(image);
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

    // OSGFIXME: guard with a flag
    if (osg::Texture::getExtensions(0, true)->isTextureCompressionARBSupported()) {
      if (s <= t && 32 <= s) {
        texture->setInternalFormatMode(osg::Texture::USE_ARB_COMPRESSION);
      } else if (t < s && 32 <= t) {
        texture->setInternalFormatMode(osg::Texture::USE_ARB_COMPRESSION);
      }
    } else if (osg::Texture::getExtensions(0, true)->isTextureCompressionS3TCSupported()) {
      if (s <= t && 32 <= s) {
        texture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT5_COMPRESSION);
      } else if (t < s && 32 <= t) {
        texture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT5_COMPRESSION);
      }
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
    registry->getOrCreateSharedStateManager()->share(tmpNode.get(), 0);

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

  osgDB::FilePathList pathList = osgDB::getDataFilePathList();
  osgDB::Registry::instance()->initFilePathLists();

  // Assume that textures are in
  // the same location as the XML file.
  if (!model) {
    if (texturepath.extension() != "")
          texturepath = texturepath.dir();

    osgDB::Registry::instance()->getDataFilePathList().push_front(texturepath.str());

    model = osgDB::readNodeFile(modelpath.str());
    if (model == 0)
      throw sg_io_exception("Failed to load 3D model", 
                            sg_location(modelpath.str()));
  }

  osgDB::Registry::instance()->getDataFilePathList().push_front(externalTexturePath.str());

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

  if (data) {
    alignmainmodel->setUserData(data);
    data->modelLoaded(path, &props, alignmainmodel.get());
  }

  std::vector<SGPropertyNode_ptr> animation_nodes;
  animation_nodes = props.getChildren("animation");
  for (unsigned i = 0; i < animation_nodes.size(); ++i)
    /// OSGFIXME: duh, why not only model?????
    SGAnimation::animate(alignmainmodel.get(), animation_nodes[i], prop_root);

  // restore old path list
  osgDB::setDataFilePathList(pathList);

  if (props.hasChild("debug-outfile")) {
    std::string outputfile = props.getStringValue("debug-outfile",
                                                  "debug-model.osg");
    osgDB::writeNodeFile(*alignmainmodel, outputfile);
  }

  return alignmainmodel.release();
}

// end of model.cxx
