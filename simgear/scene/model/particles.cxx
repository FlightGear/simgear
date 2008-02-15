// particles.cxx - classes to manage particles
// started in 2008 by Tiago Gusmão, using animation.hxx as reference

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <osgParticle/SmokeTrailEffect>
#include <osgParticle/FireEffect>
#include <osgParticle/ConnectedParticleSystem>
#include <osgParticle/MultiSegmentPlacer>
#include <osgParticle/SectorPlacer>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/FluidProgram>

#include <osg/Geode>
#include <osg/MatrixTransform>

#include "particles.hxx"

//static members
osg::Vec3 SGGlobalParticleCallback::gravity;
osg::Vec3 SGGlobalParticleCallback::wind;

osg::ref_ptr<osg::Group> SGParticles::commonRoot;
osg::ref_ptr<osgParticle::ParticleSystemUpdater> SGParticles::psu = new osgParticle::ParticleSystemUpdater;
osg::ref_ptr<osg::Geode> SGParticles::commonGeode = new osg::Geode;;

osg::Group * SGParticles::appendParticles(const SGPropertyNode* configNode, SGPropertyNode* modelRoot, const osgDB::ReaderWriter::Options* options)
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Setting up a particle system!\n");

    osgParticle::ParticleSystem *particleSys;

    //create a generic particle system
    std::string type = configNode->getStringValue("type", "normal");
    if (type == "normal")particleSys= new osgParticle::ParticleSystem;
    else particleSys= new osgParticle::ConnectedParticleSystem;
    SGParticles *callback=NULL;  //may not be used depending on the configuration

    getPSU()->addParticleSystem(particleSys); 

    getPSU()->setUpdateCallback(new SGGlobalParticleCallback(modelRoot));

    //contains counter, placer and shooter by default
    osgParticle::ModularEmitter *emitter = new osgParticle::ModularEmitter;

    emitter->setParticleSystem(particleSys);

    // Set up the alignment node ("stolen" from animation.cxx)
    osg::MatrixTransform *align = new osg::MatrixTransform;
    osg::Matrix res_matrix;
    res_matrix.makeRotate(
        configNode->getFloatValue("offsets/pitch-deg", 0.0)*SG_DEGREES_TO_RADIANS,
        osg::Vec3(0, 1, 0),
        configNode->getFloatValue("offsets/roll-deg", 0.0)*SG_DEGREES_TO_RADIANS,
        osg::Vec3(1, 0, 0),
        configNode->getFloatValue("offsets/heading-deg", 0.0)*SG_DEGREES_TO_RADIANS,
        osg::Vec3(0, 0, 1));

    osg::Matrix tmat;
    tmat.makeTranslate(configNode->getFloatValue("offsets/x-m", 0.0),
        configNode->getFloatValue("offsets/y-m", 0.0),
        configNode->getFloatValue("offsets/z-m", 0.0));
    align->setMatrix(res_matrix*tmat);

    align->setName("particle align");

    //if(dynamic_cast<CustomModularEmitter*>(emitter)==0) SG_LOG(SG_GENERAL, SG_ALERT, "observer error\n");
    //align->addObserver(dynamic_cast<CustomModularEmitter*>(emitter));

    align->addChild(emitter);

    //this name can be used in the XML animation as if it was a submodel
    std::string name = configNode->getStringValue("name", "");
    if(!name.empty()) align->setName(name);

    std::string attach = configNode->getStringValue("attach", "world");

    if (attach == "local") { //local means attached to the model and not the world
        osg::Geode *g = new osg::Geode;
        align->addChild(g);
        g->addDrawable(particleSys);
        emitter->setReferenceFrame(osgParticle::Emitter::ABSOLUTE_RF);
    } else 
        getCommonGeode()->addDrawable(particleSys);

    std::string textureFile;

    if(configNode->hasValue("texture")) {
        SG_LOG(SG_GENERAL, SG_ALERT, "requested:"<<configNode->getStringValue("texture","")<<"\n");
        textureFile= osgDB::findFileInPath(configNode->getStringValue("texture",""),
            options->getDatabasePathList());
        SG_LOG(SG_GENERAL, SG_ALERT, "found:"<<textureFile<<"\n");

        for(int i=0;i<options->getDatabasePathList().size();++i)
            SG_LOG(SG_GENERAL, SG_ALERT, "opts:"<<options->getDatabasePathList()[i]<<"\n");

    }

    if(textureFile.empty())
        textureFile="";

    particleSys->setDefaultAttributes(textureFile,configNode->getBoolValue("emissive", true),
        configNode->getBoolValue("lighting", false));

    std::string alignstr = configNode->getStringValue("align","billboard");

    if(alignstr == "fixed")
        particleSys->setParticleAlignment(osgParticle::ParticleSystem::FIXED);

    const SGPropertyNode* placernode = configNode->getChild("placer");

    if(placernode) {
        std::string emitterType = placernode->getStringValue("type", "point");

        if (emitterType == "sector") {
            osgParticle::SectorPlacer *splacer = new  osgParticle::SectorPlacer;
            float minRadius, maxRadius, minPhi, maxPhi;

            minRadius = placernode->getFloatValue("radius-min-m",0);
            maxRadius = placernode->getFloatValue("radius-max-m",1);
            minPhi = placernode->getFloatValue("phi-min-deg",0) * SG_DEGREES_TO_RADIANS;
            maxPhi = placernode->getFloatValue("phi-max-deg",360.0f) * SG_DEGREES_TO_RADIANS;

            splacer->setRadiusRange(minRadius, maxRadius);
            splacer->setPhiRange(minPhi, maxPhi);
            emitter->setPlacer(splacer);
        } else if (emitterType == "segments") {
            std::vector<SGPropertyNode_ptr> segments = placernode->getChildren("vertex");

            if(segments.size()>1) {
                osgParticle::MultiSegmentPlacer *msplacer = new  osgParticle::MultiSegmentPlacer();
                float x,y,z;

                for (unsigned i = 0; i < segments.size(); ++i) {
                    x = segments[i]->getFloatValue("x-m",0);
                    y = segments[i]->getFloatValue("y-m",0);
                    z = segments[i]->getFloatValue("z-m",0);
                    msplacer->addVertex(x, y, z);
                }

                emitter->setPlacer(msplacer);
            }
            else SG_LOG(SG_GENERAL, SG_ALERT, "Detected particle system using segment(s) with less than 2 vertices\n");
        } //else the default placer in ModularEmitter is used (PointPlacer)
    }

    const SGPropertyNode* shnode = configNode->getChild("shooter");

    if(shnode) {
        float minTheta, maxTheta, minPhi, maxPhi, speed, spread;

        minTheta = shnode->getFloatValue("theta-min-deg",0) * SG_DEGREES_TO_RADIANS;
        maxTheta = shnode->getFloatValue("theta-max-deg",360.0f) * SG_DEGREES_TO_RADIANS;
        minPhi = shnode->getFloatValue("phi-min-deg",0)* SG_DEGREES_TO_RADIANS;
        maxPhi = shnode->getFloatValue("phi-max-deg",360.0f)* SG_DEGREES_TO_RADIANS; 

        osgParticle::RadialShooter *shooter = new osgParticle::RadialShooter;
        emitter->setShooter(shooter);

        shooter->setThetaRange(minTheta, maxTheta);
        shooter->setPhiRange(minPhi, maxPhi);

        const SGPropertyNode* speednode = shnode->getChild("speed");

        if(speednode) {
            if(speednode->hasValue("value")) {
                speed = speednode->getFloatValue("value",0);
                spread = speednode->getFloatValue("spread",0);
                shooter->setInitialSpeedRange(speed-spread, speed+spread);
            } else {

                if(!callback)
                    callback = new SGParticles();

                callback->setupShooterSpeedData(speednode, modelRoot);
            }
        }

        const SGPropertyNode* rotspeednode = shnode->getChild("rotspeed");

        if(rotspeednode) {
            float x1,y1,z1,x2,y2,z2;
            x1 = rotspeednode->getFloatValue("x-min-deg-sec",0) * SG_DEGREES_TO_RADIANS;
            y1 = rotspeednode->getFloatValue("y-min-deg-sec",0) * SG_DEGREES_TO_RADIANS;
            z1 = rotspeednode->getFloatValue("z-min-deg-sec",0) * SG_DEGREES_TO_RADIANS;
            x2 = rotspeednode->getFloatValue("x-max-deg-sec",0) * SG_DEGREES_TO_RADIANS;
            y2 = rotspeednode->getFloatValue("y-max-deg-sec",0) * SG_DEGREES_TO_RADIANS;
            z2 = rotspeednode->getFloatValue("z-max-deg-sec",0) * SG_DEGREES_TO_RADIANS;
            shooter->setInitialRotationalSpeedRange(osg::Vec3f(x1,y1,z1), osg::Vec3f(x2,y2,z2));
        }
    } //else ModularEmitter uses the default RadialShooter

    const SGPropertyNode* counternode = configNode->getChild("counter");

    if(counternode) {
        osgParticle::RandomRateCounter *counter = new osgParticle::RandomRateCounter;
        emitter->setCounter(counter);
        float pps, spread;
        const SGPropertyNode* ppsnode = counternode->getChild("pps");

        if(ppsnode) {

            if(ppsnode->hasValue("value")) {
                pps = ppsnode->getFloatValue("value",0);
                spread = ppsnode->getFloatValue("spread",0);
                counter->setRateRange(pps-spread, pps+spread);
            } else {

                if(!callback)
                    callback = new SGParticles();

                callback->setupCounterData(ppsnode, modelRoot);
            }
        }
        const SGPropertyNode* conditionNode = counternode->getChild("condition");

        if(conditionNode) {

            if(!callback)
                callback = new SGParticles();
            callback->setupCounterCondition(conditionNode, modelRoot);
            callback->setupCounterCondition(pps, spread);
        }
    } //TODO: else perhaps set higher values than default? 

    const SGPropertyNode* particlenode = configNode->getChild("particle");

    if(particlenode) {
        osgParticle::Particle &particle = particleSys->getDefaultParticleTemplate();
        float r1=0, g1=0, b1=0, a1=1, r2=0, g2=0, b2=0, a2=1;

        const SGPropertyNode* startcolornode = particlenode->getChild("startcolor");

        if(startcolornode) {
            const SGPropertyNode* componentnode = startcolornode->getChild("red");

            if(componentnode) {

                if(componentnode->hasValue("value"))
                    r1 = componentnode->getFloatValue("value",0);
                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 0, 0);
                }
            }

            componentnode = startcolornode->getChild("green");
            if(componentnode) {

                if(componentnode->hasValue("value"))
                    g1 = componentnode->getFloatValue("value",0);

                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 0, 1);
                }
            }

            componentnode = startcolornode->getChild("blue");
            if(componentnode) {

                if(componentnode->hasValue("value"))
                    b1 = componentnode->getFloatValue("value",0);
                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 0, 2);
                }
            }

            componentnode = startcolornode->getChild("alpha");

            if(componentnode) {

                if(componentnode->hasValue("value"))
                    a1 = componentnode->getFloatValue("value",0);
                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 0, 3);
                }
            }
        }

        const SGPropertyNode* endcolornode = particlenode->getChild("endcolor");
        if(endcolornode) {
            const SGPropertyNode* componentnode = endcolornode->getChild("red");

            if(componentnode) {

                if(componentnode->hasValue("value"))
                    r2 = componentnode->getFloatValue("value",0);

                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 1, 0);
                }
            }

            componentnode = endcolornode->getChild("green");

            if(componentnode) {

                if(componentnode->hasValue("value"))
                    g2 = componentnode->getFloatValue("value",0);

                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 1, 1);
                }
            }

            componentnode = endcolornode->getChild("blue");

            if(componentnode) {

                if(componentnode->hasValue("value"))
                    b2 = componentnode->getFloatValue("value",0);

                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 1, 2);
                }
            }

            componentnode = endcolornode->getChild("alpha");
            if(componentnode) {

                if(componentnode->hasValue("value"))
                    a2 = componentnode->getFloatValue("value",0);

                else {

                    if(!callback)
                        callback = new SGParticles();

                    callback->setupColorComponent(componentnode, modelRoot, 1, 3);
                }
            }
        }

        particle.setColorRange(osgParticle::rangev4( osg::Vec4(r1,g1,b1,a1), osg::Vec4(r2,g2,b2,a2)));

        float startsize=1, endsize=0.1f;
        const SGPropertyNode* startsizenode = particlenode->getChild("startsize");

        if(startsizenode) {

            if(startsizenode->hasValue("value"))
                startsize = startsizenode->getFloatValue("value",0);

            else {

                if(!callback)
                    callback = new SGParticles();

                callback->setupStartSizeData(startsizenode, modelRoot);
            }
        }

        const SGPropertyNode* endsizenode = particlenode->getChild("endsize");
        if(endsizenode) {

            if(endsizenode->hasValue("value"))
                endsize = endsizenode->getFloatValue("value",0);

            else {

                if(!callback)
                    callback = new SGParticles();

                callback->setupEndSizeData(endsizenode, modelRoot);
            }
        }

        particle.setSizeRange(osgParticle::rangef(startsize, endsize));

        float life=5;
        const SGPropertyNode* lifenode = particlenode->getChild("life-sec");

        if(lifenode) {

            if(lifenode->hasValue("value"))
                life =  lifenode->getFloatValue("value",0);

            else {

                if(!callback)
                    callback = new SGParticles();

                callback->setupLifeData(lifenode, modelRoot);
            }
        }

        particle.setLifeTime(life);

        if(particlenode->hasValue("radius-m"))
            particle.setRadius(particlenode->getFloatValue("radius-m",0));

        if(particlenode->hasValue("mass-kg"))
            particle.setMass(particlenode->getFloatValue("mass-kg",0));

        if(callback) {
            callback->setupStaticColorComponent(r1, g1, b1, a1, r2, g2, b2, a2);
            callback->setupStaticSizeData(startsize, endsize);
        }
        //particle.setColorRange(osgParticle::rangev4( osg::Vec4(r1, g1, b1, a1), osg::Vec4(r2, g2, b2, a2)));
    }

    const SGPropertyNode* programnode = configNode->getChild("program");
    osgParticle::FluidProgram *program = new osgParticle::FluidProgram();

    if(programnode) {
        std::string fluid = programnode->getStringValue("fluid","air");

        if(fluid=="air") 
            program->setFluidToAir();

        else
            program->setFluidToWater();

        std::string grav = programnode->getStringValue("gravity","enabled");

        if(grav=="enabled") {

            if(attach == "world") {

                if(!callback)
                    callback = new SGParticles();

                callback->setupProgramGravity(true);
            } else
                program->setToGravity();

        } else
            program->setAcceleration(osg::Vec3(0,0,0));

        std::string wind = programnode->getStringValue("wind","enabled");
        if(wind=="enabled") {
            if(!callback)
                callback = new SGParticles();
            callback->setupProgramWind(true);
        } else
            program->setWind(osg::Vec3(0,0,0));

        align->addChild(program);

        program->setParticleSystem(particleSys);
    }
    else {  }

    if(callback) {  //this means we want property-driven changes
        SG_LOG(SG_GENERAL, SG_DEBUG, "setting up particle system user data and callback\n");
        //setup data and callback
        callback->setGeneralData(dynamic_cast<osgParticle::RadialShooter*>(emitter->getShooter()), dynamic_cast<osgParticle::RandomRateCounter*>(emitter->getCounter()), particleSys, program);
        emitter->setUpdateCallback(callback);
    }

    return align;
}
