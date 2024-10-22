/**
 * \file props.hxx
 * Interface definition for a property list.
 * Started Fall 2000 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * See props.html for documentation [replace with URL when available].
 *
 * $Id$
 */
#ifndef __PROPS_HXX
#define __PROPS_HXX

#ifdef SG_PROPS_UNTHREADSAFE
    #include "props-unsafe.hxx"
#else

#ifndef PROPS_STANDALONE
#define PROPS_STANDALONE 0
#endif

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <shared_mutex>
		
#include <simgear/compiler.h>
#if PROPS_STANDALONE
# ifndef SG_LOG
#   define SG_GENERAL	0
#   define SG_ALERT	0
#   define SG_WARN	1
#   define SG_LOG(type, level, message) (type) ? (std::cerr <<message << endl) : (std::cout <<message << endl)
# endif
#else
# include <simgear/debug/logstream.hxx>
# include <simgear/math/SGMathFwd.hxx>
# include <simgear/math/sg_types.hxx>
#endif

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/SGSourceLocation.hxx>
#include <simgear/structure/SGWeakPtr.hxx>

namespace simgear
{

class PropertyInterpolationMgr;

template<typename T>
std::istream& readFrom(std::istream& stream, T& result)
{
    stream >> result;
    return stream;
}

/**
 * Parse a string as an object of a given type.
 * XXX no error behavior yet.
 *
 * @tparam T the return type
 * @param str the string
 * @return the object.
 */
template<typename T>
inline T parseString(const std::string& str)
{
    std::istringstream stream(str);
    T result;
    readFrom(stream, result);
    return result;
}

//
// Property value types
//

#ifdef NONE
#pragma warn A sloppy coder has defined NONE as a macro!
#undef NONE
#endif

#ifdef ALIAS
#pragma warn A sloppy coder has defined ALIAS as a macro!
#undef ALIAS
#endif

#ifdef UNSPECIFIED
#pragma warn A sloppy coder has defined UNSPECIFIED as a macro!
#undef UNSPECIFIED
#endif

#ifdef BOOL
#pragma warn A sloppy coder has defined BOOL as a macro!
#undef BOOL
#endif

#ifdef INT
#pragma warn A sloppy coder has defined INT as a macro!
#undef INT
#endif

#ifdef LONG
#pragma warn A sloppy coder has defined LONG as a macro!
#undef LONG
#endif

#ifdef FLOAT
#pragma warn A sloppy coder has defined FLOAT as a macro!
#undef FLOAT
#endif

#ifdef DOUBLE
#pragma warn A sloppy coder has defined DOUBLE as a macro!
#undef DOUBLE
#endif

#ifdef STRING
#pragma warn A sloppy coder has defined STRING as a macro!
#undef STRING
#endif

/// Property system which associates property names with values.
namespace props
{
/**
 * The possible types of an SGPropertyNode. Types that appear after
 * EXTENDED are not stored in the SGPropertyNode itself.
 */
enum Type {
    NONE = 0, /**< The node hasn't been assigned a value yet. */
    ALIAS, /**< The node "points" to another node. */
    BOOL,
    INT,
    LONG,
    FLOAT,
    DOUBLE,
    STRING,
    UNSPECIFIED,
    EXTENDED, /**< The node's value is not stored in the property;
               * the actual value and type is retrieved from an
               * SGRawValue node. This type is never returned by @see
               * SGPropertyNode::getType.
               */
    // Extended properties
    VEC3D,
    VEC4D
};

template<typename T> struct PropertyTraits;

#define DEFINTERNALPROP(TYPE, PROP) \
template<> \
struct PropertyTraits<TYPE> \
{ \
    static const Type type_tag = PROP; \
    enum  { Internal = 1 }; \
}

DEFINTERNALPROP(bool, BOOL);
DEFINTERNALPROP(int, INT);
DEFINTERNALPROP(long, LONG);
DEFINTERNALPROP(float, FLOAT);
DEFINTERNALPROP(double, DOUBLE);
DEFINTERNALPROP(const char*, STRING);
DEFINTERNALPROP(const char[], STRING);
DEFINTERNALPROP(std::string, STRING);
#undef DEFINTERNALPROP

} // namespace props
} // namespace simgear



////////////////////////////////////////////////////////////////////////
// A raw value.
//
// This is the mechanism that information-providing routines can
// use to link their own values to the property manager.  Any
// SGValue can be tied to a raw value and then untied again.
//
// Note: we are forced to use inlined methods here to ensure
// that the templates will be instantiated.  We're probably taking
// a small performance hit for that.
////////////////////////////////////////////////////////////////////////

/**
 * Base class for SGRawValue classes that holds no type
 * information. This allows some generic manipulation of the
 * SGRawValue object.
 */
class SGRaw
{
public:
    /**
     * Get the type enumeration for the raw value.
     *
     * @return the type.
     */
    virtual simgear::props::Type getType() const = 0;
    virtual ~SGRaw() {}
    
    /**
     * Create a new deep copy of this raw value.
     *
     * The copy will contain its own version of the underlying value
     * as well, and will be the same type.
     *
     * @return A deep copy of the current object.
     */
    virtual SGRaw* clone() const = 0;

};

class SGRawExtended : public SGRaw
{
public:
    /**    
     * Make an SGRawValueContainer from the SGRawValue.
     *
     * This is a virtual function of SGRawExtended so that
     * SGPropertyNode::untie doesn't need to know the type of an
     * extended property.
     */
    virtual SGRawExtended* makeContainer() const = 0;
    /**
     * Write value out to a stream
     */
    virtual std::ostream& printOn(std::ostream& stream) const = 0;
    /**
     * Read value from a stream and store it.
     */
    virtual std::istream& readFrom(std::istream& stream) = 0;
};

