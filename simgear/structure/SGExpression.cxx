/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGExpression.hxx"

#include <string>
#include <sstream>
#include <simgear/props/props.hxx>

template<typename T>
static bool
SGReadValueFromString(const char* str, T& value)
{
  if (!str) {
    SG_LOG(SG_IO, SG_ALERT, "Cannot read string content.");
    return false;
  }
  std::stringstream s;
  s.str(std::string(str));
  s >> value;
  if (s.fail()) {
    SG_LOG(SG_IO, SG_ALERT, "Cannot read string content.");
    return false;
  }
  return true;
}

template<>
static bool
SGReadValueFromString(const char* str, bool& value)
{
  if (!str) {
    SG_LOG(SG_IO, SG_ALERT, "Cannot read string content.");
    return false;
  }
  std::stringstream s;
  s.str(std::string(str));
  s >> value;
  if (!s.fail())
    return true;

  std::string stdstr;
  if (!SGReadValueFromString(str, stdstr))
    return false;

  if (stdstr == "true" || stdstr == "True" || stdstr == "TRUE") {
    value = true;
    return true;
  }
  if (stdstr == "false" || stdstr == "False" || stdstr == "FALSE") {
    value = true;
    return true;
  }
  return false;
}

template<typename T>
static bool
SGReadValueFromContent(const SGPropertyNode *node, T& value)
{
  if (!node)
    return false;
  return SGReadValueFromString(node->getStringValue(), value);
}

template<typename T>
static SGExpression<T>*
SGReadIExpression(SGPropertyNode *inputRoot, const SGPropertyNode *expression);

template<typename T>
static bool
SGReadNaryOperands(SGNaryExpression<T>* nary,
                   SGPropertyNode *inputRoot, const SGPropertyNode *expression)
{
  for (unsigned i = 0; i < expression->nChildren(); ++i) {
    SGExpression<T>* inputExpression;
    inputExpression = SGReadIExpression<T>(inputRoot, expression->getChild(i));
    if (!inputExpression)
      return false;
    nary->addOperand(inputExpression);
  }
}

// template<typename T>
// static SGExpression<T>*
// SGReadBExpression(SGPropertyNode *inputRoot, const SGPropertyNode *expression)
// {
//   if (!expression)
//     return 0;

//   std::string name = expression->getName();
//   if (name == "value") {
//     T value;
//     if (!SGReadValueFromContent(expression, value)) {
//       SG_LOG(SG_IO, SG_ALERT, "Cannot read \"value\" expression.");
//       return 0;
//     }
//     return new SGConstExpression<T>(value);
//   }

//   return 0;
// }

template<typename T>
static SGExpression<T>*
SGReadIExpression(SGPropertyNode *inputRoot, const SGPropertyNode *expression)
{
  if (!expression)
    return 0;

  std::string name = expression->getName();
  if (name == "value") {
    T value;
    if (!SGReadValueFromContent(expression, value)) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"value\" expression.");
      return 0;
    }
    return new SGConstExpression<T>(value);
  }

  if (name == "property") {
    if (!inputRoot) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.\n"
             "No inputRoot argument given!");
      return 0;
    }
    if (!expression->getStringValue()) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGPropertyNode* inputNode;
    inputNode = inputRoot->getNode(expression->getStringValue(), true);
    return new SGPropertyExpression<T>(inputNode);
  }

  if (name == "abs" || name == "fabs") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadIExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGAbsExpression<T>(inputExpression);
  }

  if (name == "sqr") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadIExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGSqrExpression<T>(inputExpression);
  }

  if (name == "clip") {
    if (expression->nChildren() != 3) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    const SGPropertyNode* minProperty = expression->getChild("clipMin");
    T clipMin;
    if (!SGReadValueFromContent(minProperty, clipMin))
      clipMin = SGMisc<T>::min(SGLimits<T>::min(), -SGLimits<T>::max());

    const SGPropertyNode* maxProperty = expression->getChild("clipMax");
    T clipMax;
    if (!SGReadValueFromContent(maxProperty, clipMax))
      clipMin = SGLimits<T>::max();

    SGSharedPtr<SGExpression<T> > inputExpression;
    for (unsigned i = 0; !inputExpression && i < expression->nChildren(); ++i)
      inputExpression = SGReadIExpression<T>(inputRoot, expression->getChild(i));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGClipExpression<T>(inputExpression, clipMin, clipMax);
  }

  if (name == "div") {
    if (expression->nChildren() != 2) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpressions[2] = {
      SGReadIExpression<T>(inputRoot, expression->getChild(0)),
      SGReadIExpression<T>(inputRoot, expression->getChild(1))
    };
    if (!inputExpressions[0] || !inputExpressions[1]) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGDivExpression<T>(inputExpressions[0], inputExpressions[1]);
  }
  if (name == "mod") {
    if (expression->nChildren() != 2) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpressions[2] = {
      SGReadIExpression<T>(inputRoot, expression->getChild(0)),
      SGReadIExpression<T>(inputRoot, expression->getChild(1))
    };
    if (!inputExpressions[0] || !inputExpressions[1]) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGModExpression<T>(inputExpressions[0], inputExpressions[1]);
  }

  if (name == "sum") {
    if (expression->nChildren() < 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSumExpression<T>* output = new SGSumExpression<T>;
    if (!SGReadNaryOperands(output, inputRoot, expression)) {
      delete output;
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return output;
  }
  if (name == "prod" || name == "product") {
    if (expression->nChildren() < 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGProductExpression<T>* output = new SGProductExpression<T>;
    if (!SGReadNaryOperands(output, inputRoot, expression)) {
      delete output;
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return output;
  }
  if (name == "min") {
    if (expression->nChildren() < 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGMinExpression<T>* output = new SGMinExpression<T>;
    if (!SGReadNaryOperands(output, inputRoot, expression)) {
      delete output;
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return output;
  }
  if (name == "max") {
    if (expression->nChildren() < 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGMaxExpression<T>* output = new SGMaxExpression<T>;
    if (!SGReadNaryOperands(output, inputRoot, expression)) {
      delete output;
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return output;
  }

  return 0;
}


template<typename T>
static SGExpression<T>*
SGReadFExpression(SGPropertyNode *inputRoot, const SGPropertyNode *expression)
{
  SGExpression<T>* r = SGReadIExpression<T>(inputRoot, expression);
  if (r)
    return r;

  if (!expression)
    return 0;

  std::string name = expression->getName();
  if (name == "acos") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGACosExpression<T>(inputExpression);
  }

  if (name == "asin") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGASinExpression<T>(inputExpression);
  }

  if (name == "atan") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGATanExpression<T>(inputExpression);
  }

  if (name == "ceil") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGCeilExpression<T>(inputExpression);
  }

  if (name == "cos") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGCosExpression<T>(inputExpression);
  }

  if (name == "cosh") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGCoshExpression<T>(inputExpression);
  }

  if (name == "deg2rad") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGScaleExpression<T>(inputExpression, SGMisc<T>::pi()/180);
  }

  if (name == "exp") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGExpExpression<T>(inputExpression);
  }

  if (name == "floor") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGFloorExpression<T>(inputExpression);
  }

  if (name == "log") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGLogExpression<T>(inputExpression);
  }

  if (name == "log10") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGLog10Expression<T>(inputExpression);
  }

  if (name == "rad2deg") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGScaleExpression<T>(inputExpression, 180/SGMisc<T>::pi());
  }

  if (name == "sin") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGSinExpression<T>(inputExpression);
  }

  if (name == "sinh") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGSinhExpression<T>(inputExpression);
  }

  if (name == "sqrt") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGSqrtExpression<T>(inputExpression);
  }

  if (name == "tan") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGTanExpression<T>(inputExpression);
  }

  if (name == "tanh") {
    if (expression->nChildren() != 1) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpression;
    inputExpression = SGReadFExpression<T>(inputRoot, expression->getChild(0));
    if (!inputExpression) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGTanhExpression<T>(inputExpression);
  }

