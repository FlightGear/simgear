// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <string.h>             // for strcmp()

#include <vector>
#include <set>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <plib/ul.h>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include "animation.hxx"
#include "model.hxx"

SG_USING_STD(vector);
SG_USING_STD(set);

bool sgUseDisplayList = true;

////////////////////////////////////////////////////////////////////////
// Global state
////////////////////////////////////////////////////////////////////////
static bool
model_filter = true;


////////////////////////////////////////////////////////////////////////
// Static utility functions.
////////////////////////////////////////////////////////////////////////

static int
model_filter_callback (ssgEntity * entity, int mask)
{
  return model_filter ? 1 : 0;
}

/**
 * Callback to update an animation.
 */
static int
animation_callback (ssgEntity * entity, int mask)
{
    return ((SGAnimation *)entity->getUserData())->update();
}

/**
 * Callback to restore the state after an animation.
 */
static int
restore_callback (ssgEntity * entity, int mask)
{
    ((SGAnimation *)entity->getUserData())->restore();
    return 1;
}


/**
 * Locate a named SSG node in a branch.
 */
static ssgEntity *
find_named_node (ssgEntity * node, const char * name)
{
  char * node_name = node->getName();
  if (node_name != 0 && !strcmp(name, node_name))
    return node;
  else if (node->isAKindOf(ssgTypeBranch())) {
    int nKids = node->getNumKids();
    for (int i = 0; i < nKids; i++) {
      ssgEntity * result =
        find_named_node(((ssgBranch*)node)->getKid(i), name);
      if (result != 0)
        return result;
    }
  } 
  return 0;
}

/**
 * Splice a branch in between all child nodes and their parents.
 */
static void
splice_branch (ssgBranch * branch, ssgEntity * child)
{
  int nParents = child->getNumParents();
  branch->addKid(child);
  for (int i = 0; i < nParents; i++) {
    ssgBranch * parent = child->getParent(i);
    parent->replaceKid(child, branch);
  }
}

/**
 * Make an offset matrix from rotations and position offset.
 */
void
sgMakeOffsetsMatrix( sgMat4 * result, double h_rot, double p_rot, double r_rot,
                     double x_off, double y_off, double z_off )
{
  sgMat4 rot_matrix;
  sgMat4 pos_matrix;
  sgMakeRotMat4(rot_matrix, h_rot, p_rot, r_rot);
  sgMakeTransMat4(pos_matrix, x_off, y_off, z_off);
  sgMultMat4(*result, pos_matrix, rot_matrix);
}


