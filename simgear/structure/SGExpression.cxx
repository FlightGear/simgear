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
#include "Singleton.hxx"

#include <algorithm>
#include <map>
#include <utility>
#include <string>
#include <sstream>
#include <cstring> // for strcmp
#include <cassert>

#include <simgear/props/props.hxx>

using namespace std;

namespace simgear
{
template<typename T>
const expression::Value evalValue(const Expression* exp,
                                  const expression::Binding* b)
{
    T val;
    static_cast<const SGExpression<T>*>(exp)->eval(val, b);
    return expression::Value(val);
}

const expression::Value eval(const Expression* exp,
                             const expression::Binding* b)
{
    using namespace expression;
    switch (exp->getType()) {
    case BOOL:
        return evalValue<bool>(exp, b);
    case INT:
        return evalValue<int>(exp, b);
    case FLOAT:
        return evalValue<float>(exp, b);
    case DOUBLE:
        return evalValue<double>(exp, b);
    default:
        throw "invalid type.";
    }
}
}

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
bool
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
        value = true;   // TODO: Logic error.  Leaving in place until stability issues are resolved.
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
SGReadExpression(SGPropertyNode *inputRoot, const SGPropertyNode *expression);

template<typename T>
static bool
SGReadNaryOperands(SGNaryExpression<T>* nary,
                   SGPropertyNode *inputRoot, const SGPropertyNode *expression)
{
    for (int i = 0; i < expression->nChildren(); ++i) {
        SGExpression<T>* inputExpression;
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(i));
        if (!inputExpression)
            return false;
        nary->addOperand(inputExpression);
    }
    return true;
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
SGReadExpression(SGPropertyNode *inputRoot, const SGPropertyNode *expression)
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        for (int i = 0; !inputExpression && i < expression->nChildren(); ++i)
            inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(i));
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
            SGReadExpression<T>(inputRoot, expression->getChild(0)),
            SGReadExpression<T>(inputRoot, expression->getChild(1))
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
            SGReadExpression<T>(inputRoot, expression->getChild(0)),
            SGReadExpression<T>(inputRoot, expression->getChild(1))
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

    if (name == "difference" || name == "dif" ) {
        if (expression->nChildren() < 1) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
            return 0;
        }
        SGDifferenceExpression<T>* output = new SGDifferenceExpression<T>;
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

    if (name == "table") {
        SGInterpTable* tab = new SGInterpTable(expression);
        if (!tab) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression: malformed table");
            return 0;
        }
        
        // find input expression - i.e a child not named 'entry'
        const SGPropertyNode* inputNode = NULL;
        for (int i=0; (i<expression->nChildren()) && !inputNode; ++i) {
            if (strcmp(expression->getChild(i)->getName(), "entry") == 0) {
                continue;
            }
            
            inputNode = expression->getChild(i);
        }
        
        if (!inputNode) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression: no input found");
            return 0;
        }

        SGSharedPtr<SGExpression<T> > inputExpression;
        inputExpression = SGReadExpression<T>(inputRoot, inputNode);
        if (!inputExpression) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
            return 0;
        }
        
        return new SGInterpTableExpression<T>(inputExpression, tab);
    }
    
    if (name == "acos") {
        if (expression->nChildren() != 1) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
            return 0;
        }
        SGSharedPtr<SGExpression<T> > inputExpression;
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
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
        inputExpression = SGReadExpression<T>(inputRoot, expression->getChild(0));
        if (!inputExpression) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot read \"" << name << "\" expression.");
            return 0;
        }
        return new SGTanhExpression<T>(inputExpression);
    }
    
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
            SGReadExpression<T>(inputRoot, expression->getChild(0)),
            SGReadExpression<T>(inputRoot, expression->getChild(1))
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
            SGReadExpression<T>(inputRoot, expression->getChild(0)),
            SGReadExpression<T>(inputRoot, expression->getChild(1))
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
            SGReadExpression<T>(inputRoot, expression->getChild(0)),
            SGReadExpression<T>(inputRoot, expression->getChild(1))
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
            SGReadExpression<T>(inputRoot, expression->getChild(0)),
            SGReadExpression<T>(inputRoot, expression->getChild(1))
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
{ return SGReadExpression<int>(inputRoot, configNode); }

SGExpression<float>*
SGReadFloatExpression(SGPropertyNode *inputRoot,
                      const SGPropertyNode *configNode)
{ return SGReadExpression<float>(inputRoot, configNode); }

SGExpression<double>*
SGReadDoubleExpression(SGPropertyNode *inputRoot,
                       const SGPropertyNode *configNode)
{ return SGReadExpression<double>(inputRoot, configNode); }

// SGExpression<bool>*
// SGReadBoolExpression(SGPropertyNode *inputRoot,
//                      const SGPropertyNode *configNode)
// { return SGReadBExpression<bool>(inputRoot, configNode); }

