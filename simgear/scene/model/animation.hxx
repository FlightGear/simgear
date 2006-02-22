
// animation.hxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef _SG_ANIMATION_HXX
#define _SG_ANIMATION_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>
#include <map>

SG_USING_STD(vector);
SG_USING_STD(map);

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>


// Don't pull in the headers, since we don't need them here.
class SGInterpTable;
class SGCondition;
class SGPersonalityBranch;


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif



//////////////////////////////////////////////////////////////////////
// Animation classes
//////////////////////////////////////////////////////////////////////

/**
 * Abstract base class for all animations.
 */
class SGAnimation :  public ssgBase
{
public:
  enum PersonalityVar { INIT_SPIN, LAST_TIME_SEC_SPIN, FACTOR_SPIN,
                        POSITION_DEG_SPIN, INIT_TIMED, LAST_TIME_SEC_TIMED,
                        TOTAL_DURATION_SEC_TIMED, BRANCH_DURATION_SEC_TIMED,
                        STEP_TIMED };

  SGAnimation (SGPropertyNode_ptr props, ssgBranch * branch);

  virtual ~SGAnimation ();

  /**
   * Get the SSG branch holding the animation.
   */
  virtual ssgBranch * getBranch () { return _branch; }

  /**
   * Initialize the animation, after children have been added.
   */
  virtual void init ();

  /**
   * Update the animation.
   */
  virtual int update();

  /**
   * Restore the state after the animation.
   */
  virtual void restore();

  /**
   * Set the value of sim_time_sec.  This needs to be called every
   * frame in order for the time based animations to work correctly.
   */
  static void set_sim_time_sec( double val ) { sim_time_sec = val; }

  /**
   * Current personality branch : enable animation to behave differently
   * for similar objects
   */
  static SGPersonalityBranch *current_object;

  int get_animation_type(void) { return animation_type; }

protected:

  static double sim_time_sec;

  ssgBranch * _branch;

  int animation_type;
};


/**
 * A no-op animation.
 */
class SGNullAnimation : public SGAnimation
{
public:
  SGNullAnimation (SGPropertyNode_ptr props);
  virtual ~SGNullAnimation ();
};


/**
 * A range, or level-of-detail (LOD) animation.
 */
class SGRangeAnimation : public SGAnimation
{
public:
  SGRangeAnimation (SGPropertyNode *prop_root,
                    SGPropertyNode_ptr props);
  virtual ~SGRangeAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _min_prop;
  SGPropertyNode_ptr _max_prop;
  float _min;
  float _max;
  float _min_factor;
  float _max_factor;
  SGCondition * _condition;
};


/**
 * Animation to turn and face the screen.
 */
class SGBillboardAnimation : public SGAnimation
{
public:
  SGBillboardAnimation (SGPropertyNode_ptr props);
  virtual ~SGBillboardAnimation ();
};


/**
 * Animation to select alternative versions of the same object.
 */
class SGSelectAnimation : public SGAnimation
{
public:
  SGSelectAnimation( SGPropertyNode *prop_root,
                   SGPropertyNode_ptr props );
  virtual ~SGSelectAnimation ();
  virtual int update();
private:
  SGCondition * _condition;
};


/**
 * Animation to spin an object around a center point.
 *
 * This animation rotates at a specific velocity.
 */
class SGSpinAnimation : public SGAnimation
{
public:
  SGSpinAnimation( SGPropertyNode *prop_root,
                 SGPropertyNode_ptr props,
                 double sim_time_sec );
  virtual ~SGSpinAnimation ();
  virtual int update();
private:
  bool _use_personality;
  SGPropertyNode_ptr _prop;
  double _factor;
  double _factor_min;
  double _factor_max;
  double _position_deg;
  double _position_deg_min;
  double _position_deg_max;
  double _last_time_sec;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
  SGCondition * _condition;
};


/**
 * Animation to draw objects for a specific amount of time each.
 */
class SGTimedAnimation : public SGAnimation
{
public:
    SGTimedAnimation (SGPropertyNode_ptr props);
    virtual ~SGTimedAnimation ();
    virtual void init();
    virtual int update();
private:
    bool _use_personality;
    double _duration_sec;
    double _last_time_sec;
    double _total_duration_sec;
    int _step;
    struct DurationSpec {
        DurationSpec( double m = 0.0 ) : _min(m), _max(m) {}
        DurationSpec( double m1, double m2 ) : _min(m1), _max(m2) {}
        double _min, _max;
    };
    vector<DurationSpec> _branch_duration_specs;
    vector<double> _branch_duration_sec;
};


/**
 * Animation to rotate an object around a center point.
 *
 * This animation rotates to a specific position.
 */
class SGRotateAnimation : public SGAnimation
{
public:
  SGRotateAnimation( SGPropertyNode *prop_root, SGPropertyNode_ptr props );
  virtual ~SGRotateAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _prop;
  double _offset_deg;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_deg;
  bool _has_max;
  double _max_deg;
  double _position_deg;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
  SGCondition * _condition;
};


