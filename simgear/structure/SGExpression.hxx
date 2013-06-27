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

#ifndef _SG_EXPRESSION_HXX
#define _SG_EXPRESSION_HXX 1

#include <string>
#include <vector>
#include <functional>
#include <set>
#include <string>

#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/scene/model/persparam.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/Singleton.hxx>

/// Expression tree implementation.

namespace simgear
{
  namespace expression
  {
    enum Type {
      BOOL = 0,
      INT,
      FLOAT,
      DOUBLE
    };
    template<typename T> struct TypeTraits;
    template<> struct TypeTraits<bool> {
      static const Type typeTag = BOOL;
    };
    template<> struct TypeTraits<int> {
      static const Type typeTag = INT;
    };
    template<> struct TypeTraits<float> {
      static const Type typeTag = FLOAT;
    };
    template<> struct TypeTraits<double> {
      static const Type typeTag = DOUBLE;
    };

    struct Value
    {
      Type typeTag;
      union {
        bool boolVal;
        int intVal;
        float floatVal;
        double doubleVal;
      } val;

      Value() : typeTag(DOUBLE)
      {
        val.doubleVal = 0.0;
      }

      Value(bool val_) : typeTag(BOOL)
      {
        val.boolVal = val_;
      }

      Value(int val_) : typeTag(INT)
      {
        val.intVal = val_;
      }

      Value(float val_) : typeTag(FLOAT)
      {
        val.floatVal = val_;
      }

      Value(double val_) : typeTag(DOUBLE)
      {
        val.doubleVal = val_;
      }

    };

    class Binding;
  }

  class Expression : public SGReferenced
  {
  public:
    virtual ~Expression() {}
    virtual expression::Type getType() const = 0;
  };

  const expression::Value eval(const Expression* exp,
                               const expression::Binding* binding = 0);

}

template<typename T>
class SGExpression : public simgear::Expression {
public:
  virtual ~SGExpression() {}
  typedef T result_type;
  typedef T operand_type;
  virtual void eval(T&, const simgear::expression::Binding*) const = 0;

  T getValue(const simgear::expression::Binding* binding = 0) const
  { T value; eval(value, binding); return value; }

  double getDoubleValue(const simgear::expression::Binding* binding = 0) const
  { T value; eval(value, binding); return value; }

  virtual bool isConst() const { return false; }
  virtual SGExpression* simplify();
  virtual simgear::expression::Type getType() const
  {
    return simgear::expression::TypeTraits<T>::typeTag;
  }
  virtual simgear::expression::Type getOperandType() const
  {
    return simgear::expression::TypeTraits<T>::typeTag;
  }
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
  { }
};

/// Constant value expression
template<typename T>
class SGConstExpression : public SGExpression<T> {
public:
  SGConstExpression(const T& value = T()) : _value(value)
  { }
  void setValue(const T& value)
  { _value = value; }
  const T& getValue(const simgear::expression::Binding* binding = 0) const
  { return _value; }
  virtual void eval(T& value, const simgear::expression::Binding*) const
  { value = _value; }
  virtual bool isConst() const { return true; }
private:
  T _value;
};

template<typename T>
SGExpression<T>*
SGExpression<T>::simplify()
{
  if (isConst())
    return new SGConstExpression<T>(getValue());
  return this;
}

template<typename T>
class SGUnaryExpression : public SGExpression<T> {
public:
  const SGExpression<T>* getOperand() const
  { return _expression; }
  SGExpression<T>* getOperand()
  { return _expression; }
  void setOperand(SGExpression<T>* expression)
  {
    if (!expression)
      expression = new SGConstExpression<T>(T());
    _expression = expression;
  }
  virtual bool isConst() const
  { return getOperand()->isConst(); }
  virtual SGExpression<T>* simplify()
  {
    _expression = _expression->simplify();
    return SGExpression<T>::simplify();
  }
  
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
    { _expression->collectDependentProperties(props); }  
protected:
  SGUnaryExpression(SGExpression<T>* expression = 0)
  { setOperand(expression); }

private:
  SGSharedPtr<SGExpression<T> > _expression;
};

