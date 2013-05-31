#ifndef CANVAS_PATH_HXX_
# error Canvas - do not include this file!
#endif

#define n BOOST_PP_ITERATION()

Path& addSegment( uint8_t cmd
                  BOOST_PP_COMMA_IF(n)
                  BOOST_PP_ENUM_PARAMS(n, float coord) )
{
  _node->addChild("cmd")->setIntValue(cmd);

#define SG_CANVAS_PATH_SET_COORD(z, n, dummy)\
  _node->addChild("coord")->setFloatValue(coord##n);

  BOOST_PP_REPEAT(n, SG_CANVAS_PATH_SET_COORD, 0)
#undef SG_CANVAS_PATH_SET_COORD
  return *this;
}

#undef n