void
sgMakeAnimation( ssgBranch * model,
                 const char * name,
                 vector<SGPropertyNode_ptr> &name_nodes,
                 SGPropertyNode *prop_root,
                 SGPropertyNode_ptr node,
                 double sim_time_sec,
                 SGPath &texture_path,
                 set<ssgBranch *> &ignore_branches )
{
  bool ignore = false;
  SGAnimation * animation = 0;
  const char * type = node->getStringValue("type", "none");
  if (!strcmp("none", type)) {
    animation = new SGNullAnimation(node);
  } else if (!strcmp("range", type)) {
    animation = new SGRangeAnimation(prop_root, node);
  } else if (!strcmp("billboard", type)) {
    animation = new SGBillboardAnimation(node);
  } else if (!strcmp("select", type)) {
    animation = new SGSelectAnimation(prop_root, node);
  } else if (!strcmp("spin", type)) {
    animation = new SGSpinAnimation(prop_root, node, sim_time_sec );
  } else if (!strcmp("timed", type)) {
    animation = new SGTimedAnimation(node);
  } else if (!strcmp("rotate", type)) {
    animation = new SGRotateAnimation(prop_root, node);
  } else if (!strcmp("translate", type)) {
    animation = new SGTranslateAnimation(prop_root, node);
  } else if (!strcmp("scale", type)) {
    animation = new SGScaleAnimation(prop_root, node);
  } else if (!strcmp("texrotate", type)) {
    animation = new SGTexRotateAnimation(prop_root, node);
  } else if (!strcmp("textranslate", type)) {
    animation = new SGTexTranslateAnimation(prop_root, node);
  } else if (!strcmp("texmultiple", type)) {
    animation = new SGTexMultipleAnimation(prop_root, node);
  } else if (!strcmp("blend", type)) {
    animation = new SGBlendAnimation(prop_root, node);
    ignore = true;
  } else if (!strcmp("alpha-test", type)) {
    animation = new SGAlphaTestAnimation(node);
  } else if (!strcmp("material", type)) {
    animation = new SGMaterialAnimation(prop_root, node, texture_path);
  } else if (!strcmp("flash", type)) {
    animation = new SGFlashAnimation(node);
  } else if (!strcmp("dist-scale", type)) {
    animation = new SGDistScaleAnimation(node);
  } else if (!strcmp("noshadow", type)) {
    animation = new SGShadowAnimation(prop_root, node);
  } else if (!strcmp("shader", type)) {
    animation = new SGShaderAnimation(prop_root, node);
  } else {
    animation = new SGNullAnimation(node);
    SG_LOG(SG_INPUT, SG_WARN, "Unknown animation type " << type);
  }

  if (name != 0)
      animation->setName((char *)name);

  ssgEntity * object;
  if (name_nodes.size() > 0) {
    object = find_named_node(model, name_nodes[0]->getStringValue());
    if (object == 0) {
      SG_LOG(SG_INPUT, SG_ALERT, "Object " << name_nodes[0]->getStringValue()
             << " not found");
      delete animation;
      animation = 0;
    }
  } else {
    object = model;
  }

  if ( animation == 0 )
     return;
  
  ssgBranch * branch = animation->getBranch();
  splice_branch(branch, object);

  for (unsigned int i = 1; i < name_nodes.size(); i++) {
      const char * name = name_nodes[i]->getStringValue();
      object = find_named_node(model, name);
      if (object == 0) {
          SG_LOG(SG_INPUT, SG_ALERT, "Object " << name << " not found");
          delete animation;
          animation = 0;
      } else {
          ssgBranch * oldParent = object->getParent(0);
          branch->addKid(object);
          oldParent->removeKid(object);
      }
  }

  if ( animation != 0 ) {
    animation->init();
    branch->setUserData(animation);
    branch->setTravCallback(SSG_CALLBACK_PRETRAV, animation_callback);
    branch->setTravCallback(SSG_CALLBACK_POSTTRAV, restore_callback);
    if ( ignore ) {
      ignore_branches.insert( branch );
    }
  }
}


static void makeDList( ssgBranch *b, const set<ssgBranch *> &ignore )
{
  int nb = b->getNumKids();
  for (int i = 0; i<nb; i++) {
    ssgEntity *e = b->getKid(i);
    if (e->isAKindOf(ssgTypeLeaf())) {
     if( ((ssgLeaf*)e)->getNumVertices() > 0)
      ((ssgLeaf*)e)->makeDList();
    } else if (e->isAKindOf(ssgTypeBranch()) && ignore.find((ssgBranch *)e) == ignore.end()) {
      makeDList( (ssgBranch*)e, ignore );
    }
  }
}



////////////////////////////////////////////////////////////////////////
// Global functions.
////////////////////////////////////////////////////////////////////////

