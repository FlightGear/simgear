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

#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/scene/model/persparam.hxx>

/// Expression tree implementation.

template<typename T>
class SGExpression : public SGReferenced {
public:
  virtual ~SGExpression() {}
  virtual void eval(T&) const = 0;

  T getValue() const
  { T value; eval(value); return value; }

  virtual bool isConst() const { return false; }
  virtual SGExpression* simplify();
};

/// Constant value expression
template<typename T>
class SGConstExpression : public SGExpression<T> {
public:
  SGConstExpression(const T& value = T()) : _value(value)
  { }
  void setValue(const T& value)
  { _value = value; }
  const T& getValue() const
  { return _value; }
  virtual void eval(T& value) const
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

protected:
  SGUnaryExpression(SGExpression<T>* expression = 0)
  { setOperand(expression); }

private:
  SGSharedPtr<SGExpression<T> > _expression;
};

template<typename T>
class SGBinaryExpression : public SGExpression<T> {
public:
  const SGExpression<T>* getOperand(unsigned i) const
  { return _expressions[i]; }
  SGExpression<T>* getOperand(unsigned i)
  { return _expressions[i]; }
  void setOperand(unsigned i, SGExpression<T>* expression)
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

protected:
  SGBinaryExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
  { setOperand(0, expr0); setOperand(1, expr1); }
  
private:
  SGSharedPtr<SGExpression<T> > _expressions[2];
};

template<typename T>
class SGNaryExpression : public SGExpression<T> {
public:
  unsigned getNumOperands() const
  { return _expressions.size(); }
  const SGExpression<T>* getOperand(unsigned i) const
  { return _expressions[i]; }
  SGExpression<T>* getOperand(unsigned i)
  { return _expressions[i]; }
  unsigned addOperand(SGExpression<T>* expression)
  {
    if (!expression)
      return ~unsigned(0);
    _expressions.push_back(expression);
    return _expressions.size() - 1;
  }