// Choose between different base classes based on whether the value is
// stored internal to the property node. This frees us from defining
// the virtual functions in the SGRawExtended interface where they
// don't make sense, e.g. readFrom for the const char* type.
template<typename T, int internal = simgear::props::PropertyTraits<T>::Internal>
class SGRawBase;

template<typename T>
class SGRawBase<T, 1> : public SGRaw
{
};

template<typename T>
class SGRawBase<T, 0> : public SGRawExtended
{
    virtual SGRawExtended* makeContainer() const;
    virtual std::ostream& printOn(std::ostream& stream) const;
    virtual std::istream& readFrom(std::istream& stream);
};

/**
 * Abstract base class for a raw value.
 *
 * The property manager is implemented in two layers. The SGPropertyNode is the
 * highest and most abstract layer, representing an LValue/RValue pair: it
 * records the position of the property in the property tree and contains
 * facilities for navigation to other nodes. It is guaranteed to be persistent:
 * the SGPropertyNode will not change during a session, even if the property is
 * bound and unbound multiple times.
 *
 * When the property value is not managed internally in the
 * SGPropertyNode, the SGPropertyNode will contain a reference to an
 * SGRawValue (this class), which provides an abstract way to get,
 * set, and clone the underlying value.  The SGRawValue may change
 * frequently during a session as a value is retyped or bound and
 * unbound to various data source, but the abstract SGPropertyNode
 * layer insulates the application from those changes.
 *
 * The SGPropertyNode class always keeps a *copy* of a raw value, not the
 * original one passed to it; if you override a derived class but do not replace
 * the {@link SGRaw::clone clone()} method, strange things will happen.
 *
 * All derived SGRawValue classes must implement getValue(), setValue(), and
 * {@link SGRaw::clone clone()} for the appropriate type.
 *
 * @see SGPropertyNode
 * @see SGRawValuePointer
 * @see SGRawValueFunctions
 * @see SGRawValueFunctionsIndexed
 * @see SGRawValueMethods
 * @see SGRawValueMethodsIndexed
 * @see SGRawValueContainer
 */
template <class T>
class SGRawValue : public SGRawBase<T>
{
public:

    /**
     * The default underlying value for this type.
     *
     * Every raw value has a default; the default is false for a
     * boolean, 0 for the various numeric values, and "" for a string.
     * If additional types of raw values are added in the future, they
     * may need different kinds of default values (such as epoch for a
     * date type).  The default value is used when creating new values.
     */
    static T DefaultValue()
    {
        return T();
    }

    /**
     * Constructor.
     *
     * Use the default value for this type.
     */
    SGRawValue() {}

    /**
     * Destructor.
     */
    virtual ~SGRawValue() {}

    /**
     * Return the underlying value.
     *
     * @return The actual value for the property.
     * @see #setValue
     */
    virtual T getValue() const = 0;

    /**
     * Assign a new underlying value.
     *
     * If the new value cannot be set (because this is a read-only
     * raw value, or because the new value is not acceptable for
     * some reason) this method returns false and leaves the original
     * value unchanged.
     *
     * @param value The actual value for the property.
     * @return true if the value was set successfully, false otherwise.
     * @see #getValue
     */
    virtual bool setValue(T value) = 0;

    /**
     * Return the type tag for this raw value type.
     */
    virtual simgear::props::Type getType() const
    {
        return simgear::props::PropertyTraits<T>::type_tag;
    }
};



////////////////////////////////////////////////////////////////////////
// Default values for every type.
////////////////////////////////////////////////////////////////////////

template<> inline bool SGRawValue<bool>::DefaultValue()
{
    return false;
}

template<> inline const char* SGRawValue<const char*>::DefaultValue()
{
    return "";
}

/**
 * A raw value bound to a pointer.
 *
 * This is the most efficient way to tie an external value, but also
 * the most dangerous, because there is no way for the supplier to
 * perform bounds checking and derived calculations except by polling
 * the variable to see if it has changed.  There is no default
 * constructor, because this class would be meaningless without a
 * pointer.
 */
template <class T>
class SGRawValuePointer : public SGRawValue<T>
{
public:

    /**
     * Explicit pointer constructor.
     *
     * Create a new raw value bound to the value of the variable
     * referenced by the pointer.
     *
     * @param ptr The pointer to the variable to which this raw value
     * will be bound.
     */
    SGRawValuePointer(T* ptr) : _ptr(ptr)
    {}

    /**
     * Destructor.
     */
    virtual ~SGRawValuePointer()
    {}

    /**
     * Get the underlying value.
     *
     * This method will dereference the pointer and return the
     * variable's value.
     */
    virtual T getValue() const
    {
        return *_ptr;
    }

    /**
     * Set the underlying value.
     *
     * This method will dereference the pointer and change the
     * variable's value.
     */
    virtual bool setValue(T value)
    {
        *_ptr = value;
        return true;
    }

    /**
     * Create a copy of this raw value.
     *
     * The copy will use the same external pointer as the original.
     */
    virtual SGRaw* clone() const
    {
      return new SGRawValuePointer(_ptr);
    }

private:
    T* _ptr;
};