// if (name == "table") {
// }
// if (name == "step") {
// }
// if (name == "condition") {
// }

  if (name == "atan2") {
    if (expression->nChildren() != 2) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpressions[2] = {
      SGReadFExpression<T>(inputRoot, expression->getChild(0)),
      SGReadFExpression<T>(inputRoot, expression->getChild(1))
    };
    if (!inputExpressions[0] || !inputExpressions[1]) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGAtan2Expression<T>(inputExpressions[0], inputExpressions[1]);
  }
  if (name == "div") {
    if (expression->nChildren() != 2) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpressions[2] = {
      SGReadFExpression<T>(inputRoot, expression->getChild(0)),
      SGReadFExpression<T>(inputRoot, expression->getChild(1))
    };
    if (!inputExpressions[0] || !inputExpressions[1]) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGDivExpression<T>(inputExpressions[0], inputExpressions[1]);
  }
  if (name == "mod") {
    if (expression->nChildren() != 2) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpressions[2] = {
      SGReadFExpression<T>(inputRoot, expression->getChild(0)),
      SGReadFExpression<T>(inputRoot, expression->getChild(1))
    };
    if (!inputExpressions[0] || !inputExpressions[1]) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGModExpression<T>(inputExpressions[0], inputExpressions[1]);
  }
  if (name == "pow") {
    if (expression->nChildren() != 2) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    SGSharedPtr<SGExpression<T> > inputExpressions[2] = {
      SGReadIExpression<T>(inputRoot, expression->getChild(0)),
      SGReadIExpression<T>(inputRoot, expression->getChild(1))
    };
    if (!inputExpressions[0] || !inputExpressions[1]) {
      SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
      return 0;
    }
    return new SGPowExpression<T>(inputExpressions[0], inputExpressions[1]);
  }

  return 0;
}

SGExpression<int>*
SGReadIntExpression(SGPropertyNode *inputRoot,
                    const SGPropertyNode *configNode)
{ return SGReadIExpression<int>(inputRoot, configNode); }

SGExpression<float>*
SGReadFloatExpression(SGPropertyNode *inputRoot,
                      const SGPropertyNode *configNode)
{ return SGReadFExpression<float>(inputRoot, configNode); }

SGExpression<double>*
SGReadDoubleExpression(SGPropertyNode *inputRoot,
                       const SGPropertyNode *configNode)
{ return SGReadFExpression<double>(inputRoot, configNode); }

// SGExpression<bool>*
// SGReadBoolExpression(SGPropertyNode *inputRoot,
//                      const SGPropertyNode *configNode)
// { return SGReadBExpression<bool>(inputRoot, configNode); }
