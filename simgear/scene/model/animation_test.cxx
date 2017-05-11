#include <simgear_config.h>
#include "animation.hxx"

#include <cstring>
#include <iostream>

#include <simgear/misc/test_macros.hxx>
#include <simgear/scene/util/SGTransientModelData.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#define VERIFY_CLOSE(a, b) SG_VERIFY( norm((a) - (b)) <= 1e-5 )


struct AnimationTest:
  public SGAnimation
{
  AnimationTest(simgear::SGTransientModelData transientModelData):
    SGAnimation(transientModelData), modelData(transientModelData)
  {}

  void readConfig()
  {
    readRotationCenterAndAxis(0, center, axis, modelData);
  }

  simgear::SGTransientModelData &modelData;
  SGVec3d center,
          axis;
};

int main(int argc, char* argv[])
{
  SGPropertyNode_ptr config = new SGPropertyNode;

  simgear::SGTransientModelData transientModelData(nullptr, config, config, nullptr, "", 0);
  AnimationTest anim(transientModelData);

  SGVec3d v1(1, 2, 3),
          v2(-1, 4, 0);
  config->setDoubleValue("axis/x1-m", v1.x());
  config->setDoubleValue("axis/y1-m", v1.y());
  config->setDoubleValue("axis/z1-m", v1.z());
  config->setDoubleValue("axis/x2-m", v2.x());
  config->setDoubleValue("axis/y2-m", v2.y());
  config->setDoubleValue("axis/z2-m", v2.z());
  anim.readConfig();

  VERIFY_CLOSE(anim.center, (v1 + v2) * 0.5)
  VERIFY_CLOSE(anim.axis, normalize(v2 - v1))

  config->removeChild("axis", 0);
  config->setDoubleValue("center/x-m", v1.x());
  config->setDoubleValue("center/y-m", v1.y());
  config->setDoubleValue("center/z-m", v1.z());
  config->setDoubleValue("axis/x", v2.x());
  config->setDoubleValue("axis/y", v2.y());
  config->setDoubleValue("axis/z", v2.z());
  anim.readConfig();

  VERIFY_CLOSE(anim.center, v1)
  VERIFY_CLOSE(anim.axis, normalize(v2))

  return 0;
}