/**
 * A value managed through static functions.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class T>
class SGRawValueFunctions : public SGRawValue<T>
{
public:

    /**
     * The template type of a static getter function.
     */
    typedef T (*getter_t)();

    /**
     * The template type of a static setter function.
     */
    typedef void (*setter_t)(T);

    /**
     * Explicit constructor.
     *
     * Create a new raw value bound to the getter and setter supplied.
     *
     * @param getter A static function for getting a value, or 0
     * to read-disable the value.
     * @param setter A static function for setting a value, or 0
     * to write-disable the value.
     */
    SGRawValueFunctions(getter_t getter = 0, setter_t setter = 0)
        : _getter(getter), _setter(setter)
    {}

    /**
     * Destructor.
     */
    virtual ~SGRawValueFunctions()
    {}

    /**
     * Get the underlying value.
     *
     * This method will invoke the getter function to get a value.
     * If no getter function was supplied, this method will always
     * return the default value for the type.
     */
    virtual T getValue() const
    {
        if (_getter) return (*_getter)();
        else return SGRawValue<T>::DefaultValue();
    }

    /**
     * Set the underlying value.
     *
     * This method will invoke the setter function to change the
     * underlying value.  If no setter function was supplied, this
     * method will return false.
     */
    virtual bool setValue(T value)
    {
        if (_setter) {
            (*_setter)(value);
            return true;
        }
        else return false;
    }

    /**
     * Create a copy of this raw value, bound to the same functions.
     */
    virtual SGRaw* clone () const
    {
      return new SGRawValueFunctions(_getter,_setter);
    }

private:
    getter_t _getter;
    setter_t _setter;
};


/**
 * An indexed value bound to static functions.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.  An indexed value is useful for binding one
 * of a list of possible values (such as multiple engines for a
 * plane).  The index is hard-coded at creation time.
 *
 * @see SGRawValue
 */
template <class T>
class SGRawValueFunctionsIndexed : public SGRawValue<T>
{
public:
    typedef T (*getter_t)(int);
    typedef void (*setter_t)(int,T);

    SGRawValueFunctionsIndexed(int index, getter_t getter = 0, setter_t setter = 0)
        : _index(index), _getter(getter), _setter(setter)
    {}

    virtual ~SGRawValueFunctionsIndexed()
    {}

    virtual T getValue() const
    {
        if (_getter) return (*_getter)(_index);
        else return SGRawValue<T>::DefaultValue();
    }

    virtual bool setValue(T value)
    {
        if (_setter) {
            (*_setter)(_index, value);
            return true;
        }
        else return false;
    }

    virtual SGRaw* clone() const
    {
        return new SGRawValueFunctionsIndexed(_index, _getter, _setter);
    }
  
private:
    int _index;
    getter_t _getter;
    setter_t _setter;
};


/**
 * A value managed through an object and access methods.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C, class T>
class SGRawValueMethods : public SGRawValue<T>
{
public:
    typedef T (C::*getter_t)() const;
    typedef void (C::*setter_t)(T);

    SGRawValueMethods(C &obj, getter_t getter = 0, setter_t setter = 0)
        : _obj(obj), _getter(getter), _setter(setter)
    {}

    virtual ~SGRawValueMethods()
    {}

    virtual T getValue() const
    {
        if (_getter) {
            return (_obj.*_getter)();
        }
        else {
            return SGRawValue<T>::DefaultValue();
        }
    }

    virtual bool setValue(T value)
    {
        if (_setter)
        {
            (_obj.*_setter)(value);
            return true;
        }
        else return false;
    }

    virtual SGRaw* clone() const
    {
        return new SGRawValueMethods(_obj, _getter, _setter);
    }
  
private:
    C &_obj;
    getter_t _getter;
    setter_t _setter;
};


/**
 * A string value managed through an object and access methods.
 *
 * This class stores the std::string returned by the getter, so that
 * the value returned by c_str() is still valid.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C>
class SGStringValueMethods : public SGRawValue<const char*>
{
public:
    typedef std::string (C::*getter_t)() const;
    typedef void (C::*setter_t)(const std::string&);

    SGStringValueMethods (C &obj, getter_t getter = 0, setter_t setter = 0)
    : _obj(obj), _getter(getter), _setter(setter)
    {}

    virtual ~SGStringValueMethods()
    {}

    virtual const char* getValue() const
    {
        SGStringValueMethods* pthis = const_cast<SGStringValueMethods*>(this);
        if (_getter) {
            pthis->_value = (_obj.*_getter)();
            return pthis->_value.c_str();
        }
        else {
            return SGRawValue<const char*>::DefaultValue();
        }
    }

    virtual bool setValue(const char* value)
    {
        if (_setter) {
            (_obj.*_setter)(value ? value : "");
            return true;
        }
        else return false;
    }

    virtual SGRaw* clone() const
    {
      return new SGStringValueMethods(_obj, _getter, _setter);
    }
  
private:
    C &_obj;
    getter_t _getter;
    setter_t _setter;
    std::string _value;
};


/**
 * An indexed value managed through an object and access methods.
 *
 * A read-only value will not have a setter; a write-only value will
 * not have a getter.
 */
template <class C, class T>
class SGRawValueMethodsIndexed : public SGRawValue<T>
{
public:
    typedef T (C::*getter_t)(int) const;
    typedef void (C::*setter_t)(int, T);
    SGRawValueMethodsIndexed(C &obj, int index, getter_t getter = 0, setter_t setter = 0)
      : _obj(obj), _index(index), _getter(getter), _setter(setter)
    {}

    virtual ~SGRawValueMethodsIndexed()
    {}

    virtual T getValue() const
    {
        if (_getter)
        {
            return (_obj.*_getter)(_index);
        }
        else {
            return SGRawValue<T>::DefaultValue();
        }
    }
    virtual bool setValue(T value)
    {
        if (_setter) {
            (_obj.*_setter)(_index, value);
            return true;
        }
        else return false;
    }

    virtual SGRaw* clone() const
    {
        return new SGRawValueMethodsIndexed(_obj, _index, _getter, _setter);
    }
  
private:
    C &_obj;
    int _index;
    getter_t _getter;
    setter_t _setter;
};

