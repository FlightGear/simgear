// matmodel.hxx -- class to handle models tied to a material property
//
// Written by David Megginson, December 2001
//
// Copyright (C) 1998 - 2003  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _SG_MAT_MODEL_HXX
#define _SG_MAT_MODEL_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>

#include STL_STRING      // Standard C++ string library

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/props/props.hxx>

SG_USING_STD(string);


class SGMatModelGroup;
class SGModelLib;


/**
 * A randomly-placeable object.
 *
 * SGMaterial uses this class to keep track of the model(s) and
 * parameters for a single instance of a randomly-placeable object.
 * The object can have more than one variant model (i.e. slightly
 * different shapes of trees), but they are considered equivalent
 * and interchangeable.
 */
class SGMatModel {

public:

    /**
     * The heading type for a randomly-placed object.
     */
    enum HeadingType {
        HEADING_FIXED,
        HEADING_BILLBOARD,
        HEADING_RANDOM
    };

    /**
     * Get the number of variant models available for the object.
     *
     * @return The number of variant models.
     */
    int get_model_count( SGModelLib *modellib,
                         const string &fg_root,
                         SGPropertyNode *prop_root,
                         double sim_time_sec );


    /**
     * Get a specific variant model for the object.
     *
     * @param index The index of the model.
     * @return The model.
     */
    ssgEntity *get_model( int index,
                          SGModelLib *modellib,
                          const string &fg_root,
                          SGPropertyNode *prop_root,
                          double sim_time_sec );


    /**
     * Get a randomly-selected variant model for the object.
     *
     * @return A randomly select model from the variants.
     */
    ssgEntity *get_random_model( SGModelLib *modellib,
                                 const string &fg_root,
                                 SGPropertyNode *prop_root,
                                 double sim_time_sec );


    /**
     * Get the average number of meters^2 occupied by each instance.
     *
     * @return The coverage in meters^2.
     */
    double get_coverage_m2 () const;


    /**
     * Get the heading type for the object.
     *
     * @return The heading type.
     */
    HeadingType get_heading_type () const;

protected:

    friend class SGMatModelGroup;

    SGMatModel (const SGPropertyNode * node, double range_m);

    virtual ~SGMatModel ();

private:

    /**
     * Actually load the models.
     *
     * This class uses lazy loading so that models won't be held
     * in memory for materials that are never referenced.
     */
    void load_models( SGModelLib *modellib,
                      const string &fg_root,
                      SGPropertyNode *prop_root,
                      double sim_time_sec );

    vector<string> _paths;
    mutable vector<ssgEntity *> _models;
    mutable bool _models_loaded;
    double _coverage_m2;
    double _range_m;
    HeadingType _heading_type;
};


/**
 * A collection of related objects with the same visual range.
 *
 * Grouping objects with the same range together significantly
 * reduces the memory requirements of randomly-placed objects.
 * Each SGMaterial instance keeps a (possibly-empty) list of
 * object groups for placing randomly on the scenery.
 */
class SGMatModelGroup {

public:

    virtual ~SGMatModelGroup ();


    /**
     * Get the visual range of the object in meters.
     *
     * @return The visual range.
     */
    double get_range_m () const;


    /**
     * Get the number of objects in the group.
     *
     * @return The number of objects.
     */
    int get_object_count () const;


    /**
     * Get a specific object.
     *
     * @param index The object's index, zero-based.
     * @return The object selected.
     */
    SGMatModel * get_object (int index) const;

protected:

    friend class SGMaterial;

    SGMatModelGroup (SGPropertyNode * node);

private:

    double _range_m;
    vector<SGMatModel *> _objects;

};


#endif // _SG_MAT_MODEL_HXX 
