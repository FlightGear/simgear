// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2000 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Manage star data
 */

#ifndef _SG_STARDATA_HXX
#define _SG_STARDATA_HXX

#include <vector>
#include <simgear/math/SGMath.hxx>

class SGPath;

class SGStarData final {
public:
    // Constructor
    SGStarData( const SGPath& path );

    // Destructor
    ~SGStarData();  // non-virtual is intentional

    // load the stars database
    bool load( const SGPath& path );

    // stars
    inline int getNumStars() const { return static_cast<int>(_stars.size()); }
    inline SGVec3d *getStars() { return &(_stars[0]); }

private:
    std::vector<SGVec3d> _stars;
};


#endif // _SG_STARDATA_HXX