/**
 * A raw value that contains its value. This provides a way for
 * property nodes to contain values that shouldn't be stored in the
 * property node itself.
 */
template <class T>
class SGRawValueContainer : public SGRawValue<T>
{
public:

    /**
     * Explicit constructor.
     */
    SGRawValueContainer(const T& obj) : _obj(obj)
    {}

    /**
     * Destructor.
     */
    virtual ~SGRawValueContainer()
    {}

    /**
     * Get the underlying value.
     */
    virtual T getValue() const
    {
        return _obj;
    }

    /**
     * Set the underlying value.
     *
     * This method will dereference the pointer and change the
     * variable's value.
     */
    virtual bool setValue(T value)
    {
        _obj = value;
        return true;
    }

    /**
     * Create a copy of this raw value.
     */
    virtual SGRaw* clone() const
    {
        return new SGRawValueContainer(_obj);
    }

private:
    T _obj;
};

template<typename T>
SGRawExtended* SGRawBase<T, 0>::makeContainer() const
{
    return new SGRawValueContainer<T>(
            static_cast<const SGRawValue<T>*>(this)->getValue()
            );
}

template<typename T>
std::ostream& SGRawBase<T, 0>::printOn(std::ostream& stream) const
{
    return stream << static_cast<SGRawValue<T>*>(this)->getValue();
}

template<typename T>
std::istream& SGRawBase<T, 0>::readFrom(std::istream& stream)
{
    T value;
    simgear::readFrom(stream, value);
    static_cast<SGRawValue<T>*>(this)->setValue(value);
    return stream;
}

/**
 * The smart pointer that manage reference counting
 */
class SGPropertyNode;
typedef SGSharedPtr<SGPropertyNode> SGPropertyNode_ptr;
typedef SGSharedPtr<const SGPropertyNode> SGConstPropertyNode_ptr;

namespace simgear
{
    using PropertyList = std::vector<SGPropertyNode_ptr>;
}

/**
 * The property change listener interface.
 *
 * <p>Any class that needs to listen for property changes must implement
 * this interface.</p>
 */
class SGPropertyChangeListener
{
public:
    virtual ~SGPropertyChangeListener();

    /// Called if value of \a node has changed.
    virtual void valueChanged(SGPropertyNode* node);

    /// Called if \a child has been added to the given \a parent.
    virtual void childAdded(SGPropertyNode* parent, SGPropertyNode* child);

    /// Called if \a child has been removed from its \a parent.
    virtual void childRemoved(SGPropertyNode* parent, SGPropertyNode* child);

protected:
    SGPropertyChangeListener(bool recursive = false);
    friend class SGPropertyNode;
    virtual void register_property(SGPropertyNode* node);
    virtual void unregister_property(SGPropertyNode* node);

private:
    std::vector<SGPropertyNode*> _properties;
};


struct SGPropertyNodeListeners;

/* Forward declarations for internal locking implementation. */
struct SGPropertyLock;
struct SGPropertyLockShared;
struct SGPropertyLockExclusive;

/* Forward declaration for implementation details. */
struct SGPropertyNodeImpl;

/**
 * A node in a property tree.
 */
class SGPropertyNode : public SGReferenced
{
public:

    /**
     * Access mode attributes.
     *
     * <p>The ARCHIVE attribute is strictly advisory, and controls
     * whether the property should normally be saved and restored.</p>
     */
    enum Attribute {
        NO_ATTR = 0,
        READ = 1,
        WRITE = 2,
        ARCHIVE = 4,
        REMOVED = 8,
        TRACE_READ = 16,
        TRACE_WRITE = 32,
        USERARCHIVE = 64,
        PRESERVE = 128,
        PROTECTED = 1 << 8,
        LISTENER_SAFE = 1 << 9,       // it's safe to listen to this property, even if it's tied
        VALUE_CHANGED_UP = 1 << 10,   // If true, value changes are propogated to parent's listeners.
        VALUE_CHANGED_DOWN = 1 << 11, // If true, sets new child nodes' VALUE_CHANGED_DOWN and VALUE_CHANGED_UP.
        // beware: if you add another attribute here,
        // also update value of "LAST_USED_ATTRIBUTE".
    };

    /** Last used attribute. */
    static const int LAST_USED_ATTRIBUTE;

    /** Default constructor. */
    SGPropertyNode();

    /** Copy constructor. */
    SGPropertyNode(const SGPropertyNode &node);

    /** Destructor. */
    virtual ~SGPropertyNode();

    //
    // Basic properties.
    //

    /** Test whether this node contains a primitive leaf value. */
    bool hasValue() const;

    /** Get the node's simple name as a string. Return value is not thread-safe. */
    const std::string& getNameString() const;

    /** Get the node's pretty display name, with subscript when needed. */
    std::string getDisplayName(bool simplify = false) const;

    /** Get the node's integer index. */
    int getIndex() const;

    /** Get node's position (contiguous values) in parent (*NOT* index) */
    unsigned int getPosition() const;

    /** Get a pointer to the node's parent. */
    SGPropertyNode* getParent();
    const SGPropertyNode* getParent() const;

    //
    // Children.
    //

    /** Get the number of child nodes. */
    int nChildren() const;

    /** Get a child node by position (*NOT* index). */
    SGPropertyNode* getChild(int position);
    const SGPropertyNode* getChild(int position) const;

    /** Test whether a named child exists. */
    bool hasChild(const char* name, int index = 0) const;
    bool hasChild(const std::string& name, int index = 0) const;

