#include <simgear_config.h>
#include <simgear/compiler.h>

#include "parse_color.hxx"
#include "ColorInterpolator.hxx"
#include <simgear/props/props.hxx>
#include <simgear/misc/test_macros.hxx>

#include <iostream>
#include <memory>

#define VERIFY_COLOR(str, r, g, b, a) \
  SG_VERIFY(simgear::parseColor(str, color)) \
  SG_CHECK_EQUAL_NOSTREAM(color, osg::Vec4(r, g, b, a))

#define VERIFY_NODE_STR(node, str) \
  SG_CHECK_EQUAL(node.getStringValue(), std::string(str))

int main (int ac, char ** av)
{
  osg::Vec4 color;
  VERIFY_COLOR("#ff0000", 1,0,0,1);
  VERIFY_COLOR("#00ff00", 0,1,0,1);
  VERIFY_COLOR("#0000ff", 0,0,1,1);
  VERIFY_COLOR("rgb( 255,\t127.5,0)", 1, 0.5, 0, 1);
  VERIFY_COLOR("rgba(255,  127.5,0, 0.5)", 1, 0.5, 0, 0.5);

  SGPropertyNode color_node, color_arg;
  color_arg.setStringValue("#000000");

  auto interp = std::unique_ptr<simgear::ColorInterpolator>(new simgear::ColorInterpolator);
  interp->reset(color_arg);

  interp->update(color_node, 0.5); // with no color it should immediately set to the target
  VERIFY_NODE_STR(color_node, "rgb(0,0,0)");

  color_arg.setStringValue("rgba(255,0,0,0.5)");
  interp->reset(color_arg);

  interp->update(color_node, 0.5);
  VERIFY_NODE_STR(color_node, "rgba(127,0,0,0.75)");

  interp->update(color_node, 0.5);
  VERIFY_NODE_STR(color_node, "rgba(255,0,0,0.5)");

  // Animation has already completed and therefore should be reset and start a
  // new animation starting with the current value of the animation. As this
  // is already the same as the target value, nothing should change.
  interp->update(color_node, 0.5);
  VERIFY_NODE_STR(color_node, "rgba(255,0,0,0.5)");

  color_arg.setStringValue("#00ff00");
  interp->reset(color_arg);
  interp->update(color_node, 1.0);
  VERIFY_NODE_STR(color_node, "rgb(0,255,0)");

  std::cout << "all tests passed successfully!" << std::endl;
  return 0;
}