ssgBranch *
sgLoad3DModel( const string &fg_root, const string &path,
               SGPropertyNode *prop_root,
               double sim_time_sec, ssgEntity *(*load_panel)(SGPropertyNode *),
               SGModelData *data )
{
  ssgBranch * model = 0;
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
      if (model == 0)
        model = new ssgBranch;
    }
  }

                                // Assume that textures are in
                                // the same location as the XML file.
  if (model == 0) {
    if (texturepath.extension() != "")
          texturepath = texturepath.dir();

    ssgTexturePath((char *)texturepath.c_str());
    model = (ssgBranch *)ssgLoad((char *)modelpath.c_str());
    if (model == 0)
      throw sg_io_exception("Failed to load 3D model", 
			  sg_location(modelpath.str()));
  }
                                // Set up the alignment node
  ssgTransform * alignmainmodel = new ssgTransform;
  if ( load_panel == 0 )
    alignmainmodel->setTravCallback( SSG_CALLBACK_PRETRAV, model_filter_callback );
  alignmainmodel->addKid(model);
  sgMat4 res_matrix;
  sgMakeOffsetsMatrix(&res_matrix,
                      props.getFloatValue("/offsets/heading-deg", 0.0),
                      props.getFloatValue("/offsets/roll-deg", 0.0),
                      props.getFloatValue("/offsets/pitch-deg", 0.0),
                      props.getFloatValue("/offsets/x-m", 0.0),
                      props.getFloatValue("/offsets/y-m", 0.0),
                      props.getFloatValue("/offsets/z-m", 0.0));
  alignmainmodel->setTransform(res_matrix);

  unsigned int i;

                                // Load sub-models
  vector<SGPropertyNode_ptr> model_nodes = props.getChildren("model");
  for (i = 0; i < model_nodes.size(); i++) {
    SGPropertyNode_ptr node = model_nodes[i];
    ssgTransform * align = new ssgTransform;
    sgMat4 res_matrix;
    sgMakeOffsetsMatrix(&res_matrix,
                        node->getFloatValue("offsets/heading-deg", 0.0),
                        node->getFloatValue("offsets/roll-deg", 0.0),
                        node->getFloatValue("offsets/pitch-deg", 0.0),
                        node->getFloatValue("offsets/x-m", 0.0),
                        node->getFloatValue("offsets/y-m", 0.0),
                        node->getFloatValue("offsets/z-m", 0.0));
    align->setTransform(res_matrix);

    ssgBranch * kid = sgLoad3DModel( fg_root, node->getStringValue("path"),
                                     prop_root, sim_time_sec, load_panel );
    align->addKid(kid);
    align->setName(node->getStringValue("name", ""));
    model->addKid(align);
  }

  if ( load_panel ) {
                                // Load panels
    vector<SGPropertyNode_ptr> panel_nodes = props.getChildren("panel");
    for (i = 0; i < panel_nodes.size(); i++) {
        SG_LOG(SG_INPUT, SG_DEBUG, "Loading a panel");
        ssgEntity * panel = load_panel(panel_nodes[i]);
        if (panel_nodes[i]->hasValue("name"))
            panel->setName((char *)panel_nodes[i]->getStringValue("name"));
        model->addKid(panel);
    }
  }

  if (data) {
    data->modelLoaded(path, &props, model);
    model->setUserData(data);
  }
                                // Load animations
  set<ssgBranch *> ignore_branches;
  vector<SGPropertyNode_ptr> animation_nodes = props.getChildren("animation");
  for (i = 0; i < animation_nodes.size(); i++) {
    const char * name = animation_nodes[i]->getStringValue("name", 0);
    vector<SGPropertyNode_ptr> name_nodes =
      animation_nodes[i]->getChildren("object-name");
    sgMakeAnimation( model, name, name_nodes, prop_root, animation_nodes[i],
                     sim_time_sec, texturepath, ignore_branches);
  }

#if PLIB_VERSION > 183
  if ( model != 0 && sgUseDisplayList ) {
     makeDList( model, ignore_branches );
  }
#endif

  int m = props.getIntValue("dump", 0);
  if (m > 0)
    model->print(stderr, "", m - 1);

  return alignmainmodel;
}

bool
sgSetModelFilter( bool filter )
{
  bool old = model_filter;
  model_filter = filter;
  return old;
}

bool 
sgCheckAnimationBranch (ssgEntity * entity)
{
    return entity->getTravCallback(SSG_CALLBACK_PRETRAV) == animation_callback;
}

// end of model.cxx