    /**
     * Create a new child node with the given name and an unused index
     *
     * @param min_index Minimal index for new node (skips lower indices)
     * @param append    Whether to simply use the index after the last used index
     *                  or use a lower, unused index if it exists
     */
    SGPropertyNode* addChild( const char* name, int min_index = 0, bool append = true );
    SGPropertyNode* addChild( const std::string& name, int min_index = 0, bool append = true );

    /**
     * Add existing node as child.
     *
     * @param min_index Minimal index for new node (skips lower indices)
     * @param append    Whether to simply use the index after the last used index
     *                  or use a lower, unused index if it exists
     */
    SGPropertyNode_ptr addChild(SGPropertyNode_ptr node, const std::string& name, int min_index=0, bool append=true);

    /**
     * Create multiple child nodes with the given name an unused indices
     *
     * @param count     The number of nodes create
     * @param min_index Minimal index for new nodes (skips lower indices)
     * @param append    Whether to simply use the index after the last used index
     *                  or use a lower, unused index if it exists
     */
    simgear::PropertyList addChildren( const std::string& name, size_t count, int min_index = 0, bool append = true );

    /** Get a child node by name and index. */
    SGPropertyNode* getChild(const char* name, int index = 0, bool create = false);
    SGPropertyNode* getChild(const std::string& name, int index = 0, bool create = false);
    const SGPropertyNode* getChild(const char* name, int index = 0) const;
    const SGPropertyNode* getChild(const std::string& name, int index = 0) const;

    /** Get a vector of all children with the specified name. */
    simgear::PropertyList getChildren(const char* name) const;
    simgear::PropertyList getChildren(const std::string& name) const;

    /** If <node> is child of this node, remove it and return true. Otherwise return false. */
    bool removeChild(SGPropertyNode* node);

    /** Remove child by position and name. */
    SGPropertyNode_ptr removeChild(int pos);
    SGPropertyNode_ptr removeChild(const char* name, int index = 0);
    SGPropertyNode_ptr removeChild(const std::string& name, int index = 0);

    /** Remove all children with the specified name. */
    simgear::PropertyList removeChildren(const std::string& name);

    /** Remove all children (does not change the value of the node) */
    void removeAllChildren();

    //
    // Alias support.
    //

    /** Alias this node's leaf value to another's. */
    bool alias(SGPropertyNode* target, bool withListener);

    /** Alias this node's leaf value to another's by relative path. */
    bool alias(const char* path, bool withListener);
    bool alias(const std::string& path, bool withListener);

    /** Remove any alias for this node. */
    bool unalias();

    /** Test whether the node's leaf value is aliased to another's. */
    bool isAlias() const;

    /** Get a pointer to the current alias target, if any. */
    SGPropertyNode* getAliasTarget();
    const SGPropertyNode* getAliasTarget() const;

    //
    // Path information.
    //

    /** Get the path to this node from the root. */
    std::string getPath(bool simplify = false) const;

    /** Get the original location of the node. */
    SGSourceLocation getLocation() const;
    /** Set the original location of the node. */
    void setLocation(const SGSourceLocation& location);

    /** Get a pointer to the root node. */
    SGPropertyNode* getRootNode();
    const SGPropertyNode* getRootNode() const;

    /** deep copy one node to another. */
    void copy(SGPropertyNode*to) const;

    /** Get a pointer to another node by relative path. */
    SGPropertyNode* getNode(const char* relative_path, bool create = false);
    SGPropertyNode* getNode(const std::string& relative_path, bool create = false);
    const SGPropertyNode* getNode(const char* relative_path) const;
    const SGPropertyNode* getNode(const std::string& relative_path) const;

    /**
     * Get a pointer to another node by relative path.
     *
     * This method leaves the index off the last member of the path,
     * so that the user can specify it separately (and save some
     * string building).  For example, getNode("/bar[1]/foo", 3) is
     * exactly equivalent to getNode("bar[1]/foo[3]").  The index
     * provided overrides any given in the path itself for the last
     * component.
     */
    SGPropertyNode* getNode(const char* relative_path, int index, bool create = false);
    SGPropertyNode* getNode(const std::string& relative_path, int index, bool create = false);
    const SGPropertyNode* getNode(const char* relative_path, int index) const;
    const SGPropertyNode* getNode(const std::string& relative_path, int index) const;

    //
    // Access Mode.
    //

    /** Get or set a single mode attribute for the property node. */
    bool getAttribute(Attribute attr) const;
    void setAttribute(Attribute attr, bool state);

    /** Get or set all of the mode attributes for the property node. */
    int getAttributes() const;
    void setAttributes(int attr);

    //
    // Leaf Value (primitive).
    //

    /** Get the type of leaf value, if any, for this node. */
    simgear::props::Type getType() const;

    /** Get a value of this node. We return <defaultValue> if node is not
    readable or not the requested (or comaptible) type and its string value is
    not convertible to the requested type. */
    bool getBoolValue(bool defaultValue=false) const;
    int getIntValue(int defaultValue=0) const;
    long getLongValue(long defaultValue=0) const;
    float getFloatValue(float defaultValue=0) const;
    double getDoubleValue(double defaultValue=0) const;
    std::string getStringValue() const;

    /** Set value of this node. */
    bool setBoolValue(bool value);
    bool setIntValue(int value);
    bool setLongValue(long value);
    bool setFloatValue(float value);
    bool setDoubleValue(double value);
    bool setStringValue(const char* value);
    bool setStringValue(const std::string& value);

    /** Set a value of unspecified type for this node. */
    bool setUnspecifiedValue(const std::string &value);

    //
    // Template methods for get/set value.
    //

    /**
     * Get a value from a node. If the actual type of the node doesn't
     * match the desired type, a conversion isn't guaranteed.
     */
    template<typename T>
    T getValue(typename std::enable_if<simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0) const;

