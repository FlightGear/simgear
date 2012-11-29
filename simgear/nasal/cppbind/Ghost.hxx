///@file
/// Expose C++ objects to Nasal as ghosts
///
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SG_NASAL_GHOST_HXX_
#define SG_NASAL_GHOST_HXX_

#include "from_nasal.hxx"
#include "to_nasal.hxx"

#include <simgear/debug/logstream.hxx>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/utility/enable_if.hpp>

#include <map>

/**
 * Bindings between C++ and the Nasal scripting language
 */
namespace nasal
{

  /**
   * Traits for C++ classes exposed as Ghost. This is the basic template for
   * raw types.
   */
  template<class T>
  struct GhostTypeTraits
  {
    /** Whether the class is passed by shared pointer or raw pointer */
    typedef boost::false_type::type is_shared;

    /** The raw class type (Without any shared pointer) */
    typedef T raw_type;
  };

  template<class T>
  struct GhostTypeTraits<boost::shared_ptr<T> >
  {
    typedef boost::true_type::type is_shared;
    typedef T raw_type;
  };

#ifdef OSG_REF_PTR
  template<class T>
  struct GhostTypeTraits<osg::ref_ptr<T> >
  {
    typedef boost::true_type::type is_shared;
    typedef T raw_type;
  };
#endif

#ifdef SGSharedPtr_HXX
  template<class T>
  struct GhostTypeTraits<SGSharedPtr<T> >
  {
    typedef boost::true_type::type is_shared;
    typedef T raw_type;
  };
#endif

  /**
   * Policy for creating ghost instances from shared pointer objects.
   */
  template<class T>
  struct SharedPointerPolicy
  {
    typedef typename GhostTypeTraits<T>::raw_type   raw_type;
    typedef T                                       pointer;
    typedef boost::false_type                       returns_dynamic_type;

    /**
     * Create a shared pointer on the heap to handle the reference counting for
     * the passed shared pointer while it is used in Nasal space.
     */
    static T* createInstance(const T& ptr)
    {
      return new T(ptr);
    }

    static pointer getPtr(void* ptr)
    {
      if( ptr )
        return *static_cast<T*>(ptr);
      else
        return pointer();
    }

    static raw_type* getRawPtr(void* ptr)
    {
      if( ptr )
        return static_cast<T*>(ptr)->get();
      else
        return 0;
    }

    static raw_type* getRawPtr(const T& ptr)
    {
      return ptr.get();
    }
  };

  /**
   * Policy for creating ghost instances as raw objects on the heap.
   */
  template<class T>
  struct RawPointerPolicy
  {
    typedef typename GhostTypeTraits<T>::raw_type   raw_type;
    typedef raw_type*                               pointer;
    typedef boost::true_type                        returns_dynamic_type;

    /**
     * Create a new object instance on the heap
     */
    static T* createInstance()
    {
      return new T();
    }

    static pointer getPtr(void* ptr)
    {
      BOOST_STATIC_ASSERT((boost::is_same<pointer, T*>::value));
      return static_cast<T*>(ptr);
    }

    static raw_type* getRawPtr(void* ptr)
    {
      BOOST_STATIC_ASSERT((boost::is_same<raw_type, T>::value));
      return static_cast<T*>(ptr);
    }
  };

  namespace internal
  {
    /**
     * Metadata for Ghost object types
     */
    class GhostMetadata
    {
      public:
        /**
         * Add a nasal base class to the ghost. Will be available in the ghosts
         * parents array.
         */
        void addNasalBase(const naRef& parent)
        {
          assert( naIsHash(parent) );
          _parents.push_back(parent);
        }

        bool isBaseOf(naGhostType* ghost_type) const
        {
          if( ghost_type == &_ghost_type )
            return true;

          for( DerivedList::const_iterator derived = _derived_classes.begin();
                                           derived != _derived_classes.end();
                                         ++derived )
          {
            if( (*derived)->isBaseOf(ghost_type) )
              return true;
          }

          return false;
        }

