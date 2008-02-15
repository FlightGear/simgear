// particles.hxx - classes to manage particles
// started in 2008 by Tiago Gusmão, using animation.hxx as reference

#ifndef _SG_PARTICLES_HXX
#define _SG_PARTICLES_HXX 1
#endif

#ifndef __cplusplus
# error This library requires C++
#endif 

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osgParticle/SmokeTrailEffect>
#include <osgParticle/Particle>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/ParticleSystem>
#include <osg/MatrixTransform>
#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osg/Notify>
#include <osg/Vec3>


#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/structure/SGExpression.hxx>
#include <simgear/math/SGQuat.hxx>
#include <simgear/math/SGMatrix.hxx>


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "animation.hxx"

class SGGlobalParticleCallback : public osg::NodeCallback 
{
public:
    SGGlobalParticleCallback(const SGPropertyNode* modelRoot) 
    {
        this->modelRoot=modelRoot;
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        SGQuat<float> q = SGQuat<float>::fromLonLatDeg(modelRoot->getFloatValue("/position/longitude-deg",0), modelRoot->getFloatValue("/position/latitude-deg",0));

        SGMatrix<float> m(q);
        osg::Matrix om(m.data());
        osg::Vec3 v(0,0,9.81);
        gravity = om.postMult(v);

        osg::Vec3 w(-modelRoot->getFloatValue("/environment/wind-from-north-fps",0) * SG_FEET_TO_METER, 
            -modelRoot->getFloatValue("/environment/wind-from-east-fps",0) * SG_FEET_TO_METER, 0);
        wind = om.postMult(w);

        //SG_LOG(SG_GENERAL, SG_ALERT, "wind vector:"<<w[0]<<","<<w[1]<<","<<w[2]<<"\n");
    }

    static const osg::Vec3 &getGravityVector()
    {
        return gravity;
    }

    static const osg::Vec3 &getWindVector()
    {
        return wind;
    }


    static osg::Vec3 gravity;
    static osg::Vec3 wind;
private:

    const SGPropertyNode* modelRoot;
};



class SGParticles : public osg::NodeCallback 
{
public:
    SGParticles( ) : 
      shooterValue(NULL),
          counterValue(NULL),
          startSizeValue(NULL),
          endSizeValue(NULL),
          lifeValue(NULL),
          refFrame(NULL),
          program(NULL),
          useGravity(false),
          useWind(false),
          counterCond(NULL)
      {
          memset(colorComponents, 0, sizeof(colorComponents));
      }

      static osg::Group * appendParticles(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::ReaderWriter::Options* options);

      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
          //SG_LOG(SG_GENERAL, SG_ALERT, "callback!\n");