    // Getter for extended property
    template<typename T>
    T getValue(typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0) const;

    /** Get a list of values from all children with the given name. */
    template<typename T, typename T_get /* = T */> // TODO use C++11 or traits
    std::vector<T> getChildValues(const std::string& name) const;

    /** Get a list of values from all children with the given name. */
    template<typename T>
    std::vector<T> getChildValues(const std::string& name) const;

    template<typename T>
    bool setValue(const T& val,
                  typename std::enable_if<simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0);

    template<typename T>
    bool setValue(const T& val,
                  typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0);

    template<int N>
    bool setValue(const char(&val)[N])
    {
        return setValue(&val[0]);
    }

    template<typename T>
    bool setValue(
            SGPropertyLockExclusive& exclusive,
            const T& val,
            typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0
            );

    template<typename T>
    bool setValue(
            SGPropertyLockExclusive& exclusive,
            const T& val,
            typename std::enable_if<simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0
            );

    /**
     * Set relative node to given value and afterwards make read only.
     *
     * @param relative_path   Path to node
     * @param value           Value to set
     *
     * @return whether value could be set
     */
    template<typename T>
    bool setValueReadOnly(const std::string& relative_path, const T& value)
    {
        SGPropertyNode* node = getNode(relative_path, true);
        bool ret = node->setValue(value);
        node->setAttributes(READ);
        return ret;
    }

#if !PROPS_STANDALONE
    /**
     * Interpolate current value to target value within given time.
     *
     * @param type        Type of interpolation ("numeric", "color", etc.)
     * @param target      Node containing target value
     * @param duration    Duration of interpolation (in seconds)
     * @param easing      Easing function (http://easings.net/)
     */
    bool interpolate( const std::string& type,
                      const SGPropertyNode& target,
                      double duration = 0.6,
                      const std::string& easing = "swing" );

    /**
     * Interpolate current value to a series of values within given durations.
     *
     * @param type        Type of interpolation ("numeric", "color", etc.)
     * @param values      Nodes containing intermediate and target values
     * @param deltas      Durations for each interpolation step (in seconds)
     * @param easing      Easing function (http://easings.net/)
     */
    bool interpolate( const std::string& type,
                      const simgear::PropertyList& values,
                      const double_list& deltas,
                      const std::string& easing = "swing" );

    /** Set the interpolation manager used by the interpolate methods. */
    static void setInterpolationMgr(simgear::PropertyInterpolationMgr* mgr);

    /** Get the interpolation manager. */
    static simgear::PropertyInterpolationMgr* getInterpolationMgr();
#endif

    /** Print the value of the property to a stream. */
    std::ostream& printOn(std::ostream& stream) const;

    //
    // Data binding.
    //

    /** Test whether this node is bound to an external data source. */
    bool isTied() const;

    /** Bind this node to an external source. */
    template<typename T>
    bool tie(const SGRawValue<T> &rawValue, bool useDefault = true);

    /** Unbind this node from any external data source. */
    bool untie();

    //
    // Convenience methods using paths.
    // TODO: add attribute methods
    //

    /** Get another node's type. */
    simgear::props::Type getType(const char* relative_path) const;
    simgear::props::Type getType(const std::string& relative_path) const;

    /** Test whether another node has a leaf value. */
    bool hasValue(const char* relative_path) const;
    bool hasValue(const std::string& relative_path) const;

    /** Get another node's value. */
    bool getBoolValue(const char* relative_path, bool defaultValue = false) const;
    int getIntValue(const char* relative_path, int defaultValue = 0) const;
    long getLongValue(const char* relative_path, long defaultValue = 0L) const;
    float getFloatValue(const char* relative_path, float defaultValue = 0.0f) const;
    double getDoubleValue(const char* relative_path, double defaultValue = 0.0) const;
    std::string getStringValue(const char* relative_path, const char* defaultValue = "") const;

    bool getBoolValue(const std::string& relative_path, bool defaultValue = false) const;
    int getIntValue(const std::string& relative_path, int defaultValue = 0) const;
    long getLongValue(const std::string& relative_path, long defaultValue = 0L) const;
    float getFloatValue(const std::string& relative_path, float defaultValue = 0.0f) const;
    double getDoubleValue(const std::string& relative_path, double defaultValue = 0.0) const;
    std::string getStringValue(const std::string& relative_path, const char* defaultValue = "") const;

    /** Set another node's value. */
    bool setBoolValue(const char* relative_path, bool value);
    bool setIntValue(const char* relative_path, int value);
    bool setLongValue(const char* relative_path, long value);
    bool setFloatValue(const char* relative_path, float value);
    bool setDoubleValue(const char* relative_path, double value);

    bool setBoolValue(const std::string& relative_path, bool value);
    bool setIntValue(const std::string& relative_path, int value);
    bool setLongValue(const std::string& relative_path, long value);
    bool setFloatValue(const std::string& relative_path, float value);
    bool setDoubleValue(const std::string& relative_path, double value);

    bool setStringValue(const char* relative_path, const char* value);
    bool setStringValue(const char* relative_path, const std::string& value);

    bool setStringValue(const std::string& relative_path, const char* value);
    bool setStringValue(const std::string& relative_path, const std::string& value);

    /** Set another node's value with no specified type. */
    bool setUnspecifiedValue(const char* relative_path, const char* value);

    /** Test whether another node is bound to an external data source. */
    bool isTied(const char* relative_path) const;
    bool isTied(const std::string& relative_path) const;