  virtual bool isConst() const
  {
    for (unsigned i = 0; i < _expressions.size(); ++i)
      if (!_expressions[i]->isConst())
        return false;
    return true;
  }
  virtual SGExpression<T>* simplify()
  {
    for (unsigned i = 0; i < _expressions.size(); ++i)
      _expressions[i] = _expressions[i]->simplify();
    return SGExpression<T>::simplify();
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
  virtual void eval(T& value) const
  { doEval(value); }
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

  virtual void eval(T& value) const
  { value = getOperand()->getValue(); if (value <= 0) value = -value; }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGACosExpression : public SGUnaryExpression<T> {
public:
  SGACosExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = acos(SGMisc<T>::clip(getOperand()->getValue(), -1, 1)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGASinExpression : public SGUnaryExpression<T> {
public:
  SGASinExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = asin(SGMisc<T>::clip(getOperand()->getValue(), -1, 1)); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGATanExpression : public SGUnaryExpression<T> {
public:
  SGATanExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = atan(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGCeilExpression : public SGUnaryExpression<T> {
public:
  SGCeilExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = ceil(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGCosExpression : public SGUnaryExpression<T> {
public:
  SGCosExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = cos(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGCoshExpression : public SGUnaryExpression<T> {
public:
  SGCoshExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = cosh(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGExpExpression : public SGUnaryExpression<T> {
public:
  SGExpExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = exp(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGFloorExpression : public SGUnaryExpression<T> {
public:
  SGFloorExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = floor(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGLogExpression : public SGUnaryExpression<T> {
public:
  SGLogExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = log(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGLog10Expression : public SGUnaryExpression<T> {
public:
  SGLog10Expression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = log10(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSinExpression : public SGUnaryExpression<T> {
public:
  SGSinExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = sin(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSinhExpression : public SGUnaryExpression<T> {
public:
  SGSinhExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = sinh(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSqrExpression : public SGUnaryExpression<T> {
public:
  SGSqrExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = getOperand()->getValue(); value = value*value; }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGSqrtExpression : public SGUnaryExpression<T> {
public:
  SGSqrtExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = sqrt(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGTanExpression : public SGUnaryExpression<T> {
public:
  SGTanExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = tan(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;
};

template<typename T>
class SGTanhExpression : public SGUnaryExpression<T> {
public:
  SGTanhExpression(SGExpression<T>* expr = 0)
    : SGUnaryExpression<T>(expr)
  { }

  virtual void eval(T& value) const
  { value = tanh(getOperand()->getValue()); }

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

  virtual void eval(T& value) const
  { value = _scale * getOperand()->getValue(); }

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

  virtual void eval(T& value) const
  { value = _bias + getOperand()->getValue(); }

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

  virtual void eval(T& value) const
  {
    if (_interpTable)
      value = _interpTable->interpolate(getOperand()->getValue());
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

  virtual void eval(T& value) const
  {
    value = SGMisc<T>::clip(getOperand()->getValue(), _clipMin, _clipMax);
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

  virtual void eval(T& value) const
  { value = apply_mods(getOperand()->getValue()); }

  using SGUnaryExpression<T>::getOperand;

private:
  T apply_mods(T property) const
  {
    T modprop;
    if (_step > 0) {
      T scrollval = 0;
      if(_scroll > 0) {
        // calculate scroll amount (for odometer like movement)
        T remainder  =  _step - fmod(fabs(property), _step);
        if (remainder < _scroll) {
          scrollval = (_scroll - remainder) / _scroll * _step;
        }
      }
      // apply stepping of input value
      if(property > 0)
        modprop = ((floor(property/_step) * _step) + scrollval);
      else
        modprop = ((ceil(property/_step) * _step) + scrollval);
    } else {
      modprop = property;
    }
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

  virtual void eval(T& value) const
  {
    if (_enable->test())
      value = getOperand()->getValue();
    else
      value = _disabledValue;
  }

  virtual SGExpression<T>* simplify()
  {
    if (!_enable)
      return getOperand()->simplify();
    return SGUnaryExpression<T>::simplify();
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
  virtual void eval(T& value) const
  { value = atan2(getOperand(0)->getValue(), getOperand(1)->getValue()); }
  using SGBinaryExpression<T>::getOperand;
};

template<typename T>
class SGDivExpression : public SGBinaryExpression<T> {
public:
  SGDivExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGBinaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value) const
  { value = getOperand(0)->getValue() / getOperand(1)->getValue(); }
  using SGBinaryExpression<T>::getOperand;
};

template<typename T>
class SGModExpression : public SGBinaryExpression<T> {
public:
  SGModExpression(SGExpression<T>* expr0, SGExpression<T>* expr1)
    : SGBinaryExpression<T>(expr0, expr1)
  { }
  virtual void eval(T& value) const
  { value = mod(getOperand(0)->getValue(), getOperand(1)->getValue()); }
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
  virtual void eval(T& value) const
  { value = pow(getOperand(0)->getValue(), getOperand(1)->getValue()); }
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
  virtual void eval(T& value) const
  {
    value = T(0);
    unsigned sz = SGNaryExpression<T>::getNumOperands();
    for (unsigned i = 0; i < sz; ++i)
      value += getOperand(i)->getValue();
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
  virtual void eval(T& value) const
  {
    value = T(1);
    unsigned sz = SGNaryExpression<T>::getNumOperands();
    for (unsigned i = 0; i < sz; ++i)
      value *= getOperand(i)->getValue();
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
  virtual void eval(T& value) const
  {
    unsigned sz = SGNaryExpression<T>::getNumOperands();
    if (sz < 1)
      return;
    
    value = getOperand(0)->getValue();
    for (unsigned i = 1; i < sz; ++i)
      value = SGMisc<T>::min(value, getOperand(i)->getValue());
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
  virtual void eval(T& value) const
  {
    unsigned sz = SGNaryExpression<T>::getNumOperands();
    if (sz < 1)
      return;
    
    value = getOperand(0)->getValue();
    for (unsigned i = 1; i < sz; ++i)
      value = SGMisc<T>::max(value, getOperand(i)->getValue());
  }
  using SGNaryExpression<T>::getOperand;
};

typedef SGExpression<int> SGExpressioni;
typedef SGExpression<float> SGExpressionf;
typedef SGExpression<double> SGExpressiond;
typedef SGExpression<bool> SGExpressionb;

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

#endif // _SG_EXPRESSION_HXX
