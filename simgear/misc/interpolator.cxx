// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2003 Andrew J. Ross - andy@plausible.org

/**
 * @file
 * @brief Subsystem that manages smooth linear interpolation of property values across multiple data points and arbitrary time intervals.
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "interpolator.hxx"

void SGInterpolator::addNew(SGPropertyNode* prop, int nPoints)
{
    // Set the property type to a double, if it isn't already, and
    // make sure we aren't already managing this node.
    prop->setDoubleValue(prop->getDoubleValue());
    cancel(prop);

    Interp* iterp = new Interp();
    iterp->target = prop;
    iterp->nPoints = nPoints;
    iterp->curve = new double[2*nPoints];

    // Dirty trick: leave the new value sitting in _list to avoid
    // having to return a pointer to a private type.
    iterp->next = _list;
    _list = iterp;
}

void SGInterpolator::interpolate(SGPropertyNode* prop, int nPoints,
                                 double* values, double* deltas)
{
    addNew(prop, nPoints);
    for(int i=0; i<nPoints; i++) {
        _list->dt(i)  = deltas[i];
        _list->val(i) = values[i];
    }
}

void SGInterpolator::interpolate(SGPropertyNode* prop, double val, double dt)
{
    addNew(prop, 1);
    _list->dt(0) = dt;
    _list->val(0) = val;
}

//
// Delete all the list elements where "expr" is true.
//
// Silly preprocessor hack to avoid writing the linked list code in
// two places.  You would think that an STL set would be the way to
// go, but I had terrible trouble getting it to work with the
// dynamically allocated "curve" member.  Frankly, this is easier to
// write, and the code is smaller to boot...
//
#define DELETE_WHERE(EXPR)\
Interp *p = _list, **last = &_list;      \
while(p) {                               \
    if(EXPR) {                           \
        *last = p->next;                 \
        delete p;                        \
        p = (*last) ? (*last)->next : 0; \
    } else {                             \
        last = &(p->next);               \
        p = p->next; } }

void SGInterpolator::cancel(SGPropertyNode* prop)
{
    DELETE_WHERE(p->target == prop)
}

void SGInterpolator::update(double dt)
{
    DELETE_WHERE(interp(p, dt))
}

// This is the where the only "real" work happens.  Walk through the
// data points until we find one with some time left, slurp it up and
// repeat until we run out of dt.
bool SGInterpolator::interp(Interp* rec, double dt)
{
    double val = rec->target->getDoubleValue();
    int i;
    for(i=0; i < rec->nPoints; i++) {
        if(rec->dt(i) > 0 && dt < rec->dt(i)) {
            val += (dt / rec->dt(i)) * (rec->val(i) - val);
            rec->dt(i) -= dt;
            break;
        }
        dt -= rec->dt(i);
        val = rec->val(i);
    }
    rec->target->setDoubleValue(val);

    // Return true if this one is done
    return i == rec->nPoints;
}