    /** Bind another node to an external source. */
    bool tie(const char* relative_path, const SGRawValue<bool> &rawValue, bool useDefault = true);
    bool tie(const char* relative_path, const SGRawValue<int> &rawValue, bool useDefault = true);
    bool tie(const char* relative_path, const SGRawValue<long> &rawValue, bool useDefault = true);
    bool tie(const char* relative_path, const SGRawValue<float> &rawValue, bool useDefault = true);
    bool tie(const char* relative_path, const SGRawValue<double> &rawValue, bool useDefault = true);
    bool tie(const char* relative_path, const SGRawValue<const char*> &rawValue, bool useDefault = true);

    bool tie(const std::string& relative_path, const SGRawValue<bool> &rawValue, bool useDefault = true);
    bool tie(const std::string& relative_path, const SGRawValue<int> &rawValue, bool useDefault = true);
    bool tie(const std::string& relative_path, const SGRawValue<long> &rawValue, bool useDefault = true);
    bool tie(const std::string& relative_path, const SGRawValue<float> &rawValue, bool useDefault = true);
    bool tie(const std::string& relative_path, const SGRawValue<double> &rawValue, bool useDefault = true);
    bool tie(const std::string& relative_path, const SGRawValue<const char*> &rawValue, bool useDefault = true);

    /** Unbind another node from any external data source. */
    bool untie(const char* relative_path);
    bool untie(const std::string& relative_path);

    /**
     * Add a change listener to the property. If "initial" is set call the
     * listener initially.
     */
    void addChangeListener(SGPropertyChangeListener* listener, bool initial = false);

    /** Remove a change listener from the property. */
    void removeChangeListener(SGPropertyChangeListener* listener);

    /** Get the number of listeners. */
    int nListeners() const;

    /** Fire a value change event to all listeners. */
    void fireValueChanged();

    /** Fire a child-added event to all listeners. */
    void fireChildAdded(SGPropertyNode* child);

    /**
     * Trigger a child-added and value-changed event for every child (Unlimited
     * depth).
     *
     * @param fire_self   Whether to trigger the events also for the node itself.
     *
     * It can be used to simulating the creation of a property tree, eg. for
     * (re)initializing a subsystem which is controlled through the property tree.
     */
    void fireCreatedRecursive(bool fire_self = false);

    /** Fire a child-removed event to all listeners. */
    void fireChildRemoved(SGPropertyNode* child);

    /**
     * Fire a child-removed event for every child of this node (Unlimited depth)
     *
     * Upon removal of a child node only for this single node a child-removed
     * event is triggered. If eg. resource cleanup relies on receiving a
     * child-removed event for every child this method can be used.
     */
    void fireChildrenRemovedRecursive();

    /** Clear any existing value and set the type to NONE. */
    void clearValue();

    /**
     * Compare two property trees. The property trees are equal if: 1)
     * They have no children, and have the same type and the values are
     * equal, or 2) have the same number of children, and the
     * corresponding children in each tree are equal. "corresponding"
     * means have the same name and index.
     *
     * Attributes, removed children, and aliases aren't considered.
     */
    static bool compare(const SGPropertyNode& lhs, const SGPropertyNode& rhs);

protected:

    /* fire*() generally need to temporarily modify _listeners->_num_iterators
    so take exclusive lock rather than shared. */

    /** Protected constructor for making new nodes on demand. */
    SGPropertyNode(const std::string& name, int index, SGPropertyNode* parent);

    template<typename Itr>
    SGPropertyNode(Itr begin, Itr end, int index, SGPropertyNode* parent);

    static simgear::PropertyInterpolationMgr* _interpolation_mgr;

private:

    // very internal path parsing function
    template<typename SplitItr>
    friend SGPropertyNode* find_node_aux(SGPropertyNode* current, SplitItr& itr, bool create, int last_index);
    
    // Template-style getValue when lock is already held.
    template<typename T>
    T getValue(
            SGPropertyLock& lock,
            typename std::enable_if<simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0
            ) const;
    
    // For SGVec3d and SGVec4d.
    template<typename T>
    T getValue(
            SGPropertyLock& lock,
            typename std::enable_if<!simgear::props::PropertyTraits<T>::Internal>::type* dummy = 0
            ) const;

    // For boost
    friend size_t hash_value(const SGPropertyNode& node);

    // Allow implementation code access to _mutex.
    //
    friend SGPropertyLock;
    friend SGPropertyLockShared;
    friend SGPropertyLockExclusive;
    
    // Misc implementation access.
    friend SGPropertyNodeImpl;

    // Class data.
    //
    
    // Support for thread-safety.
    //
    mutable std::shared_mutex _mutex;
    
    // Core data.
    //
    int _index;
    std::string _name;
    SGPropertyNode* _parent;
    simgear::PropertyList _children;
    simgear::props::Type _type;
    bool _tied;
    int _attr = NO_ATTR;

    /**
     * @brief when a property is an alias, we record information about it in
     * this struct
     * 
     */
    struct AliasData {
        AliasData(SGPropertyNode* t) : target(t) {}

        SGPropertyNode_ptr target;
        SGPropertyChangeListener* listener = nullptr;
    };

    // The right kind of pointer...
    union {
        AliasData* alias;
        SGRaw* val;
    } _value;

    union {
        bool bool_val;
        int int_val;
        long long_val;
        float float_val;
        double double_val;
        char* string_val;
    } _local_val;

    SGPropertyNodeListeners*  _listeners;
};

// Convenience functions for use in templates
template<typename T>
#if PROPS_STANDALONE
T
#else
typename std::enable_if<!std::is_enum<T>::value, T>::type
#endif
getValue(const SGPropertyNode*);

template<>
inline bool getValue<bool>(const SGPropertyNode* node)
{
    return node->getBoolValue();
}

template<>
inline int getValue<int>(const SGPropertyNode* node)
{
    return node->getIntValue();
}

