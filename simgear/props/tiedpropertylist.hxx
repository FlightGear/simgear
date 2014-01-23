// tiedpropertylist.hxx -- Maintain tied properties
//
// Written by Torsten Dreyer started in 2010
//
// Copyright (C) 2010  Torsten Dreyer - torsten (at) t3r _dot_ de
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
#ifndef __TIEDPROPERTYLIST_HXX
#define  __TIEDPROPERTYLIST_HXX
#include <simgear/props/props.hxx>
#include <assert.h>

namespace simgear {

/** 
 * @brief A list of tied properties that get automatically untied
 * This helper class keeps track of tied properties and unties
 * each tied property when this class gets destructed.
*/
class TiedPropertyList : simgear::PropertyList {
public:
    TiedPropertyList() {}
    TiedPropertyList( SGPropertyNode_ptr root ) : _root(root) {}
    virtual ~TiedPropertyList()
    { 
        _root = 0;
        if (! empty())
        {
            SG_LOG(SG_GENERAL, SG_ALERT, "Detected properties with dangling ties. Use 'Untie' before removing a TiedPropertyList.");
            // running debug mode: go, fix it!
            assert(empty());
        }
    }

    void setRoot( SGPropertyNode_ptr root ) { _root = root; }
    SGPropertyNode_ptr getRoot() const { return _root; }

    template<typename T> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, const SGRawValue<T> &rawValue, bool useDefault = true  ) {
        bool success = node->tie( rawValue, useDefault );
        if( success ) {
            push_back( node );
        } else {
#if PROPS_STANDALONE
            cerr << "Failed to tie property " << node->getPath() << endl;
#else
            SG_LOG(SG_GENERAL, SG_WARN, "Failed to tie property " << node->getPath() );
#endif
        }
        return node;
    }

    template <class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, V * value, bool useDefault = true ) {
        return Tie( node, SGRawValuePointer<V>(value), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, V * value, bool useDefault = true ) {
        return Tie( _root->getNode(relative_path,true), SGRawValuePointer<V>(value), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, int prop_index, V * value, bool useDefault = true ) {
        return Tie( _root->getNode(relative_path,prop_index,true), SGRawValuePointer<V>(value), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, V (*getter)(), void (*setter)(V) = 0, bool useDefault = true ) {
        return Tie(node, SGRawValueFunctions<V>(getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, V (*getter)(), void (*setter)(V) = 0, bool useDefault = true ) {
        return Tie(_root->getNode(relative_path,true), SGRawValueFunctions<V>(getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, int prop_index, V (*getter)(), void (*setter)(V) = 0, bool useDefault = true ) {
        return Tie(_root->getNode(relative_path,prop_index,true), SGRawValueFunctions<V>(getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, int index, V (*getter)(int), void (*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( node, SGRawValueFunctionsIndexed<V>(index, getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, int index, V (*getter)(int), void (*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path, true ), SGRawValueFunctionsIndexed<V>(index, getter, setter), useDefault );
    }

    template <class V> SGPropertyNode_ptr Tie( const char * relative_path, int prop_index, int index, V (*getter)(int), void (*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path,prop_index,true ), SGRawValueFunctionsIndexed<V>(index, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, T * obj, V (T::*getter)() const, void (T::*setter)(V) = 0, bool useDefault = true) {
        return Tie( node, SGRawValueMethods<T,V>(*obj, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( const char * relative_path, T * obj, V (T::*getter)() const, void (T::*setter)(V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path, true), SGRawValueMethods<T,V>(*obj, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( const char * relative_path, int prop_index, T * obj, V (T::*getter)() const, void (T::*setter)(V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path,prop_index,true), SGRawValueMethods<T,V>(*obj, getter, setter), useDefault );
    }

    template <class T, class V> SGPropertyNode_ptr Tie( SGPropertyNode_ptr node, T * obj, int index, V (T::*getter)(int) const, void (T::*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( node, SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter), useDefault);
    }

    template <class T, class V> SGPropertyNode_ptr Tie( const char * relative_path, T * obj, int index, V (T::*getter)(int) const, void (T::*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path, true ), SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter), useDefault);
    }

    template <class T, class V> SGPropertyNode_ptr Tie( const char * relative_path, int prop_index, T * obj, int index, V (T::*getter)(int) const, void (T::*setter)(int, V) = 0, bool useDefault = true) {
        return Tie( _root->getNode( relative_path,prop_index,true ), SGRawValueMethodsIndexed<T,V>(*obj, index, getter, setter), useDefault);
    }

    void Untie() {
        while( ! empty() ) {
            back()->untie();
            pop_back();
        }
    }

    void setAttribute (SGPropertyNode::Attribute attr, bool state)
    {
        for (std::vector<SGPropertyNode_ptr>::iterator it=begin() ; it < end(); it++ )
           (*it)->setAttribute(attr, state);
    }
private:
    SGPropertyNode_ptr _root;
};

} // namespace
#endif
