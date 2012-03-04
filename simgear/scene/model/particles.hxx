// particles.hxx - classes to manage particles
// Copyright (C) 2008 Tiago Gusmão
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

#ifndef _SG_PARTICLES_HXX
#define _SG_PARTICLES_HXX 1

#include <osg/ref_ptr>
#include <osgParticle/SmokeTrailEffect>
#include <osgParticle/Particle>
#include <osgParticle/ModularEmitter>
#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osg/Notify>
#include <osg/Vec3>

namespace osg
{
class Group;
class MatrixTransform;
class Node;
class NodeVisitor;
}

namespace osgParticle
{
class ParticleSystem;
class ParticleSystemUpdater;
}

#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/structure/SGExpression.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
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

namespace simgear
{

class GlobalParticleCallback : public osg::NodeCallback 
{
public:
    GlobalParticleCallback(const SGPropertyNode* modelRoot) 
    {
        this->modelRoot=modelRoot;
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    static const osg::Vec3 &getGravityVector()
    {
        return gravity;
    }

    static const osg::Vec3 &getWindVector()
    {
        return wind;
    }

    static void setSwitch(const SGPropertyNode* n)
    {
        enabledNode = n;
    }

    static bool getEnabled()
    {
        return enabled;
    }

private:
    static osg::Vec3 gravity;
    static osg::Vec3 wind;
    SGConstPropertyNode_ptr modelRoot;
    static SGConstPropertyNode_ptr enabledNode;
    static bool enabled;
};



class Particles : public osg::NodeCallback 
{
public:
    Particles();

    static osg::Group * appendParticles(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::Options* options);

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

    void setupShooterSpeedData(const SGPropertyNode* configNode,
                               SGPropertyNode* modelRoot)
    {
        shooterValue = read_value(configNode, modelRoot, "-m",
                                  -SGLimitsd::max(), SGLimitsd::max());
        if(!shooterValue){
            SG_LOG(SG_GENERAL, SG_ALERT, "shooter property error!\n");
        }
        shooterExtraRange = configNode->getFloatValue("extrarange",0);
    }

    void setupCounterData(const SGPropertyNode* configNode,
                          SGPropertyNode* modelRoot)
    {
        counterValue = read_value(configNode, modelRoot, "-m",
                                  -SGLimitsd::max(), SGLimitsd::max());
        if(!counterValue){
            SG_LOG(SG_GENERAL, SG_ALERT, "counter property error!\n");
        }
        counterExtraRange = configNode->getFloatValue("extrarange",0);
    }

    void setupCounterCondition(const SGPropertyNode* configNode,
                               SGPropertyNode* modelRoot)
    {
        counterCond = sgReadCondition(modelRoot, configNode);
    }

    void setupCounterCondition(float counterStaticValue,
                               float counterStaticExtraRange)
    {
        this->counterStaticValue = counterStaticValue;
        this->counterStaticExtraRange = counterStaticExtraRange;
    }

    void setupStartSizeData(const SGPropertyNode* configNode,
                            SGPropertyNode* modelRoot)
    {
        startSizeValue = read_value(configNode, modelRoot, "-m",
                                    -SGLimitsd::max(), SGLimitsd::max());
        if(!startSizeValue) 
        {
            SG_LOG(SG_GENERAL, SG_ALERT, "startSizeValue error!\n");
        }
    }

    void setupEndSizeData(const SGPropertyNode* configNode,
                          SGPropertyNode* modelRoot)
    {
        endSizeValue = read_value(configNode, modelRoot, "-m",
                                  -SGLimitsd::max(), SGLimitsd::max());
        if(!endSizeValue){
            SG_LOG(SG_GENERAL, SG_ALERT, "startSizeValue error!\n");
        }
    }