namespace simgear
{
namespace expression
{

bool Parser::readChildren(const SGPropertyNode* exp,
                          vector<Expression*>& result)
{
    for (int i = 0; i < exp->nChildren(); ++i)
        result.push_back(read(exp->getChild(i)));
    return true;
}

void ExpressionParser::addExpParser(const string& token, exp_parser parsefn)
{
    ParserMapSingleton::instance()
        ->_parserTable.insert(std::make_pair(token, parsefn));
}

Expression* valueParser(const SGPropertyNode* exp, Parser* parser)
{
    switch (exp->getType()) {
    case props::BOOL:
        return new SGConstExpression<bool>(getValue<bool>(exp));        
    case props::INT:
        return new SGConstExpression<int>(getValue<int>(exp));        
    case props::FLOAT:
        return new SGConstExpression<float>(getValue<float>(exp));
    case props::DOUBLE:
        return new SGConstExpression<double>(getValue<double>(exp));
    default:
        return 0;
    }
}

ExpParserRegistrar valueRegistrar("value", valueParser);

template<typename T, typename OpType>
inline Expression* makeConvert(Expression* e)
{
    return new ConvertExpression<T,
        OpType>(static_cast<SGExpression<OpType>*>(e));
}

Type promoteAndConvert(vector<Expression*>& exps, Type minType = BOOL)
{
    vector<Expression*>::iterator maxElem
        = max_element(exps.begin(), exps.end());
    Type maxType = (*maxElem)->getType();
    Type resultType = minType < maxType ? maxType : minType;
    for (vector<Expression*>::iterator itr = exps.begin(), end = exps.end();
         itr != end;
         ++itr) {
        if ((*itr)->getType() != resultType) {
            switch ((*itr)->getType()) {
            case BOOL:
                switch (resultType) {
                case INT:
                    *itr = makeConvert<int, bool>(*itr);
                    break;
                case FLOAT:
                    *itr = makeConvert<float, bool>(*itr);
                    break;
                case DOUBLE:
                    *itr = makeConvert<double, bool>(*itr);
                    break;
                default:
                    break;
                }
                break;
            case INT:
                switch (resultType) {
                case FLOAT:
                    *itr = makeConvert<float, int>(*itr);
                    break;
                case DOUBLE:
                    *itr = makeConvert<double, int>(*itr);
                    break;
                default:
                    break;
                }
                break;
            case FLOAT:
                *itr = makeConvert<double, float>(*itr);
                break;
            default:
                break;
            }
        }
    }
    return resultType;
}

template<template<typename OpType> class Expr>
Expression* makeTypedOperandExp(Type operandType, vector<Expression*> children)
{
    switch (operandType) {
    case BOOL:
    {
        Expr<bool> *expr = new Expr<bool>();
        expr->addOperands(children.begin(), children.end());
        return expr;
    }
    case INT:
    {
        Expr<int> *expr = new Expr<int>();
        expr->addOperands(children.begin(), children.end());
        return expr;
    }
    case FLOAT:
    {
        Expr<float> *expr = new Expr<float>();
        expr->addOperands(children.begin(), children.end());
        return expr;
    }
    case DOUBLE:
    {
        Expr<double> *expr = new Expr<double>();
        expr->addOperands(children.begin(), children.end());
        return expr;
    }
    default:
        return 0;
    }
}

template<template<typename OpType> class PredExp>
Expression* predParser(const SGPropertyNode* exp, Parser* parser)
{
    vector<Expression*> children;
    parser->readChildren(exp, children);
    Type operandType = promoteAndConvert(children);
    return makeTypedOperandExp<PredExp>(operandType, children);
}

ExpParserRegistrar equalRegistrar("equal", predParser<EqualToExpression>);
ExpParserRegistrar lessRegistrar("less", predParser<LessExpression>);
ExpParserRegistrar leRegistrar("less-equal", predParser<LessEqualExpression>);

template<typename Logicop>
Expression* logicopParser(const SGPropertyNode* exp, Parser* parser)
{
    std::vector<Expression*> children;
    parser->readChildren(exp, children);
    std::vector<Expression*>::const_iterator notBool =
        std::find_if(children.begin(), children.end(),
                     [] (const Expression* const e) {
                         return e->getType () != BOOL; });
    if (notBool != children.end())
        throw("non boolean operand to logical expression");
    Logicop *expr = new Logicop;
    expr->addOperands(children.begin(), children.end());
    return expr;
}

ExpParserRegistrar andRegistrar("and", logicopParser<AndExpression>);
ExpParserRegistrar orRegistrar("or", logicopParser<OrExpression>);

size_t BindingLayout::addBinding(const string& name, Type type)
{
    //XXX error checkint
    std::vector<VariableBinding>::const_iterator itr =
        std::find_if(bindings.begin(), bindings.end(),
                     [&name] (const VariableBinding& v) {
                         return v.name == name; });
    if (itr != bindings.end())
        return itr->location;
    size_t result = bindings.size();
    bindings.push_back(VariableBinding(name, type, bindings.size()));
    return result;
}

bool BindingLayout::findBinding(const std::string& name,
                                VariableBinding& result) const
{
    std::vector<VariableBinding>::const_iterator itr =
        std::find_if(bindings.begin(), bindings.end(),
                     [&name] (const VariableBinding& v) {
                         return v.name == name; });
    if (itr != bindings.end()) {
        result = *itr;
        return true;
    } else {
        return false;
    }
}
}
}