          if(shooterValue)
              shooter->setInitialSpeedRange(shooterValue->getValue(), shooterValue->getValue()+shooterExtraRange);
          if(counterValue)
              counter->setRateRange(counterValue->getValue(), counterValue->getValue()+counterExtraRange);
          else if(counterCond)
              counter->setRateRange(counterStaticValue, counterStaticValue + counterStaticExtraRange);
          if(counterCond && !counterCond->test())
              counter->setRateRange(0,0);
          bool colorchange=false;
          for(int i=0;i<8;++i){
              if(colorComponents[i]){
                  staticColorComponents[i] = colorComponents[i]->getValue();
                  colorchange=true;
              }
          }
          if(colorchange)
              particleSys->getDefaultParticleTemplate().setColorRange(osgParticle::rangev4( osg::Vec4(staticColorComponents[0], staticColorComponents[1], staticColorComponents[2], staticColorComponents[3]), osg::Vec4(staticColorComponents[4], staticColorComponents[5], staticColorComponents[6], staticColorComponents[7])));
          if(startSizeValue)
              startSize = startSizeValue->getValue();
          if(endSizeValue)
              endSize = endSizeValue->getValue();
          if(startSizeValue || endSizeValue)
              particleSys->getDefaultParticleTemplate().setSizeRange(osgParticle::rangef(startSize, endSize));
          if(lifeValue)
              particleSys->getDefaultParticleTemplate().setLifeTime(lifeValue->getValue());
          if(program){
              if(useGravity)
                  program->setAcceleration(SGGlobalParticleCallback::getGravityVector());
              if(useWind)
                  program->setWind(SGGlobalParticleCallback::getWindVector());
          }
      }

      inline void setupShooterSpeedData(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
      {
          shooterValue = read_value(configNode, modelRoot, "-m", -SGLimitsd::max(), SGLimitsd::max());
          if(!shooterValue){
              SG_LOG(SG_GENERAL, SG_ALERT, "shooter property error!\n");
          }
          shooterExtraRange = configNode->getFloatValue("extrarange",0);
      }

      inline void setupCounterData(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
      {
          counterValue = read_value(configNode, modelRoot, "-m", -SGLimitsd::max(), SGLimitsd::max());
          if(!counterValue){
              SG_LOG(SG_GENERAL, SG_ALERT, "counter property error!\n");
          }
          counterExtraRange = configNode->getFloatValue("extrarange",0);
      }

      inline void setupCounterCondition(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
      {
          counterCond = sgReadCondition(modelRoot, configNode);
      }

      inline void setupCounterCondition(float counterStaticValue, float counterStaticExtraRange)
      {
          this->counterStaticValue = counterStaticValue;
          this->counterStaticExtraRange = counterStaticExtraRange;
      }

      inline void setupStartSizeData(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
      {
          startSizeValue = read_value(configNode, modelRoot, "-m", -SGLimitsd::max(), SGLimitsd::max());
          if(!startSizeValue) 
          {
              SG_LOG(SG_GENERAL, SG_ALERT, "startSizeValue error!\n");
          }
      }

      inline void setupEndSizeData(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
      {
          endSizeValue = read_value(configNode, modelRoot, "-m", -SGLimitsd::max(), SGLimitsd::max());
          if(!endSizeValue){
              SG_LOG(SG_GENERAL, SG_ALERT, "startSizeValue error!\n");
          }
      }

      inline void setupLifeData(const SGPropertyNode* configNode, SGPropertyNode* modelRoot)
      {
          lifeValue = read_value(configNode, modelRoot, "-m", -SGLimitsd::max(), SGLimitsd::max());
          if(!lifeValue){
              SG_LOG(SG_GENERAL, SG_ALERT, "lifeValue error!\n");
          }
      }

      inline void setupStaticSizeData(float startSize, float endSize)
      {
          this->startSize=startSize;
          this->endSize=endSize;
      }


      inline void setGeneralData(osgParticle::RadialShooter * shooter, osgParticle::RandomRateCounter * counter, osgParticle::ParticleSystem * particleSys, osgParticle::FluidProgram *program)
      {
          this->shooter = shooter;
          this->counter = counter;
          this->particleSys = particleSys;
          this->program = program;
      }

      inline void setupProgramGravity(bool useGravity)
      {
          this->useGravity = useGravity;
      }

      inline void setupProgramWind(bool useWind)
      {
          this->useWind = useWind;
      }

      //component: r=0, g=1, ... ; color: 0=start color, 1=end color
      inline void setupColorComponent(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, int color, int component)
      {
          SGExpressiond *colorValue = read_value(configNode, modelRoot, "-m", -SGLimitsd::max(), SGLimitsd::max());
          if(!colorValue){
              SG_LOG(SG_GENERAL, SG_ALERT, "color property error!\n");
          }
          colorComponents[(color*4)+component] = colorValue;
          //number of color components = 4
      }

      inline void setupStaticColorComponent(float r1,float g1, float b1, float a1, float r2,
          float g2, float b2, float a2)
      {
          staticColorComponents[0] = r1;
          staticColorComponents[1] = g1;
          staticColorComponents[2] = b1;
          staticColorComponents[3] = a1;
          staticColorComponents[4] = r2;
          staticColorComponents[5] = g2;
          staticColorComponents[6] = b2;
          staticColorComponents[7] = a2;
      }

      static inline osg::Group * getCommonRoot()
      {
          if(!commonRoot.valid())
          {
              SG_LOG(SG_GENERAL, SG_DEBUG, "Particle common root called!\n");
              commonRoot = new osg::Group;
              commonRoot.get()->setName("common particle system root");
              commonGeode.get()->setName("common particle system geode");
              commonRoot.get()->addChild(commonGeode.get());
              commonRoot.get()->addChild(psu.get());
          }
          return commonRoot.get();
      }

      static inline osg::Geode * getCommonGeode()
      {
          return commonGeode.get();
      }

      static inline osgParticle::ParticleSystemUpdater * getPSU()
      {
          return psu.get();
      }

private:
    float shooterExtraRange;
    float counterExtraRange;
    SGExpressiond * shooterValue;
    SGExpressiond * counterValue;
    SGExpressiond * colorComponents[8];
    SGExpressiond * startSizeValue;
    SGExpressiond * endSizeValue;
    SGExpressiond * lifeValue;
    SGCondition * counterCond;
    osg::MatrixTransform *refFrame;
    float staticColorComponents[8];
    float startSize;
    float endSize;
    float counterStaticValue;
    float counterStaticExtraRange;
    osgParticle::RadialShooter * shooter;
    osgParticle::RandomRateCounter * counter;
    osgParticle::ParticleSystem * particleSys;
    osgParticle::FluidProgram * program;
    bool useGravity;
    bool useWind;
    static osg::ref_ptr<osgParticle::ParticleSystemUpdater> psu;
    static osg::ref_ptr<osg::Group> commonRoot;
    static osg::ref_ptr<osg::Geode> commonGeode;
};


/*
class CustomModularEmitter : public osgParticle::ModularEmitter, public osg::Observer
{
public:

virtual void objectDeleted (void *)
{
SG_LOG(SG_GENERAL, SG_ALERT, "object deleted!\n");

SGParticles::getCommonRoot()->removeChild(this->getParticleSystem()->getParent(0));
SGParticles::getPSU()->removeParticleSystem(this->getParticleSystem());
}


//   ~CustomModularEmitter()
//   {
//     SG_LOG(SG_GENERAL, SG_ALERT, "object deleted!\n");
//     
//     SGParticles::getCommonRoot()->removeChild(this->getParticleSystem()->getParent(0));
//     SGParticles::getPSU()->removeParticleSystem(this->getParticleSystem());
//   }
};
*/