template<typename T>
class SGBinaryExpression : public SGExpression<T> {
public:
  const SGExpression<T>* getOperand(size_t i) const
  { return _expressions[i]; }
  SGExpression<T>* getOperand(size_t i)
  { return _expressions[i]; }
  void setOperand(size_t i, SGExpression<T>* expression)
  {
    if (!expression)
      expression = new SGConstExpression<T>(T());
    if (2 <= i)
      i = 0;
    _expressions[i] = expression;
  }

  virtual bool isConst() const
  { return getOperand(0)->isConst() && getOperand(1)->isConst(); }
  virtual SGExpression<T>* simplify()
  {
    _expressions[0] = _expressions[0]->simplify();
    _expressions[1] = _expressions[1]->simplify();
    return SGExpression<T>::simplify();
  }

  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
  {
    _expressions[0]->collectDependentProperties(props);
    _expressions[1]->collectDependentProperties(props); 
  } 
  
protected:
  SGBinaryExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
  { setOperand(0, expr0); setOperand(1, expr1); }
  
private:
  SGSharedPtr<SGExpression<T> > _expressions[2];
};

template<typename T>
class SGNaryExpression : public SGExpression<T> {
public:
  size_t getNumOperands() const
  { return _expressions.size(); }
  const SGExpression<T>* getOperand(size_t i) const
  { return _expressions[i]; }
  SGExpression<T>* getOperand(size_t i)
  { return _expressions[i]; }
  size_t addOperand(SGExpression<T>* expression)
  {
    if (!expression)
      return ~size_t(0);
    _expressions.push_back(expression);
    return _expressions.size() - 1;
  }

  template<typename Iter>
  void addOperands(Iter begin, Iter end)
  {
    for (Iter iter = begin; iter != end; ++iter)
      {
        addOperand(static_cast< ::SGExpression<T>*>(*iter));
      }
  }

  virtual bool isConst() const
  {
    for (size_t i = 0; i < _expressions.size(); ++i)
      if (!_expressions[i]->isConst())
        return false;
    return true;
  }
  virtual SGExpression<T>* simplify()
  {
    for (size_t i = 0; i < _expressions.size(); ++i)
      _expressions[i] = _expressions[i]->simplify();
    return SGExpression<T>::simplify();
  }

  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
  {
    for (size_t i = 0; i < _expressions.size(); ++i)
      _expressions[i]->collectDependentProperties(props);
  } 
protected:
  SGNaryExpression()
  { }
  SGNaryExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
  { addOperand(expr0); addOperand(expr1); }
  
private:
  std::vector<SGSharedPtr<SGExpression<T> > > _expressions;
};




template<typename T>
class SGPropertyExpression : public SGExpression<T> {
public:
  SGPropertyExpression(const SGPropertyNode* prop) : _prop(prop)
  { }
  void setPropertyNode(const SGPropertyNode* prop)
  { _prop = prop; }
  virtual void eval(T& value, const simgear::expression::Binding*) const
  { doEval(value); }
  
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
    { props.insert(_prop.get()); }
private:
  void doEval(float& value) const
  { if (_prop) value = _prop->getFloatValue(); }
  void doEval(double& value) const
  { if (_prop) value = _prop->getDoubleValue(); }
  void doEval(int& value) const
  { if (_prop) value = _prop->getIntValue(); }
  void doEval(long& value) const
  { if (_prop) value = _prop->getLongValue(); }
  void doEval(bool& value) const
  { if (_prop) value = _prop->getBoolValue(); }
  SGSharedPtr<const SGPropertyNode> _prop;
};

