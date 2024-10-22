// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 Torsten Dreyer

#pragma once

#include <simgear/structure/SGExpression.hxx>

namespace simgear {

using Value_ptr = SGSharedPtr<class Value>;
using PeriodicalValue_ptr = SGSharedPtr<class PeriodicalValue>;


/**
 * @brief Model a periodical value like angular values
 *
 * Most common use for periodical values are angular values.
 * If y = f(x) = f(x + n*period), this is a periodical function
 */

class PeriodicalValue : public SGReferenced {
private:
    Value_ptr minPeriod; // The minimum value of the period
    Value_ptr maxPeriod; // The maximum value of the period
public:
     PeriodicalValue( SGPropertyNode& prop_root,
                      SGPropertyNode& cfg );
     double normalize( double value ) const;
     double normalizeSymmetric( double value ) const;
};

/**
 * @brief A input value for analog autopilot components
 *
 * Input values may be constants, property values, transformed with a scale
 * and/or offset, clamped to min/max values, be periodical, bound to 
 * conditions or evaluated from expressions.
 */
class Value : public SGReferenced, public SGPropertyChangeListener
{
 private:
     double             _value = 0.0;    // The value as a constant or initializer for the property
     bool _abs = false;            // return absolute value
     SGPropertyNode_ptr _property; // The name of the property containing the value
     Value_ptr _offset;            // A fixed offset, defaults to zero
     Value_ptr _scale;             // A constant scaling factor defaults to one
     Value_ptr _min;               // A minimum clip defaults to no clipping
     Value_ptr _max;               // A maximum clip defaults to no clipping
     PeriodicalValue_ptr  _periodical; //
     SGSharedPtr<const SGCondition> _condition;
     SGSharedPtr<SGExpressiond> _expression;  ///< expression to generate the value
     SGPropertyNode_ptr _pathNode;
     SGPropertyNode_ptr _rootNode;
    
    void valueChanged(SGPropertyNode* node) override;
    void initPropertyFromInitialValue();
public:
    Value(SGPropertyNode& prop_root,
          SGPropertyNode& node,
          double value = 0.0,
          double offset = 0.0,
          double scale = 1.0);
    Value(double value = 0.0);
    virtual ~Value();

    /**
     *
     * @param prop_root Root node for all properties with relative path
     * @param cfg       Configuration node
     * @param value     Default initial value
     * @param offset    Default initial offset
     * @param scale     Default initial scale
     */
    void parse( SGPropertyNode& prop_root,
                SGPropertyNode& cfg,
                double value = 0.0,
                double offset = 0.0,
                double scale = 1.0 );

    /* get the value of this input, apply scale and offset and clipping */
    double get_value() const;

    /* set the input value after applying offset and scale */
    void set_value( double value );

    inline double get_scale() const {
        return _scale == nullptr ? 1.0 : _scale->get_value();
    }

    inline double get_offset() const {
        return _offset == nullptr ? 0.0 : _offset->get_value();
    }

    bool is_enabled() const;

    void collectDependentProperties(std::set<const SGPropertyNode*>& props) const;
};

/**
 * @brief A chained list of Values
 *
 * Many compoments support ValueLists as input. Each Value may be bound to
 * a condition. This list supports the get_value() function to retrieve the value
 * of the first InputValue in this list that has a condition evaluating to true.
 */
class ValueList : public std::vector<Value_ptr>
{
public:
    ValueList(double def = 0.0) : _def(def) {}

    Value_ptr get_active() const;

    double get_value() const
    {
        Value_ptr input = get_active();
        return input == nullptr ? _def : input->get_value();
    }

    void collectDependentProperties(std::set<const SGPropertyNode*>& props) const;
  private:
      double _def = 0.0;
};

} // namespace simgear