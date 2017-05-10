// Copyright (C) 2017 Richard Harrison rjh@zaretto.com
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

#ifndef SGTRANSIENTMODELDATA_HXX
#define SGTRANSIENTMODELDATA_HXX 1
#include <simgear/math/SGGeometry.hxx>

namespace simgear
{
    typedef std::map<std::string, SGLineSegment<double>> SGAxisDefinitionMap;

    /*
     * Transient data that is used with a given context for any given model. 
     * Currently used during the model load to provide a consistent set of data when creating the animations.
     * - provides a map to allow reuse of axis definitions
     * Expected possible future expansion for improvements to model light animations.
     * - typical usage for the SGAnimation::animate() call would be to create one of these like this
     *      simgear::SGTransientModelData modelData(group.get(), prop_root, options.get(), path.local8BitStr());
     *   This will create the basic transient model data that doesn't change between invocations of animate()
     *   then to adapt the model data for an animation element use the LoadAnimationValuesForElement method, e.g.
     *         modelData.LoadAnimationValuesForElement(animation_nodes[i], i);
     */
    class SGTransientModelData {
    public:
        // fully specified constructor. Probably not relevant as creating an object this specified implies setting up for an individual animation element, which will preclude
        // the use of the axis definition cache between objects.
        SGTransientModelData(osg::Node* _node, const SGPropertyNode* _configNode, SGPropertyNode* _modelRoot, osgDB::Options* _options, const std::string &_path, int _i)
            : node(_node), configNode(_configNode), modelRoot(_modelRoot), options(_options), path(_path), index(_i)
        {}
        // usual form of construction - setup the elements that are constant, and specify the config node and index via LoadAnimationValuesForElement 
        SGTransientModelData(osg::Node* _node, SGPropertyNode* _modelRoot, osgDB::Options* _options, const std::string &_path)
            : node(_node), configNode(nullptr), modelRoot(_modelRoot), options(_options), path(_path), index(0)
        {}

        /*
         * Loads the animation values required for a specific element.
         */
        void LoadAnimationValuesForElement(const SGPropertyNode* _configNode, int _index)
        {
            configNode = _configNode;
            index = _index;
        }

        osg::Node*  getNode() { return node; }
        const SGPropertyNode* getConfigNode() { return configNode; }
        SGPropertyNode* getModelRoot() { return modelRoot; }
        const osgDB::Options* getOptions() { return options; }
        const std::string &getPath() { return path; }
        int getIndex() { return index; }

        /*
         * Find an already located axis definition object line segment. Returns null if nothing found.
         */
        const SGLineSegment<double> *getAxisDefinition(const std::string axis_object_name)
        {
            SGAxisDefinitionMap::const_iterator axisDefinition = axisDefinitions.find(axis_object_name);

            if (axisDefinition != axisDefinitions.end())
            {
                return &(axisDefinition->second);
            }
            return 0;
        }
        /*
         * Add an axis definition line segment. Always returns the line segment that has been added.
         */
        const SGLineSegment<double> *addAxisDefinition(const std::string object_name, const SGLineSegment<double> &line)
        {
            axisDefinitions[object_name] = line;
            return getAxisDefinition(object_name);
        }

    private:
        osg::Node* node = nullptr;
        const SGPropertyNode* configNode = nullptr;
        SGPropertyNode* modelRoot = nullptr;
        const osgDB::Options* options = nullptr;
        const std::string path;
        int index = 0;
        SGAxisDefinitionMap axisDefinitions;

    };
}
#endif