    void setupLifeData(const SGPropertyNode* configNode,
                       SGPropertyNode* modelRoot)
    {
        lifeValue = read_value(configNode, modelRoot, "-m",
                               -SGLimitsd::max(), SGLimitsd::max());
        if(!lifeValue){
            SG_LOG(SG_GENERAL, SG_ALERT, "lifeValue error!\n");
        }
    }

    void setupStaticSizeData(float startSize, float endSize)
    {
        this->startSize=startSize;
        this->endSize=endSize;
    }


    void setGeneralData(osgParticle::RadialShooter* shooter,
                        osgParticle::RandomRateCounter* counter,
                        osgParticle::ParticleSystem* particleSys,
                        osgParticle::FluidProgram* program)
    {
        this->shooter = shooter;
        this->counter = counter;
        this->particleSys = particleSys;
        this->program = program;
    }

    void setupProgramGravity(bool useGravity)
    {
        this->useGravity = useGravity;
    }

    void setupProgramWind(bool useWind)
    {
        this->useWind = useWind;
    }

    //component: r=0, g=1, ... ; color: 0=start color, 1=end color
    void setupColorComponent(const SGPropertyNode* configNode,
                             SGPropertyNode* modelRoot, int color,
                             int component)
    {
        SGExpressiond *colorValue = read_value(configNode, modelRoot, "-m",
                                               -SGLimitsd::max(),
                                               SGLimitsd::max());
        if(!colorValue){
            SG_LOG(SG_GENERAL, SG_ALERT, "color property error!\n");
        }
        colorComponents[(color*4)+component] = colorValue;
        //number of color components = 4
    }

    void setupStaticColorComponent(float r1,float g1, float b1, float a1,
                                   float r2, float g2, float b2, float a2)
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

    static osg::Group * getCommonRoot();

    static osg::Geode * getCommonGeode()
    {
        return commonGeode.get();
    }

    static osgParticle::ParticleSystemUpdater * getPSU()
    {
        return psu.get();
    }

    static void setFrozen(bool e) { _frozen = e; }

    /**
     *  Set and get the wind vector for particles in the
     * atmosphere. This vector is in the Z-up Y-north frame, and the
     * magnitude is the velocity in meters per second.
     */
    static void setWindVector(const osg::Vec3& wind) { _wind = wind; }
    static void setWindFrom(const double from_deg, const double speed_kt) {
	double map_rad = -from_deg * SG_DEGREES_TO_RADIANS;
	double speed_mps = speed_kt * SG_KT_TO_MPS;
	_wind[0] = cos(map_rad) * speed_mps;
	_wind[1] = sin(map_rad) * speed_mps;
	_wind[2] = 0.0;
    }
    static const osg::Vec3& getWindVector() { return _wind; }
protected:
    float shooterExtraRange;
    float counterExtraRange;
    SGSharedPtr<SGExpressiond> shooterValue;
    SGSharedPtr<SGExpressiond> counterValue;
    SGSharedPtr<SGExpressiond> colorComponents[8];
    SGSharedPtr<SGExpressiond> startSizeValue;
    SGSharedPtr<SGExpressiond> endSizeValue;
    SGSharedPtr<SGExpressiond> lifeValue;
    SGSharedPtr<SGCondition> counterCond;
    float staticColorComponents[8];
    float startSize;
    float endSize;
    float counterStaticValue;
    float counterStaticExtraRange;
    osg::ref_ptr<osgParticle::RadialShooter> shooter;
    osg::ref_ptr<osgParticle::RandomRateCounter> counter;
    osg::ref_ptr<osgParticle::ParticleSystem> particleSys;
    osg::ref_ptr<osgParticle::FluidProgram> program;
    osg::ref_ptr<osg::MatrixTransform> particleFrame;
    
    bool useGravity;
    bool useWind;
    static bool _frozen;
    static osg::ref_ptr<osgParticle::ParticleSystemUpdater> psu;
    static osg::ref_ptr<osg::Group> commonRoot;
    static osg::ref_ptr<osg::Geode> commonGeode;
    static osg::Vec3 _wind;
};
}
#endif


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
