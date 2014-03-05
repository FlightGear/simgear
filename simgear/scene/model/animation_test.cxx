#include "animation.hxx"

#include <cstring>
#include <iostream>

#define VERIFY_CLOSE(a, b) \
  if( norm((a) - (b)) > 1e-5 ) \
  { \
    std::cerr << "line " << __LINE__ << ": failed: "\
              << #a << " != " << #b\
              << " [" << (a) << " != " << (b) << "]" << std::endl; \
    return 1; \
  }

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "failed: line " << __LINE__ << ": " << #a << std::endl; \
    return 1; \
  }

struct AnimationTest:
  public SGAnimation
{
  AnimationTest(const SGPropertyNode* n):
    SGAnimation(n, 0)
  {}

  void readConfig()
  {
    readRotationCenterAndAxis(center, axis);
  }

  SGVec3d center,
          axis;
};

int main(int argc, char* argv[])
{
  SGPropertyNode_ptr config = new SGPropertyNode;
  AnimationTest anim(config);

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