      protected:

        typedef std::vector<const GhostMetadata*> DerivedList;

        const std::string   _name;
        naGhostType         _ghost_type;
        DerivedList         _derived_classes;
        std::vector<naRef>  _parents;

        explicit GhostMetadata(const std::string& name):
          _name(name)
        {

        }

        void addDerived(const GhostMetadata* derived)
        {
          assert(derived);
          _derived_classes.push_back(derived);

          SG_LOG
          (
            SG_NASAL,
            SG_INFO,
            "Ghost::addDerived: " <<_ghost_type.name << " -> " << derived->_name
          );
        }

        naRef getParents(naContext c)
        {
          return nasal::to_nasal(c, _parents);
        }
    };
  }

  /**
   * Context passed to a function/method being called from Nasal
   */
  struct CallContext
  {
    CallContext(naContext c, size_t argc, naRef* args):
      c(c),
      argc(argc),
      args(args)
    {}

    /**
     * Get the argument with given index if it exists. Otherwise returns the
     * passed default value.
     *
     * @tparam T    Type of argument (converted using ::from_nasal)
     * @param index Index of requested argument
     * @param def   Default value returned if too few arguments available
     */
    template<class T>
    T getArg(size_t index, const T& def = T()) const
    {
      if( index >= argc )
        return def;

      return from_nasal<T>(c, args[index]);
    }

    /**
     * Get the argument with given index. Raises a Nasal runtime error if there
     * are to few arguments available.
     */
    template<class T>
    T requireArg(size_t index) const
    {
      if( index >= argc )
        naRuntimeError(c, "Missing required arg #%d", index);

      return from_nasal<T>(c, args[index]);
    }

    naContext   c;
    size_t      argc;
    naRef      *args;
  };

