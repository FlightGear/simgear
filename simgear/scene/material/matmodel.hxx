// matmodel.hxx -- class to handle models tied to a material property
//
// Written by David Megginson, December 2001
//
// Copyright (C) 1998 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_MAT_MODEL_HXX
#define _SG_MAT_MODEL_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>      // Standard C++ string library
#include <vector>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/Billboard>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/sg_random.h>


class SGMatModelGroup;


/**
 * A randomly-placeable object.
 *
 * SGMaterial uses this class to keep track of the model(s) and
 * parameters for a single instance of a randomly-placeable object.
 * The object can have more than one variant model (i.e. slightly
 * different shapes of trees), but they are considered equivalent
 * and interchangeable.
 */
class SGMatModel : public SGReferenced {

public:

    /**
     * The heading type for a randomly-placed object.
     */
    enum HeadingType {
        HEADING_FIXED,
        HEADING_BILLBOARD,
        HEADING_RANDOM,
        HEADING_MASK
    };

    /**
     * Get the number of variant models available for the object.
     *
     * @return The number of variant models.
     */
    int get_model_count( SGPropertyNode *prop_root );


    /**
     * Get a randomly-selected variant model for the object.
     *
     * @return A randomly select model from the variants.
     */
    osg::Node *get_random_model( SGPropertyNode *prop_root, mt *seed );


    /**
     * Get the average number of meters^2 occupied by each instance.
     *
     * @return The coverage in meters^2.
     */
    double get_coverage_m2 () const;

    /**
     * Get the visual range of the object in meters.
     *
     * @return The visual range.
     */
    double get_range_m () const;

    /**
     * Get the minimum spacing between this and any
     * other objects in m
     *
     * @return The spacing in m.
     */
    double get_spacing_m () const;
    
    
    /**
     * Get a randomized visual range
     *
     * @return a randomized visual range
     */    
    double get_randomized_range_m(mt* seed) const;    

    /**
     * Get the heading type for the object.
     *
     * @return The heading type.
     */
    HeadingType get_heading_type () const;

    virtual ~SGMatModel ();
    

protected:

    friend class SGMatModelGroup;

    SGMatModel (const SGPropertyNode * node, double range_m);

private:

    /**
     * Actually load the models.
     *
     * This class uses lazy loading so that models won't be held
     * in memory for materials that are never referenced.
     */
    void load_models( SGPropertyNode *prop_root );

    std::vector<std::string> _paths;
    mutable std::vector<osg::ref_ptr<osg::Node> > _models;
    mutable bool _models_loaded;
    double _coverage_m2;
    double _spacing_m;
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
class SGMatModelGroup : public SGReferenced {

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
    std::vector<SGSharedPtr<SGMatModel> > _objects;
};

#endif // _SG_MAT_MODEL_HXX 
