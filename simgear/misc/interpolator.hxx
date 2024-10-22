// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2003 Andrew J. Ross <andy@plausible.org>

/**
 * @file
 * @brief Subsystem that manages smooth linear interpolation of property values across multiple data points and arbitrary time intervals.
 */

#pragma once

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

// TODO: support a callback upon interpolation completion so that user
// code can register another one immediately without worrying about
// timer aliasing.

class SGInterpolator : public SGSubsystem
{
public:
    SGInterpolator() { _list = 0; }

    // Subsystem API.
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "interpolator"; }

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

        virtual ~Interp() { delete[] curve; }
        double& dt(int i)  { return curve[2*i]; }
        double& val(int i) { return curve[2*i + 1]; }
    };
    Interp* _list;

    bool interp(Interp* rec, double dt);
    void addNew(SGPropertyNode* prop, int nPoints);
};
