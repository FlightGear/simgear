#ifndef __SG_INTERPOLATOR_HXX
#define __SG_INTERPOLATOR_HXX

// SGInterpolator
//   Subsystem that manages smooth linear interpolation of property
//   values across multiple data points and arbitrary time intervals.

// Written by Andrew J. Ross, started December 2003
//
// Copyright (C) 2003  Andrew J. Ross - andy@plausible.org
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

// TODO: support a callback upon interpolation completion so that user
// code can register another one immediately without worrying about
// timer aliasing.

class SGInterpolator : public SGSubsystem {
public:
    SGInterpolator() { _list = 0; }
    virtual void init() {}
    virtual void update(double delta_time_sec);

    // Simple method that interpolates a double property value from
    // its current value (default of zero) to the specified target
    // over the specified time.
    void interpolate(SGPropertyNode* prop, double value, double dt_sec);

    // More elaborate version that takes a pointer to lists of
    // arbitrary size.
    void interpolate(SGPropertyNode* prop, int nPoints,
                     double* values, double* deltas);

    // Cancels any interpolation of the specified property, leaving
    // its value at the current (mid-interpolation) state.
    void cancel(SGPropertyNode* prop);

private:
    struct Interp {
        SGPropertyNode_ptr target;
        int nPoints;
        double* curve; // time0, val0, time1, val1, ...
        Interp* next;

        ~Interp() { delete[] curve; }
        double& dt(int i)  { return curve[2*i]; }
        double& val(int i) { return curve[2*i + 1]; }
    };
    Interp* _list;

    bool interp(Interp* rec, double dt);
    void addNew(SGPropertyNode* prop, int nPoints);
};

#endif // __SG_INTERPOLATOR_HXX
