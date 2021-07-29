// particles.hxx - classes to manage particles
// Copyright (C) 2008 Tiago Gusm�o
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
}

#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/structure/SGExpression.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGQuat.hxx>
#include <simgear/math/SGMatrix.hxx>


namespace simgear
{

class ParticlesGlobalManager;

class Particles : public osg::NodeCallback 
{
public:
    Particles() = default;

    void operator()(osg::Node* node, osg::NodeVisitor* nv) override;

    void setupShooterSpeedData(const SGPropertyNode* configNode,
                               SGPropertyNode* modelRoot);

    void setupCounterData(const SGPropertyNode* configNode,
                          SGPropertyNode* modelRoot);

    void setupCounterCondition(const SGPropertyNode* configNode,
                               SGPropertyNode* modelRoot);
    void setupCounterCondition(float counterStaticValue,
                               float counterStaticExtraRange);

    void setupStartSizeData(const SGPropertyNode* configNode,
                            SGPropertyNode* modelRoot);

    void setupEndSizeData(const SGPropertyNode* configNode,
                          SGPropertyNode* modelRoot);

    void setupLifeData(const SGPropertyNode* configNode,
                       SGPropertyNode* modelRoot);

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
                             int component);

    void setupStaticColorComponent(float r1, float g1, float b1, float a1,
                                   float r2, float g2, float b2, float a2);

protected:
    friend class ParticlesGlobalManager;

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

    bool useGravity = false;
    bool useWind = false;
};

class ParticlesGlobalManager
{
public:
    ~ParticlesGlobalManager();

    static ParticlesGlobalManager* instance();
    static void clear();

    bool isEnabled() const;

    osg::ref_ptr<osg::Group> appendParticles(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::Options* options);

    osg::Group* getCommonRoot();
    osg::Geode* getCommonGeode();

    void initFromMainThread();

    void setFrozen(bool e);
    bool isFrozen() const;

    /**
        @brief update function: call from the main thread , outside of OSG calls.
     */
    void update(double dt, const SGGeod& pos);

    void setSwitchNode(const SGPropertyNode* n);

    /**
     *  Set and get the wind vector for particles in the
     * atmosphere. This vector is in the Z-up Y-north frame, and the
     * magnitude is the velocity in meters per second.
     */
    void setWindVector(const osg::Vec3& wind);
    void setWindFrom(const double from_deg, const double speed_kt);

    osg::Vec3 getWindVector() const;

private:
    ParticlesGlobalManager();

    class ParticlesGlobalManagerPrivate;
    class UpdaterCallback;
    class RegistrationCallback;

    // because Private inherits NodeCallback, we need to own it
    // via an osg::ref_ptr
    osg::ref_ptr<ParticlesGlobalManagerPrivate> d;
};

} // namespace simgear

#endif