template<>
inline long getValue<long>(const SGPropertyNode* node)
{
    return node->getLongValue();
}

template<>
inline float getValue<float>(const SGPropertyNode* node)
{
    return node->getFloatValue();
}

template<>
inline double getValue<double>(const SGPropertyNode* node)
{
    return node->getDoubleValue();
}

template<>
inline std::string getValue<std::string>(const SGPropertyNode* node)
{
    return node->getStringValue();
}

namespace simgear
{
  /**
   * Default trait for extracting enum values from SGPropertyNode. Create your
   * own specialization for specific enum types to enable validation of values.
   */
  template<class T>
  struct enum_traits
  {
    /**
     * Typename of the enum
     */
    static const char* name() { return typeid(T).name(); }

    /**
     * @return Default value (will be used if validation fails)
     */
    static T defVal() { return T(); }

    /**
     * @return Whether the given integer value has an enum value defined
     */
    static bool validate(int) { return true; }
  };
} // namespace simgear

/** Extract enum from SGPropertyNode. */
template<typename T>
#if PROPS_STANDALONE
inline T
#else
inline typename std::enable_if<std::is_enum<T>::value, T>::type
#endif
getValue(const SGPropertyNode* node)
{
  typedef simgear::enum_traits<T> Traits;
  int val = node->getIntValue();
  if( !Traits::validate(val) )
  {
    SG_LOG
    (
      SG_GENERAL,
      SG_WARN,
      "Invalid value for enum (" << Traits::name() << ", val = " << val << ")"
    );
    return Traits::defVal();
  }
  return static_cast<T>(node->getIntValue());
}

inline bool setValue(SGPropertyNode* node, bool value)
{
    return node->setBoolValue(value);
}

inline bool setValue(SGPropertyNode* node, int value)
{
    return node->setIntValue(value);
}

inline bool setValue(SGPropertyNode* node, long value)
{
    return node->setLongValue(value);
}

inline bool setValue(SGPropertyNode* node, float value)
{
    return node->setFloatValue(value);
}

inline bool setValue(SGPropertyNode* node, double value)
{
    return node->setDoubleValue(value);
}

inline bool setValue(SGPropertyNode* node, const char* value)
{
    return node->setStringValue(value);
}

inline bool setValue(SGPropertyNode* node, const std::string& value)
{
    return node->setStringValue(value.c_str());
}

/** Utility function for creation of a child property node. */
inline SGPropertyNode* makeChild(SGPropertyNode* parent, const char* name, int index = 0)
{
    return parent->getChild(name, index, true);
}

namespace simgear
{
  /**
   * Utility function for creation of a child property node using a
   * relative path.
   */
  template<typename StringType>
  inline SGPropertyNode*
  makeNode(SGPropertyNode* parent, const StringType& name)
  {
      return parent->getNode(name, true);
  }
}

// For boost::hash
size_t hash_value(const SGPropertyNode& node);

// Helper comparison and hash functions for common cases

namespace simgear
{
namespace props
{
struct Compare
{
    bool operator()(const SGPropertyNode* lhs, const SGPropertyNode* rhs) const
    {
        return SGPropertyNode::compare(*lhs, *rhs);
    }
    bool operator()(SGPropertyNode_ptr lhs, const SGPropertyNode* rhs) const
    {
        return SGPropertyNode::compare(*lhs, *rhs);
    }
    bool operator()(const SGPropertyNode* lhs, SGPropertyNode_ptr rhs) const
    {
        return SGPropertyNode::compare(*lhs, *rhs);
    }
    bool operator()(SGPropertyNode_ptr lhs, SGPropertyNode_ptr rhs) const
    {
        return SGPropertyNode::compare(*lhs, *rhs);
    }
};

struct Hash
{
    size_t operator()(const SGPropertyNode* node) const
    {
        return hash_value(*node);
    }
    size_t operator()(SGPropertyNode_ptr node) const
    {
        return hash_value(*node);
    }
};
} // namespace props
} // namespace simgear

/** Convenience class for change listener callbacks without
 * creating a derived class implementing a "valueChanged" method.
 * Also removes listener on destruction automatically.
 */
template<class T>
class SGPropertyChangeCallback
    : public SGPropertyChangeListener
{
public:
    SGPropertyChangeCallback(T* obj,
                             void (T::*method)(SGPropertyNode*),
                             SGPropertyNode_ptr property,
                             bool initial=false
                             )
        : _obj(obj),
        _callback(method),
        _property(property)
    {
        _property->addChangeListener(this,initial);
    }

    SGPropertyChangeCallback(const SGPropertyChangeCallback<T>& other)
        : SGPropertyChangeListener(other),
        _obj(other._obj),
        _callback(other._callback),
        _property(other._property)
    {
        _property->addChangeListener(this,false);
    }

    virtual ~SGPropertyChangeCallback()
    {
        _property->removeChangeListener(this);
    }
    
    void valueChanged(SGPropertyNode* node)
    {
        (_obj->*_callback)(node);
    }
    
private:
    T* _obj;
    void (T::*_callback)(SGPropertyNode*);
    SGPropertyNode_ptr _property;
};


#endif // SG_PROPS_UNTHREADSAFE

// Sets property nodes that control property locking.
//
// active: whether we use locking.
// verbose: whether we detect and report on lock contention.
// timing: whether we gather timing information (compiled-out by default).
// parent_listeners: whether to call parent nodes' listeners when property values change.
//
void SGPropertyLockControl(
        SGPropertyNode* active,
        SGPropertyNode* verbose,
        SGPropertyNode* timing,
        SGPropertyNode* parent_listeners
        );

#endif // __PROPS_HXX