  /**
   * Class for exposing C++ objects to Nasal
   *
   * @code{cpp}
   * // Example class to be exposed to Nasal
   * class MyClass
   * {
   *   public:
   *     void setX(int x);
   *     int getX() const;
   *
   *     naRef myMember(naContext c, int argc, naRef* args);
   * }
   *
   * void exposeClasses()
   * {
   *   // Register a nasal ghost type for MyClass. This needs to be done only
   *   // once before creating the first ghost instance.
   *   Ghost<MyClass>::init("MyClass")
   *     // Members can be exposed by getters and setters
   *     .member("x", &MyClass::getX, &MyClass::setX)
   *     // For readonly variables only pass a getter
   *     .member("x_readonly", &MyClass::getX)
   *     // It is also possible to expose writeonly members
   *     .member("x_writeonly", &MyClass::setX)
   *     // Methods use a slightly different syntax - The pointer to the member
   *     // function has to be passed as template argument
   *     .method<&MyClass::myMember>("myMember");
   * }
   * @endcode
   */
  template<class T>
  class Ghost:
    public internal::GhostMetadata,
    protected boost::mpl::if_< typename GhostTypeTraits<T>::is_shared,
                               SharedPointerPolicy<T>,
                               RawPointerPolicy<T> >::type
  {
    public:
      typedef T                                                   value_type;
      typedef typename GhostTypeTraits<T>::raw_type               raw_type;
      typedef typename Ghost::pointer                             pointer;
      typedef naRef (raw_type::*member_func_t)(const CallContext&);
      typedef naRef (*free_func_t)(raw_type&, const CallContext&);
      typedef boost::function<naRef(naContext, raw_type&)>        getter_t;
      typedef boost::function<void(naContext, raw_type&, naRef)>  setter_t;

      /**
       * A ghost member. Can consist either of getter and/or setter functions
       * for exposing a data variable or a single callable function.
       */
      struct member_t
      {
        member_t():
          func(0)
        {}

        member_t( const getter_t& getter,
                  const setter_t& setter,
                  naCFunction func = 0 ):
          getter( getter ),
          setter( setter ),
          func( func )
        {}

        member_t(naCFunction func):
          func( func )
        {}

        getter_t    getter;
        setter_t    setter;
        naCFunction func;
      };

      typedef std::map<std::string, member_t> MemberMap;

      /**
       * Register a new ghost type.
       *
       * @note Only intialize each ghost type once!
       *
       * @param name    Descriptive name of the ghost type.
       */
      static Ghost& init(const std::string& name)
      {
        assert( !getSingletonPtr() );

        getSingletonHolder().reset( new Ghost(name) );
        return *getSingletonPtr();
      }

      /**
       * Register a base class for this ghost. The base class needs to be
       * registers on its own before it can be used as base class.
       *
       * @tparam BaseGhost  Type of base class already wrapped into Ghost class
       *                    (Ghost<Base>)
       *
       * @code{cpp}
       * Ghost<MyBase>::init("MyBase");
       * Ghost<MyClass>::init("MyClass")
       *   .bases<Ghost<MyBase> >();
       * @endcode
       */
      template<class BaseGhost>
      typename boost::enable_if_c
        <    boost::is_base_of<GhostMetadata, BaseGhost>::value
          && boost::is_base_of<typename BaseGhost::raw_type, raw_type>::value,
          Ghost
        >::type&
      bases()
      {
        BaseGhost* base = BaseGhost::getSingletonPtr();
        base->addDerived
        (
          this,
          // Both ways of retrieving the address of a static member function
          // should be legal but not all compilers know this.
          // g++-4.4.7+ has been tested to work with both versions
#if defined(GCC_VERSION) && GCC_VERSION < 40407
          // The old version of g++ used on Jenkins (16.11.2012) only compiles
          // this version.
          &getTypeFor<BaseGhost>
#else
          // VS (2008, 2010, ... ?) only allow this version.
          &Ghost::getTypeFor<BaseGhost>
#endif
        );

        // Replace any getter that is not available in the current class.
        // TODO check if this is the correct behavior of function overriding
        for( typename BaseGhost::MemberMap::const_iterator member =
               base->_members.begin();
               member != base->_members.end();
             ++member )
        {
          if( _members.find(member->first) == _members.end() )
            _members[member->first] = member_t
            (
              member->second.getter,
              member->second.setter,
              member->second.func
            );
        }

        return *this;
      }

      /**
       * Register a base class for this ghost. The base class needs to be
       * registers on its own before it can be used as base class.
       *
       * @tparam Base   Type of base class (Base as used in Ghost<Base>)
       *
       * @code{cpp}
       * Ghost<MyBase>::init("MyBase");
       * Ghost<MyClass>::init("MyClass")
       *   .bases<MyBase>();
       * @endcode
       */
      template<class Base>
      typename boost::enable_if_c
        <   !boost::is_base_of<GhostMetadata, Base>::value
          && boost::is_base_of<typename Ghost<Base>::raw_type, raw_type>::value,
          Ghost
        >::type&
      bases()
      {
        return bases< Ghost<Base> >();
      }

      /**
       * Register an existing Nasal class/hash as base class for this ghost.
       *
       * @param parent  Nasal hash/class
       */
      Ghost& bases(const naRef& parent)
      {
        addNasalBase(parent);
        return *this;
      }

      /**
       * Register a member variable by passing a getter and/or setter method.
       *
       * @param field   Name of member
       * @param getter  Getter for variable
       * @param setter  Setter for variable (Pass 0 to prevent write access)
       *
       */
      template<class Var>
      Ghost& member( const std::string& field,
                     Var (raw_type::*getter)() const,
                     void (raw_type::*setter)(Var) = 0 )
      {
        member_t m;
        if( getter )
        {
          naRef (*to_nasal_)(naContext, Var) = &nasal::to_nasal;

          // Getter signature: naRef(naContext, raw_type&)
          m.getter = boost::bind(to_nasal_, _1, boost::bind(getter, _2));
        }

        if( setter )
        {
          Var (*from_nasal_)(naContext, naRef) = &nasal::from_nasal;

          // Setter signature: void(naContext, raw_type&, naRef)
          m.setter = boost::bind(setter, _2, boost::bind(from_nasal_, _1, _3));
        }

        return member(field, m.getter, m.setter);
      }

      /**
       * Register a write only member variable.
       *
       * @param field   Name of member
       * @param setter  Setter for variable
       */
      template<class Var>
      Ghost& member( const std::string& field,
                     void (raw_type::*setter)(Var) )
      {
        return member<Var>(field, 0, setter);
      }

      /**
       * Register a member variable by passing a getter and/or setter method.
       *
       * @param field   Name of member
       * @param getter  Getter for variable
       * @param setter  Setter for variable (Pass empty to prevent write access)
       *
       */
      Ghost& member( const std::string& field,
                     const getter_t& getter,
                     const setter_t& setter = setter_t() )
      {
        if( !getter.empty() || !setter.empty() )
          _members[field] = member_t(getter, setter);
        else
          SG_LOG
          (
            SG_NASAL,
            SG_WARN,
            "Member '" << field << "' requires a getter or setter"
          );
        return *this;
      }

      /**
       * Register a member function.
       *
       * @note Because only function pointers can be registered as Nasal
       *       functions it is needed to pass the function pointer as template
       *       argument. This allows us to create a separate instance of the
       *       MemberFunctionWrapper for each registered function and therefore
       *       provides us with the needed static functions to be passed on to
       *       Nasal.
       *
       * @tparam func   Pointer to member function being registered.
       *
       * @code{cpp}
       * class MyClass
       * {
       *   public:
       *     naRef myMethod(naContext c, int argc, naRef* args);
       * }
       *
       * Ghost<MyClass>::init("Test")
       *   .method<&MyClass::myMethod>("myMethod");
       * @endcode
       */
      template<member_func_t func>
      Ghost& method(const std::string& name)
      {
        _members[name].func = &MemberFunctionWrapper<func>::call;
        return *this;
      }

      /**
       * Register a free function as member function. The object instance is
       * passed as additional first argument.
       *
       * @tparam func   Pointer to free function being registered.
       *
       * @note Due to a severe bug in Visual Studio it is not possible to create
       *       a specialization of #method for free function pointers and
       *       member function pointers at the same time. Do overcome this
       *       limitation we had to use a different name for this function.
       *
       * @code{cpp}
       * class MyClass;
       * naRef myMethod(MyClass& obj, naContext c, int argc, naRef* args);
       *
       * Ghost<MyClass>::init("Test")
       *   .method_func<&myMethod>("myMethod");
       * @endcode
       */
      template<free_func_t func>
      Ghost& method_func(const std::string& name)
      {
        _members[name].func = &FreeFunctionWrapper<func>::call;
        return *this;
      }

      // TODO use variadic template when supporting C++11
      /**
       * Create a Nasal instance of this ghost.
       *
       * @param c   Active Nasal context
       */
      static naRef create( naContext c )
      {
        return makeGhost(c, Ghost::createInstance());
      }

      /**
       * Create a Nasal instance of this ghost.
       *
       * @param c   Active Nasal context
       * @param a1  Parameter used for creating new instance
       */
      template<class A1>
      static naRef create( naContext c, const A1& a1 )
      {
        return makeGhost(c, Ghost::createInstance(a1));
      }

      /**
       * Nasal callback for creating a new instance of this ghost.
       */
      static naRef f_create(naContext c, naRef me, int argc, naRef* args)
      {
        return create(c);
      }

      static bool isBaseOf(naGhostType* ghost_type)
      {
        if( !ghost_type )
          return false;

        return getSingletonPtr()->GhostMetadata::isBaseOf(ghost_type);
      }

      static bool isBaseOf(naRef obj)
      {
        return isBaseOf( naGhost_type(obj) );
      }

      /**
       * Convert Nasal object to C++ object. To get a valid object the passed
       * Nasal objects has to be derived class of the target class (Either
       * derived in C++ or in Nasal using a 'parents' vector)
       */
      static pointer fromNasal(naContext c, naRef me)
      {
        // Check if it's a ghost and if it can be converted
        if( isBaseOf( naGhost_type(me) ) )
          return Ghost::getPtr( naGhost_ptr(me) );

        // Now if it is derived from a ghost (hash with ghost in parent vector)
        // TODO handle recursive parents
        else if( naIsHash(me) )
        {
          naRef na_parents = naHash_cget(me, const_cast<char*>("parents"));
          if( !naIsVector(na_parents) )
          {
            SG_LOG(SG_NASAL, SG_DEBUG, "Missing 'parents' vector for ghost");
            return pointer();
          }

          typedef std::vector<naRef> naRefs;
          naRefs parents = from_nasal<naRefs>(c, na_parents);
          for( naRefs::const_iterator parent = parents.begin();
                                      parent != parents.end();
                                    ++parent )
          {
            if( isBaseOf(naGhost_type(*parent)) )
              return Ghost::getPtr( naGhost_ptr(*parent) );
          }
        }

        return pointer();
      }

    private:

      template<class>
      friend class Ghost;

      typedef naGhostType* (*type_checker_t)(const raw_type*);
      typedef std::vector<type_checker_t> DerivedList;
      DerivedList _derived_types;

      void addDerived( const internal::GhostMetadata* derived_meta,
                       const type_checker_t& derived_info )
      {
        GhostMetadata::addDerived(derived_meta);
        _derived_types.push_back(derived_info);
      }

      template<class BaseGhost>
      static
      typename boost::enable_if
        < boost::is_polymorphic<typename BaseGhost::raw_type>,
          naGhostType*
        >::type
      getTypeFor(const typename BaseGhost::raw_type* base)
      {
        // Check first if passed pointer can by converted to instance of class
        // this ghost wraps.
        if(   !boost::is_same
                 < typename BaseGhost::raw_type,
                   typename Ghost::raw_type
                 >::value
            && dynamic_cast<const typename Ghost::raw_type*>(base) != base )
          return 0;

        // Now check if we can further downcast to one of our derived classes.
        for( typename DerivedList::reverse_iterator
               derived = getSingletonPtr()->_derived_types.rbegin();
               derived != getSingletonPtr()->_derived_types.rend();
             ++derived )
        {
          naGhostType* ghost_type =
            (*derived)( static_cast<const typename Ghost::raw_type*>(base) );
          if( ghost_type )
            return ghost_type;
        }

        // If base is not an instance of any derived class, this class has to
        // be the dynamic type.
        return &getSingletonPtr()->_ghost_type;
      }

      template<class BaseGhost>
      static
      typename boost::disable_if
        < boost::is_polymorphic<typename BaseGhost::raw_type>,
          naGhostType*
        >::type
      getTypeFor(const typename BaseGhost::raw_type* base)
      {
        // For non polymorphic classes there is no possibility to get the actual
        // dynamic type, therefore we can only use its static type.
        return &BaseGhost::getSingletonPtr()->_ghost_type;
      }

      static Ghost* getSingletonPtr()
      {
        return getSingletonHolder().get();
      }

      static raw_type& requireObject(naContext c, naRef me)
      {
        raw_type* obj = Ghost::getRawPtr( fromNasal(c, me) );
        naGhostType* ghost_type = naGhost_type(me);

        if( !obj )
          naRuntimeError
          (
            c,
            "method called on object of wrong type: is '%s' expected '%s'",
            ghost_type ? ghost_type->name : "unknown",
            getSingletonPtr()->_ghost_type.name
          );

        return *obj;
      }

      /**
       * Wrapper class to enable registering pointers to member functions as
       * Nasal function callbacks. We need to use the function pointer as
       * template parameter to ensure every registered function gets a static
       * function which can be passed to Nasal.
       */
      template<member_func_t func>
      struct MemberFunctionWrapper
      {
        /**
         * Called from Nasal upon invocation of the according registered
         * function. Forwards the call to the passed object instance.
         */
        static naRef call(naContext c, naRef me, int argc, naRef* args)
        {
          return (requireObject(c, me).*func)(CallContext(c, argc, args));
        }
      };

      /**
       * Wrapper class to enable registering pointers to free functions (only
       * external linkage). We need to use the function pointer as template
       * parameter to ensure every registered function gets a static function
       * which can be passed to Nasal. Even though we just wrap another simple
       * function pointer this intermediate step is need to be able to retrieve
       * the object the function call belongs to and pass it along as argument.
       */
      template<free_func_t func>
      struct FreeFunctionWrapper
      {
        /**
         * Called from Nasal upon invocation of the according registered
         * function. Forwards the call to the passed function pointer and passes
         * the required parameters.
         */
        static naRef call(naContext c, naRef me, int argc, naRef* args)
        {
          return func(requireObject(c, me), CallContext(c, argc, args));
        }
      };

      typedef std::auto_ptr<Ghost> GhostPtr;
      MemberMap _members;

      explicit Ghost(const std::string& name):
        GhostMetadata( name )
      {
        _ghost_type.destroy = &destroyGhost;
        _ghost_type.name = _name.c_str();
        _ghost_type.get_member = &getMember;
        _ghost_type.set_member = &setMember;
      }

      static GhostPtr& getSingletonHolder()
      {
        static GhostPtr instance;
        return instance;
      }

      static naRef makeGhost(naContext c, void *ptr)
      {
        if( !Ghost::getRawPtr(ptr) )
          return naNil();

        naGhostType* ghost_type = 0;
        if( Ghost::returns_dynamic_type::value )
          // For pointer policies already returning instances of an object with
          // the dynamic type of this Ghost's raw_type the type is always the
          // same.
          ghost_type = &getSingletonPtr()->_ghost_type;
        else
          // If wrapping eg. shared pointers the users passes an already
          // existing instance of an object which will then be hold be a new
          // shared pointer. We therefore have to check for the dynamic type
          // of the object as it might differ from the passed static type.
          ghost_type = getTypeFor<Ghost>( Ghost::getRawPtr(ptr) );

        if( !ghost_type )
          return naNil();

        return naNewGhost2(c, ghost_type, ptr);
      }

      static void destroyGhost(void *ptr)
      {
        delete (T*)ptr;
      }

      /**
       * Callback for retrieving a ghost member.
       */
      static const char* getMember(naContext c, void* g, naRef key, naRef* out)
      {
        const std::string key_str = nasal::from_nasal<std::string>(c, key);
        if( key_str == "parents" )
        {
          if( getSingletonPtr()->_parents.empty() )
            return 0;

          *out = getSingletonPtr()->getParents(c);
          return "";
        }

        typename MemberMap::iterator member =
          getSingletonPtr()->_members.find(key_str);

        if( member == getSingletonPtr()->_members.end() )
          return 0;

        if( member->second.func )
          *out = nasal::to_nasal(c, member->second.func);
        else if( !member->second.getter.empty() )
          *out = member->second.getter(c, *Ghost::getRawPtr(g));
        else
          return "Read-protected member";

        return "";
      }

      /**
       * Callback for writing to a ghost member.
       */
      static void setMember(naContext c, void* g, naRef field, naRef val)
      {
        const std::string key = nasal::from_nasal<std::string>(c, field);
        typename MemberMap::iterator member =
          getSingletonPtr()->_members.find(key);

        if( member == getSingletonPtr()->_members.end() )
          naRuntimeError(c, "ghost: No such member: %s", key.c_str());
        if( member->second.setter.empty() )
          naRuntimeError(c, "ghost: Write protected member: %s", key.c_str());

        member->second.setter(c, *Ghost::getRawPtr(g), val);
      }
  };

} // namespace nasal

#endif /* SG_NASAL_GHOST_HXX_ */
