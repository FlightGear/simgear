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

private:
    static osg::Vec3 gravity;
    static osg::Vec3 wind;
    const SGPropertyNode* modelRoot;
};



class Particles : public osg::NodeCallback 
{
public:
    Particles() : 
        shooterValue(NULL),
        counterValue(NULL),
        startSizeValue(NULL),
        endSizeValue(NULL),
        lifeValue(NULL),
        counterCond(NULL),
        refFrame(NULL),
        program(NULL),
        useGravity(false),
        useWind(false)
    {
        memset(colorComponents, 0, sizeof(colorComponents));
    }

    static osg::Group * appendParticles(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::ReaderWriter::Options* options);

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

    static osg::Group * getCommonRoot()
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

    static osg::Geode * getCommonGeode()
    {
        return commonGeode.get();
    }

    static osgParticle::ParticleSystemUpdater * getPSU()
    {
        return psu.get();
    }

protected:
    float shooterExtraRange;
    float counterExtraRange;
    SGExpressiond* shooterValue;
    SGExpressiond* counterValue;
    SGExpressiond* colorComponents[8];
    SGExpressiond* startSizeValue;
    SGExpressiond* endSizeValue;
    SGExpressiond* lifeValue;
    SGCondition* counterCond;
    osg::MatrixTransform* refFrame;
    float staticColorComponents[8];
    float startSize;
    float endSize;
    float counterStaticValue;
    float counterStaticExtraRange;
    osgParticle::RadialShooter* shooter;
    osgParticle::RandomRateCounter* counter;
    osgParticle::ParticleSystem* particleSys;
    osgParticle::FluidProgram* program;
    bool useGravity;
    bool useWind;
    static osg::ref_ptr<osgParticle::ParticleSystemUpdater> psu;
    static osg::ref_ptr<osg::Group> commonRoot;
    static osg::ref_ptr<osg::Geode> commonGeode;
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