template<typename T>
class SGAbsExpression : public SGUnaryExpression<T> {
public:
  SGAbsExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = getOperand()->getValue(b); if (value <= 0) value = -value; }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGACosExpression : public SGUnaryExpression<T> {
public:
  SGACosExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = acos((double)SGMisc<T>::clip(getOperand()->getValue(b), -1, 1)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGASinExpression : public SGUnaryExpression<T> {
public:
  SGASinExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = asin((double)SGMisc<T>::clip(getOperand()->getValue(b), -1, 1)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGATanExpression : public SGUnaryExpression<T> {
public:
  SGATanExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = atan(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGCeilExpression : public SGUnaryExpression<T> {
public:
  SGCeilExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = ceil(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGCosExpression : public SGUnaryExpression<T> {
public:
  SGCosExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = cos(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGCoshExpression : public SGUnaryExpression<T> {
public:
  SGCoshExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = cosh(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGExpExpression : public SGUnaryExpression<T> {
public:
  SGExpExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = exp(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGFloorExpression : public SGUnaryExpression<T> {
public:
  SGFloorExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = floor(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGLogExpression : public SGUnaryExpression<T> {
public:
  SGLogExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = log(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGLog10Expression : public SGUnaryExpression<T> {
public:
  SGLog10Expression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = log10(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSinExpression : public SGUnaryExpression<T> {
public:
  SGSinExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = sin(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSinhExpression : public SGUnaryExpression<T> {
public:
  SGSinhExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = sinh(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSqrExpression : public SGUnaryExpression<T> {
public:
  SGSqrExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = getOperand()->getValue(b); value = value*value; }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSqrtExpression : public SGUnaryExpression<T> {
public:
  SGSqrtExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = sqrt(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGTanExpression : public SGUnaryExpression<T> {
public:
  SGTanExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = tan(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGTanhExpression : public SGUnaryExpression<T> {
public:
  SGTanhExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = tanh(getOperand()->getDoubleValue(b)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGScaleExpression : public SGUnaryExpression<T> {
public:
  SGScaleExpression(SGExpression<T>* expr = 0, const T& scale = T(1))
    : SGUnaryExpression<T>(expr), _scale(scale)
  { }
  void setScale(const T& scale)
  { _scale = scale; }
  const T& getScale() const
  { return _scale; }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = _scale * getOperand()->getValue(b); }

  virtual SGExpression<T>* simplify()
  {
    if (_scale == 1)
      return getOperand()->simplify();
    return SGUnaryExpression<T>::simplify();
  }

  using SGUnaryExpression<T>::getOperand;
private:
  T _scale;
};

template<typename T>
class SGBiasExpression : public SGUnaryExpression<T> {
public:
  SGBiasExpression(SGExpression<T>* expr = 0, const T& bias = T(0))
    : SGUnaryExpression<T>(expr), _bias(bias)
  { }

  void setBias(const T& bias)
  { _bias = bias; }
  const T& getBias() const
  { return _bias; }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = _bias + getOperand()->getValue(b); }

  virtual SGExpression<T>* simplify()
  {
    if (_bias == 0)
      return getOperand()->simplify();
    return SGUnaryExpression<T>::simplify();
  }

  using SGUnaryExpression<T>::getOperand;
private:
  T _bias;
};

template<typename T>
class SGInterpTableExpression : public SGUnaryExpression<T> {
public:
  SGInterpTableExpression(SGExpression<T>* expr,
                          const SGInterpTable* interpTable) :
    SGUnaryExpression<T>(expr),
    _interpTable(interpTable)
  { }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    if (_interpTable)
      value = _interpTable->interpolate(getOperand()->getValue(b));
  }

  using SGUnaryExpression<T>::getOperand;
private:
  SGSharedPtr<SGInterpTable const> _interpTable;
};

template<typename T>
class SGClipExpression : public SGUnaryExpression<T> {
public:
  SGClipExpression(SGExpression<T>* expr)
    : SGUnaryExpression<T>(expr),
      _clipMin(SGMisc<T>::min(-SGLimits<T>::max(), SGLimits<T>::min())),
      _clipMax(SGLimits<T>::max())
  { }
  SGClipExpression(SGExpression<T>* expr,
                   const T& clipMin, const T& clipMax)
    : SGUnaryExpression<T>(expr),
      _clipMin(clipMin),
      _clipMax(clipMax)
  { }

  void setClipMin(const T& clipMin)
  { _clipMin = clipMin; }
  const T& getClipMin() const
  { return _clipMin; }

  void setClipMax(const T& clipMax)
  { _clipMax = clipMax; }
  const T& getClipMax() const
  { return _clipMax; }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    value = SGMisc<T>::clip(getOperand()->getValue(b), _clipMin, _clipMax);
  }

  virtual SGExpression<T>* simplify()
  {
    if (_clipMin <= SGMisc<T>::min(-SGLimits<T>::max(), SGLimits<T>::min()) &&
        _clipMax >= SGLimits<T>::max())
      return getOperand()->simplify();
    return SGUnaryExpression<T>::simplify();
  }

  using SGUnaryExpression<T>::getOperand;
private:
  T _clipMin;
  T _clipMax;
};

template<typename T>
class SGStepExpression : public SGUnaryExpression<T> {
public:
  SGStepExpression(SGExpression<T>* expr = 0,
                   const T& step = T(1), const T& scroll = T(0))
    : SGUnaryExpression<T>(expr), _step(step), _scroll(scroll)
  { }

  void setStep(const T& step)
  { _step = step; }
  const T& getStep() const
  { return _step; }

  void setScroll(const T& scroll)
  { _scroll = scroll; }
  const T& getScroll() const
  { return _scroll; }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = apply_mods(getOperand()->getValue(b)); }

  using SGUnaryExpression<T>::getOperand;

private:
  T apply_mods(T property) const
  {
    if( _step <= SGLimits<T>::min() ) return property;

    // apply stepping of input value
    T modprop = floor(property/_step)*_step;

    // calculate scroll amount (for odometer like movement)
    T remainder = property <= SGLimits<T>::min() ? -fmod(property,_step) : (_step - fmod(property,_step));
    if( remainder > SGLimits<T>::min() && remainder < _scroll )
      modprop += (_scroll - remainder) / _scroll * _step;

    return modprop;
  }

  T _step;
  T _scroll;
};

template<typename T>
class SGEnableExpression : public SGUnaryExpression<T> {
public:
  SGEnableExpression(SGExpression<T>* expr = 0,
                     SGCondition* enable = 0,
                     const T& disabledValue = T(0))
    : SGUnaryExpression<T>(expr),
      _enable(enable),
      _disabledValue(disabledValue)
  { }

  const T& getDisabledValue() const
  { return _disabledValue; }
  void setDisabledValue(const T& disabledValue)
  { _disabledValue = disabledValue; }

  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    if (_enable->test())
      value = getOperand()->getValue(b);
    else
      value = _disabledValue;
  }

  virtual SGExpression<T>* simplify()
  {
    if (!_enable)
      return getOperand()->simplify();
    return SGUnaryExpression<T>::simplify();
  }
  
  virtual void collectDependentProperties(std::set<const SGPropertyNode*>& props) const
  {
    SGUnaryExpression<T>::collectDependentProperties(props);
    _enable->collectDependentProperties(props);
  }

  using SGUnaryExpression<T>::getOperand;
private:
  SGSharedPtr<SGCondition> _enable;
  T _disabledValue;
};

template<typename T>
class SGAtan2Expression : public SGBinaryExpression<T> {
public:
  SGAtan2Expression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGBinaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = atan2(getOperand(0)->getDoubleValue(b), getOperand(1)->getDoubleValue(b)); }
  using SGBinaryExpression<T>::getOperand;
};

template<typename T>
class SGDivExpression : public SGBinaryExpression<T> {
public:
  SGDivExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGBinaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = getOperand(0)->getValue(b) / getOperand(1)->getValue(b); }
  using SGBinaryExpression<T>::getOperand;
};

template<typename T>
class SGModExpression : public SGBinaryExpression<T> {
public:
  SGModExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGBinaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = mod(getOperand(0)->getValue(b), getOperand(1)->getValue(b)); }
  using SGBinaryExpression<T>::getOperand;
private:
  int mod(const int& v0, const int& v1) const
  { return v0 % v1; }
  float mod(const float& v0, const float& v1) const
  { return fmod(v0, v1); }
  double mod(const double& v0, const double& v1) const
  { return fmod(v0, v1); }
};

template<typename T>
class SGPowExpression : public SGBinaryExpression<T> {
public:
  SGPowExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGBinaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  { value = pow(getOperand(0)->getDoubleValue(b), getOperand(1)->getDoubleValue(b)); }
  using SGBinaryExpression<T>::getOperand;
};

template<typename T>
class SGSumExpression : public SGNaryExpression<T> {
public:
  SGSumExpression()
  { }
  SGSumExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGNaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    value = T(0);
    size_t sz = SGNaryExpression<T>::getNumOperands();
    for (size_t i = 0; i < sz; ++i)
      value += getOperand(i)->getValue(b);
  }
  using SGNaryExpression<T>::getValue;
  using SGNaryExpression<T>::getOperand;
};

template<typename T>
class SGDifferenceExpression : public SGNaryExpression<T> {
public:
  SGDifferenceExpression()
  { }
  SGDifferenceExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGNaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    value = getOperand(0)->getValue(b);
    size_t sz = SGNaryExpression<T>::getNumOperands();
    for (size_t i = 1; i < sz; ++i)
      value -= getOperand(i)->getValue(b);
  }
  using SGNaryExpression<T>::getValue;
  using SGNaryExpression<T>::getOperand;
};

template<typename T>
class SGProductExpression : public SGNaryExpression<T> {
public:
  SGProductExpression()
  { }
  SGProductExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGNaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    value = T(1);
    size_t sz = SGNaryExpression<T>::getNumOperands();
    for (size_t i = 0; i < sz; ++i)
      value *= getOperand(i)->getValue(b);
  }
  using SGNaryExpression<T>::getValue;
  using SGNaryExpression<T>::getOperand;
};

template<typename T>
class SGMinExpression : public SGNaryExpression<T> {
public:
  SGMinExpression()
  { }
  SGMinExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGNaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    size_t sz = SGNaryExpression<T>::getNumOperands();
    if (sz < 1)
      return;
    
    value = getOperand(0)->getValue(b);
    for (size_t i = 1; i < sz; ++i)
      value = SGMisc<T>::min(value, getOperand(i)->getValue(b));
  }
  using SGNaryExpression<T>::getOperand;
};

template<typename T>
class SGMaxExpression : public SGNaryExpression<T> {
public:
  SGMaxExpression()
  { }
  SGMaxExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGNaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value, const simgear::expression::Binding* b) const
  {
    size_t sz = SGNaryExpression<T>::getNumOperands();
    if (sz < 1)
      return;
    
    value = getOperand(0)->getValue(b);
    for (size_t i = 1; i < sz; ++i)
      value = SGMisc<T>::max(value, getOperand(i)->getValue(b));
  }
  using SGNaryExpression<T>::getOperand;
};

typedef SGExpression<int> SGExpressioni;
typedef SGExpression<float> SGExpressionf;
typedef SGExpression<double> SGExpressiond;
typedef SGExpression<bool> SGExpressionb;

typedef SGSharedPtr<SGExpressioni> SGExpressioni_ref;
typedef SGSharedPtr<SGExpressionf> SGExpressionf_ref;
typedef SGSharedPtr<SGExpressiond> SGExpressiond_ref;
typedef SGSharedPtr<SGExpressionb> SGExpressionb_ref;

/**
 * Global function to make an expression out of properties.

  <clip>
    <clipMin>0</clipMin>
    <clipMax>79</clipMax>
    <abs>
      <sum>
        <rad2deg>
          <property>sim/model/whatever-rad</property>
        </rad2deg>
        <property>sim/model/someother-deg</property>
        <value>-90</value>
      </sum>
    </abs>
  <clip>

will evaluate to an expression:

SGMisc<T>::clip(abs(deg2rad*sim/model/whatever-rad + sim/model/someother-deg - 90), clipMin, clipMax);

 */
SGExpression<int>*
SGReadIntExpression(SGPropertyNode *inputRoot,
                    const SGPropertyNode *configNode);

SGExpression<float>*
SGReadFloatExpression(SGPropertyNode *inputRoot,
                      const SGPropertyNode *configNode);

SGExpression<double>*
SGReadDoubleExpression(SGPropertyNode *inputRoot,
                       const SGPropertyNode *configNode);

SGExpression<bool>*
SGReadBoolExpression(SGPropertyNode *inputRoot,
                     const SGPropertyNode *configNode);

namespace simgear
{
  namespace expression
  {
  struct ParseError : public sg_exception
  {
      ParseError(const std::string& message = std::string())
          : sg_exception(message) {}
  };
    
  // Support for binding variables around an expression.
  class Binding
  {
  public:
      virtual ~Binding() {}
      const virtual Value* getBindings() const = 0;
      virtual Value* getBindings() = 0;
  };

  class VariableLengthBinding : public Binding
  {
  public:
      const Value* getBindings() const
      {
          if (_bindings.empty())
              return 0;
          else
              return &_bindings[0];
      }
      Value* getBindings()
      {
          if (_bindings.empty())
              return 0;
          else
              return &_bindings[0];
      }
      std::vector<Value> _bindings;
  };

  template<int Size> class FixedLengthBinding : public Binding
  {
  public:
      Value* getBindings()
      {
          return &_bindings[0];
      }
      const Value* getBindings() const
      {
          return &_bindings[0];
      }
      Value _bindings[Size];
  };

  struct VariableBinding
  {
      VariableBinding() : type(expression::DOUBLE), location(-1) {}

      VariableBinding(const std::string& name_, expression::Type type_,
                      int location_)
          : name(name_), type(type_), location(location_)
      {
      }
      std::string name;
      expression::Type type;
      int location;
  };

  class BindingLayout
  {
  public:
      size_t addBinding(const std::string& name, expression::Type type);
      bool findBinding(const std::string& name, VariableBinding& result) const;
      std::vector<VariableBinding> bindings;
  };

  class Parser {
  public:
      typedef Expression* (*exp_parser)(const SGPropertyNode* exp,
                                        Parser* parser);
      void addParser(const std::string& name, exp_parser parser)
      {
          getParserMap().insert(std::make_pair(name, parser));
      }
      Expression* read(const SGPropertyNode* exp)
      {
          ParserMap& map = getParserMap();
          ParserMap::iterator itr = map.find(exp->getName());
          if (itr == map.end())
              throw ParseError(std::string("unknown expression ") + exp->getName());
          exp_parser parser = itr->second;
          return (*parser)(exp, this);
      }
      // XXX vector of SGSharedPtr?
      bool readChildren(const SGPropertyNode* exp,
                        std::vector<Expression*>& result);
      /**
       * Function that parses a property tree, producing an expression.
       */
      typedef std::map<const std::string, exp_parser> ParserMap;
      virtual ParserMap& getParserMap() = 0;
      /**
       * After an expression is parsed, the binding layout may contain
       * references that need to be bound during evaluation.
       */
      BindingLayout& getBindingLayout() { return _bindingLayout; }
  protected:
      BindingLayout _bindingLayout;
  };

  class ExpressionParser : public Parser
  {
  public:
      ParserMap& getParserMap()
      {
          return ParserMapSingleton::instance()->_parserTable;
      }
      static void addExpParser(const std::string&, exp_parser);
  protected:
      struct ParserMapSingleton : public simgear::Singleton<ParserMapSingleton>
      {
          ParserMap _parserTable;
      };
  };

  /**
   * Constructor for registering parser functions.
   */
  struct ExpParserRegistrar
  {
      ExpParserRegistrar(const std::string& token, Parser::exp_parser parser)
      {
          ExpressionParser::addExpParser(token, parser);
      }
  };

  }

  /**
   * Access a variable definition. Use a location from a BindingLayout.
   */
  template<typename T>
  class VariableExpression : public ::SGExpression<T> {
  public:
    VariableExpression(int location) : _location(location) {}
    virtual ~VariableExpression() {}
    virtual void eval(T& value, const simgear::expression::Binding* b) const
    {
      const expression::Value* values = b->getBindings();
      value = *reinterpret_cast<const T *>(&values[_location].val);
    }
  protected:
    int _location;

  };

  /**
   * An n-ary expression where the types of the argument aren't the
   * same as the return type.
   */
  template<typename T, typename OpType>
  class GeneralNaryExpression : public ::SGExpression<T> {
  public:
    typedef OpType operand_type;
    size_t getNumOperands() const
    { return _expressions.size(); }
    const ::SGExpression<OpType>* getOperand(size_t i) const
    { return _expressions[i]; }
    ::SGExpression<OpType>* getOperand(size_t i)
    { return _expressions[i]; }
    size_t addOperand(::SGExpression<OpType>* expression)
    {
      if (!expression)
        return ~size_t(0);
      _expressions.push_back(expression);
      return _expressions.size() - 1;
    }
    
    template<typename Iter>
    void addOperands(Iter begin, Iter end)
    {
      for (Iter iter = begin; iter != end; ++iter)
        {
          addOperand(static_cast< ::SGExpression<OpType>*>(*iter));
        }
    }
    
    virtual bool isConst() const
    {
      for (size_t i = 0; i < _expressions.size(); ++i)
        if (!_expressions[i]->isConst())
          return false;
      return true;
    }
    virtual ::SGExpression<T>* simplify()
    {
      for (size_t i = 0; i < _expressions.size(); ++i)
        _expressions[i] = _expressions[i]->simplify();
      return SGExpression<T>::simplify();
    }

    simgear::expression::Type getOperandType() const
    {
      return simgear::expression::TypeTraits<OpType>::typeTag;
    }
    
  protected:
    GeneralNaryExpression()
    { }
    GeneralNaryExpression(::SGExpression<OpType>* expr0,
                          ::SGExpression<OpType>* expr1)
    { addOperand(expr0); addOperand(expr1); }

    std::vector<SGSharedPtr<SGExpression<OpType> > > _expressions;
  };

  /**
   * A predicate that wraps, for example the STL template predicate
   * expressions like std::equal_to.
   */
  template<typename OpType, template<typename PredOp> class Pred>
  class PredicateExpression : public GeneralNaryExpression<bool, OpType> {
  public:
    PredicateExpression()
    {
    }
    PredicateExpression(::SGExpression<OpType>* expr0,
                        ::SGExpression<OpType>* expr1)
      : GeneralNaryExpression<bool, OpType>(expr0, expr1)
    {
    }
    virtual void eval(bool& value, const simgear::expression::Binding* b) const
    {
      size_t sz = this->getNumOperands();
      if (sz != 2)
        return;
      value = _pred(this->getOperand(0)->getValue(b),
                    this->getOperand(1)->getValue(b));
    }
  protected:
    Pred<OpType> _pred;
  };

  template<template<typename OT> class Pred, typename OpType>
  PredicateExpression<OpType, Pred>*
  makePredicate(SGExpression<OpType>* op1, SGExpression<OpType>* op2)
  {
    return new PredicateExpression<OpType, Pred>(op1, op2);
  }
  
  template<typename OpType>
  class EqualToExpression : public PredicateExpression<OpType, std::equal_to>
  {
  public:
    EqualToExpression() {}
    EqualToExpression(::SGExpression<OpType>* expr0,
                      ::SGExpression<OpType>* expr1)
      : PredicateExpression<OpType, std::equal_to>(expr0, expr1)
    {
    }
  };

  template<typename OpType>
  class LessExpression : public PredicateExpression<OpType, std::less>
  {
  public:
    LessExpression() {}
    LessExpression(::SGExpression<OpType>* expr0, ::SGExpression<OpType>* expr1)
      : PredicateExpression<OpType, std::less>(expr0, expr1)
    {
    }
  };

  template<typename OpType>
  class LessEqualExpression
    : public PredicateExpression<OpType, std::less_equal>
  {
  public:
    LessEqualExpression() {}
    LessEqualExpression(::SGExpression<OpType>* expr0,
                        ::SGExpression<OpType>* expr1)
      : PredicateExpression<OpType, std::less_equal>(expr0, expr1)
    {
    }
  };

  class NotExpression : public ::SGUnaryExpression<bool>
  {
  public:
    NotExpression(::SGExpression<bool>* expr = 0)
      : ::SGUnaryExpression<bool>(expr)
    {
    }
    void eval(bool& value, const expression::Binding* b) const
    {
      value = !getOperand()->getValue(b);
    }
  };

  class OrExpression : public ::SGNaryExpression<bool>
  {
  public:
    void eval(bool& value, const expression::Binding* b) const
    {
      value = false;
      for (int i = 0; i < (int)getNumOperands(); ++i) {
        value = value || getOperand(i)->getValue(b);
        if (value)
          return;
      }
    }
  };

  class AndExpression : public ::SGNaryExpression<bool>
  {
  public:
    void eval(bool& value, const expression::Binding* b) const
    {
      value = true;
      for (int i = 0; i < (int)getNumOperands(); ++i) {
        value = value && getOperand(i)->getValue(b);
        if (!value)
          return;
      }
    }
  };

  /**
   * Convert an operand from OpType to T.
   */
  template<typename T, typename OpType>
  class ConvertExpression : public GeneralNaryExpression<T, OpType>
  {
  public:
    ConvertExpression() {}
    ConvertExpression(::SGExpression<OpType>* expr0)
    {
      this->addOperand(expr0);
    }
    virtual void eval(T& value, const simgear::expression::Binding* b) const
    {
      typename ConvertExpression::operand_type result;
      this->_expressions.at(0)->eval(result, b);
      value = result;
    }
  };
}
#endif // _SG_EXPRESSION_HXX