/**
 * Animation to slide along an axis.
 */
class SGTranslateAnimation : public SGAnimation
{
public:
  SGTranslateAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGTranslateAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _prop;
  double _offset_m;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_m;
  bool _has_max;
  double _max_m;
  double _position_m;
  sgMat4 _matrix;
  sgVec3 _axis;
  SGCondition * _condition;
};

/**
 * Animation to blend an object.
 */
class SGBlendAnimation : public SGAnimation
{
public:
  SGBlendAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGBlendAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _prop;
  SGInterpTable * _table;
  double _prev_value;
  double _offset;
  double _factor;
  bool _has_min;
  double _min;
  bool _has_max;
  double _max;
};

/**
 * Animation to scale an object.
 */
class SGScaleAnimation : public SGAnimation
{
public:
  SGScaleAnimation( SGPropertyNode *prop_root,
                        SGPropertyNode_ptr props );
  virtual ~SGScaleAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _prop;
  double _x_factor;
  double _y_factor;
  double _z_factor;
  double _x_offset;
  double _y_offset;
  double _z_offset;
  SGInterpTable * _table;
  bool _has_min_x;
  bool _has_min_y;
  bool _has_min_z;
  double _min_x;
  double _min_y;
  double _min_z;
  bool _has_max_x;
  bool _has_max_y;
  bool _has_max_z;
  double _max_x;
  double _max_y;
  double _max_z;
  double _x_scale;
  double _y_scale;
  double _z_scale;
  sgMat4 _matrix;
};

/**
 * Animation to rotate texture mappings around a center point.
 *
 * This animation rotates to a specific position.
 */
class SGTexRotateAnimation : public SGAnimation
{
public:
  SGTexRotateAnimation( SGPropertyNode *prop_root, SGPropertyNode_ptr props );
  virtual ~SGTexRotateAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _prop;
  double _offset_deg;
  double _factor;
  SGInterpTable * _table;
  bool _has_min;
  double _min_deg;
  bool _has_max;
  double _max_deg;
  double _position_deg;
  sgMat4 _matrix;
  sgVec3 _center;
  sgVec3 _axis;
};


/**
 * Animation to slide texture mappings along an axis.
 */
class SGTexTranslateAnimation : public SGAnimation
{
public:
  SGTexTranslateAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGTexTranslateAnimation ();
  virtual int update();
private:
  SGPropertyNode_ptr _prop;
  double _offset;
  double _factor;
  double _step;
  double _scroll;
  SGInterpTable * _table;
  bool _has_min;
  double _min;
  bool _has_max;
  double _max;
  double _position;
  sgMat4 _matrix;
  sgVec3 _axis;
};



/**
 * Classes for handling multiple types of Texture translations on one object
 */

class SGTexMultipleAnimation : public SGAnimation
{
public:
  SGTexMultipleAnimation( SGPropertyNode *prop_root,
                      SGPropertyNode_ptr props );
  virtual ~SGTexMultipleAnimation ();
  virtual int update();
private:
  class TexTransform
    {
    public:
    SGPropertyNode_ptr prop;
    int subtype; //  0=translation, 1=rotation
    double offset;
    double factor;
    double step;
    double scroll;
    SGInterpTable * table;
    bool has_min;
    double min;
    bool has_max;
    double max;
    double position;
    sgMat4 matrix;
    sgVec3 center;
    sgVec3 axis;
  };
  SGPropertyNode_ptr _prop;
  TexTransform* _transform;
  int _num_transforms;
};


/**
 * An "animation" to enable the alpha test 
 */
class SGAlphaTestAnimation : public SGAnimation
{
public:
  SGAlphaTestAnimation(SGPropertyNode_ptr props);
  virtual ~SGAlphaTestAnimation ();
  virtual void init();
private:
  void setAlphaClampToBranch(ssgBranch *b, float clamp);
  float _alpha_clamp;
};


/**
 * An "animation" to modify material properties
 */
class SGMaterialAnimation : public SGAnimation
{
public:
    SGMaterialAnimation(SGPropertyNode *prop_root, SGPropertyNode_ptr props,
            const SGPath &texpath);
    virtual ~SGMaterialAnimation() {}
    virtual void init();
    virtual int update();
private:
    enum {
        DIFFUSE = 1,
        AMBIENT = 2,
        SPECULAR = 4,
        EMISSION = 8,
        SHININESS = 16,
        TRANSPARENCY = 32,
        THRESHOLD = 64,
        TEXTURE = 128,
    };
    struct ColorSpec {
        float red, green, blue;
        float factor;
        float offset;
        SGPropertyNode_ptr red_prop;
        SGPropertyNode_ptr green_prop;
        SGPropertyNode_ptr blue_prop;
        SGPropertyNode_ptr factor_prop;
        SGPropertyNode_ptr offset_prop;
        sgVec4 v;
        inline bool dirty() {
            return red >= 0.0 || green >= 0.0 || blue >= 0.0;
        }
        inline bool live() {
            return red_prop || green_prop || blue_prop
                    || factor_prop || offset_prop;
        }
        inline bool operator!=(ColorSpec& a) {
            return red != a.red || green != a.green || blue != a.blue
                    || factor != a.factor || offset != a.offset;
        }
        sgVec4 &rgba() {
            v[0] = clamp(red * factor + offset);
            v[1] = clamp(green * factor + offset);
            v[2] = clamp(blue * factor + offset);
            v[3] = 1.0;
            return v;
        }
        inline float clamp(float val) {
            return val < 0.0 ? 0.0 : val > 1.0 ? 1.0 : val;
        }
    };
    struct PropSpec {
        float value;
        float factor;
        float offset;
        float min;
        float max;
        SGPropertyNode_ptr value_prop;
        SGPropertyNode_ptr factor_prop;
        SGPropertyNode_ptr offset_prop;
        inline bool dirty() { return value >= 0.0; }
        inline bool live() { return value_prop || factor_prop || offset_prop; }
        inline bool operator!=(PropSpec& a) {
            return value != a.value || factor != a.factor || offset != a.offset;
        }
    };
    SGCondition *_condition;
    bool _last_condition;
    SGPropertyNode *_prop_root;
    string _prop_base;
    SGPath _texture_base;
    SGPath _texture;
    string _texture_str;
    ssgSimpleState* _cached_material;
    ssgSimpleState* _cloned_material;
    unsigned _read;
    unsigned _update;
    unsigned _static_update;
    bool _global;
    ColorSpec _diff;
    ColorSpec _amb;
    ColorSpec _emis;
    ColorSpec _spec;
    float _shi;
    PropSpec _trans;
    float _thresh;	// alpha_clamp (see man glAlphaFunc)
    string _tex;
    string _tmpstr;
    SGPropertyNode_ptr _shi_prop;
    SGPropertyNode_ptr _thresh_prop;
    SGPropertyNode_ptr _tex_prop;

    void cloneMaterials(ssgBranch *b);
    void setMaterialBranch(ssgBranch *b);
    void initColorGroup(SGPropertyNode_ptr, ColorSpec *, int flag);
    void updateColorGroup(ColorSpec *, int flag);
    inline float clamp(float val, float min = 0.0, float max = 1.0) {
        return val < min ? min : val > max ? max : val;
    }
    const char *path(const char *rel) {
        return (_tmpstr = _prop_base + rel).c_str();
    }
};


/**
 * An "animation" that compute a scale according to 
 * the angle between an axis and the view direction
 */
class SGFlashAnimation : public SGAnimation
{
public:
  SGFlashAnimation(SGPropertyNode_ptr props);
  virtual ~SGFlashAnimation ();

  static void flashCallback( sgMat4 r, sgFrustum *f, sgMat4 m, void *d );
  void flashCallback( sgMat4 r, sgFrustum *f, sgMat4 m );

private:
  sgVec3 _axis, _center;
  float _power, _factor, _offset, _min_v, _max_v;
  bool _two_sides;
};


/**
 * An animation that compute a scale according to 
 * the distance from a point and the viewer
 */
class SGDistScaleAnimation : public SGAnimation
{
public:
  SGDistScaleAnimation(SGPropertyNode_ptr props);
  virtual ~SGDistScaleAnimation ();

  static void distScaleCallback( sgMat4 r, sgFrustum *f, sgMat4 m, void *d );
  void distScaleCallback( sgMat4 r, sgFrustum *f, sgMat4 m );

private:
  sgVec3 _center;
  float _factor, _offset, _min_v, _max_v;
  bool _has_min, _has_max;
  SGInterpTable * _table;
};

/**
 * An animation to tell wich objects don't cast shadows.
 */
class SGShadowAnimation : public SGAnimation
{
public:
  SGShadowAnimation ( SGPropertyNode *prop_root,
                   SGPropertyNode_ptr props );
  virtual ~SGShadowAnimation ();
  virtual int update();
  bool get_condition_value(void);
private:
  SGCondition * _condition;
  bool _condition_value;
};

/**
+ * An "animation" that replace fixed opengl pipeline by shaders
+ */
class SGShaderAnimation : public SGAnimation
{
public:
  SGShaderAnimation ( SGPropertyNode *prop_root,
                   SGPropertyNode_ptr props );
  virtual ~SGShaderAnimation ();
  virtual void init();
  virtual int update();
  bool get_condition_value(void);
private:
  SGCondition * _condition;
  bool _condition_value;
  int _shader_type;
  float _param_1;
  sgVec4 _param_color;
public:
  bool _depth_test;
  float _factor;
  SGPropertyNode_ptr _factor_prop;
  float _speed;
  SGPropertyNode_ptr _speed_prop;
  ssgTexture *_effectTexture;
  unsigned char *_textureData;
  GLint _texWidth, _texHeight;
  sgVec4 _envColor;
};


#endif // _SG_ANIMATION_HXX
