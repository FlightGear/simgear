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

#include "NasalCallContext.hxx"
#include "NasalObjectHolder.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>
#include <simgear/structure/SGWeakPtr.hxx>

#include <boost/bind.hpp>
#include <boost/call_traits.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>

#include <map>

template<class T>
inline T* get_pointer(boost::weak_ptr<T> const& p)
{
  return p.lock().get();
}

template<class T>
inline T* get_pointer(SGWeakPtr<T> const& p)
{
  return p.lock().get();
}

/**
 * Bindings between C++ and the Nasal scripting language
 */
namespace nasal
{

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
          if( ghost_type == _ghost_type_ptr )
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
        const naGhostType  *_ghost_type_ptr;
        DerivedList         _derived_classes;
        std::vector<naRef>  _parents;

        GhostMetadata( const std::string& name,
                       const naGhostType* ghost_type ):
          _name(name),
          _ghost_type_ptr(ghost_type)
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
            "Ghost::addDerived: " << _name << " -> " << derived->_name
          );
        }

        naRef getParents(naContext c)
        {
          return nasal::to_nasal(c, _parents);
        }
    };

    /**
     * Hold callable method and convert to Nasal function if required.
     */
    class MethodHolder:
      public SGWeakReferenced
    {
      public:
        virtual ~MethodHolder() {}

        naRef get_naRef(naContext c)
        {
          if( !_obj.valid() )
            _obj.reset(createNasalObject(c));
          return _obj.get_naRef();
        }

      protected:
        ObjectHolder<> _obj;

        virtual naRef createNasalObject(naContext c) = 0;
    };

    BOOST_MPL_HAS_XXX_TRAIT_DEF(element_type)

    template<class T>
    struct reduced_type
    {
      typedef typename boost::remove_cv<
        typename boost::remove_reference<T>::type
      >::type type;
    };

    template<class T1, class T2>
    struct reduced_is_same:
      public boost::is_same<typename reduced_type<T1>::type, T2>
    {};
  }

  typedef SGSharedPtr<internal::MethodHolder> MethodHolderPtr;
  typedef SGWeakPtr<internal::MethodHolder> MethodHolderWeakPtr;

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
   *     int myMember();
   *     void doSomethingElse(const nasal::CallContext& ctx);
   * }
   * typedef boost::shared_ptr<MyClass> MyClassPtr;
   *
   * std::string myOtherFreeMember(int num);
   *
   * void exposeClasses()
   * {
   *   // Register a nasal ghost type for MyClass. This needs to be done only
   *   // once before creating the first ghost instance. The exposed class needs
   *   // to be wrapped inside a shared pointer, eg. boost::shared_ptr.
   *   Ghost<MyClassPtr>::init("MyClass")
   *     // Members can be exposed by getters and setters
   *     .member("x", &MyClass::getX, &MyClass::setX)
   *     // For readonly variables only pass a getter
   *     .member("x_readonly", &MyClass::getX)
   *     // It is also possible to expose writeonly members
   *     .member("x_writeonly", &MyClass::setX)
   *     // Methods can be nearly anything callable and accepting a reference
   *     // to an instance of the class type. (member functions, free functions
   *     // and anything else bindable using boost::function and boost::bind)
   *     .method("myMember", &MyClass::myMember)
   *     .method("doSomething", &MyClass::doSomethingElse)
   *     .method("other", &myOtherFreeMember);
   * }
   * @endcode
   */
  template<class T>
  class Ghost:
    public internal::GhostMetadata
  {
      BOOST_STATIC_ASSERT( internal::has_element_type<T>::value );

    public:
      typedef typename T::element_type                              raw_type;
      typedef T                                                     pointer;
      typedef naRef (raw_type::*member_func_t)(const CallContext&);
      typedef naRef (*free_func_t)(raw_type&, const CallContext&);
      typedef boost::function<naRef(naContext, raw_type&)>          getter_t;
      typedef boost::function<void(naContext, raw_type&, naRef)>    setter_t;
      typedef boost::function<naRef(raw_type&, const CallContext&)> method_t;
      typedef boost::function<bool( naContext,
                                    raw_type&,
                                    const std::string&,
                                    naRef )>               fallback_setter_t;

      class MethodHolder:
        public internal::MethodHolder
      {
        public:
          explicit MethodHolder(const method_t& method):
            _method(method)
          {}

        protected:

          typedef SGSharedPtr<MethodHolder> SharedPtr;
          typedef SGWeakPtr<MethodHolder> WeakPtr;

          method_t  _method;

          virtual naRef createNasalObject(naContext c)
          {
            return naNewFunc
            (
              c,
              naNewCCodeUD( c,
                            &MethodHolder::call,
                            new WeakPtr(this),
                            &destroyHolder )
            );
          }

          static void destroyHolder(void* user_data)
          {
            delete static_cast<WeakPtr*>(user_data);
          }

          static naRef call( naContext c,
                             naRef me,
                             int argc,
                             naRef* args,
                             void* user_data )
          {
            WeakPtr* holder_weak = static_cast<WeakPtr*>(user_data);
            if( !holder_weak )
              naRuntimeError(c, "invalid method holder!");

            try
            {
              SharedPtr holder = holder_weak->lock();
              if( !holder )
                throw std::runtime_error("holder has expired");

              return holder->_method
              (
                requireObject(c, me),
                CallContext(c, argc, args)
              );
            }
            catch(const std::exception& ex)
            {
              naRuntimeError(c, "Fatal error in method call: %s", ex.what());
            }
            catch(...)
            {
              naRuntimeError(c, "Unknown exception in method call.");
            }

            return naNil();
          }
      };

      /**
       * A ghost member. Can consist either of getter and/or setter functions
       * for exposing a data variable or a single callable function.
       */
      struct member_t
      {
        member_t()
        {}

        member_t( const getter_t& getter,
                  const setter_t& setter,
                  const MethodHolderPtr& func = MethodHolderPtr() ):
          getter( getter ),
          setter( setter ),
          func( func )
        {}

        explicit member_t(const MethodHolderPtr& func):
          func( func )
        {}

        getter_t        getter;
        setter_t        setter;
        MethodHolderPtr func;
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
        getSingletonHolder().reset( new Ghost(name) );
        return *getSingletonPtr();
      }

      /**
       * Check whether ghost type has already been initialized.
       */
      static bool isInit()
      {
        return getSingletonPtr();
      }

      /**
       * Register a base class for this ghost. The base class needs to be
       * registers on its own before it can be used as base class.
       *
       * @tparam BaseGhost  Type of base class already wrapped into Ghost class
       *                    (Ghost<BasePtr>)
       *
       * @code{cpp}
       * Ghost<MyBasePtr>::init("MyBase");
       * Ghost<MyClassPtr>::init("MyClass")
       *   .bases<Ghost<MyBasePtr> >();
       * @endcode
       */
      template<class BaseGhost>
      typename boost::enable_if
        <
          boost::is_base_of<GhostMetadata, BaseGhost>,
          Ghost
        >::type&
      bases()
      {
        BOOST_STATIC_ASSERT
        ((
          boost::is_base_of<typename BaseGhost::raw_type, raw_type>::value
        ));

        BaseGhost* base = BaseGhost::getSingletonPtr();
        base->addDerived
        (
          this,
          // Both ways of retrieving the address of a static member function
          // should be legal but not all compilers know this.
          // g++-4.4.7+ has been tested to work with both versions
#if defined(SG_GCC_VERSION) && SG_GCC_VERSION < 40407
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

        if( !_fallback_setter )
          _fallback_setter = base->_fallback_setter;

        return *this;
      }

      /**
       * Register a base class for this ghost. The base class needs to be
       * registers on its own before it can be used as base class.
       *
       * @tparam Base   Type of base class (Base as used in Ghost<BasePtr>)
       *
       * @code{cpp}
       * Ghost<MyBasePtr>::init("MyBase");
       * Ghost<MyClassPtr>::init("MyClass")
       *   .bases<MyBasePtr>();
       * @endcode
       */
      template<class Base>
      typename boost::disable_if
        <
          boost::is_base_of<GhostMetadata, Base>,
          Ghost
        >::type&
      bases()
      {
        BOOST_STATIC_ASSERT
        ((
          boost::is_base_of<typename Ghost<Base>::raw_type, raw_type>::value
        ));

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
      template<class Ret, class Param>
      Ghost& member( const std::string& field,
                     Ret (raw_type::*getter)() const,
                     void (raw_type::*setter)(Param) )
      {
        return member(field, to_getter(getter), to_setter(setter));
      }

      /**
       * Register a read only member variable.
       *
       * @param field   Name of member
       * @param getter  Getter for variable
       */
      template<class Ret>
      Ghost& member( const std::string& field,
                     Ret (raw_type::*getter)() const )
      {
        return member(field, to_getter(getter), setter_t());
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
        return member(field, getter_t(), to_setter(setter));
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
       * Register a function which is called upon setting an unknown member of
       * this ghost.
       */
      Ghost& _set(const fallback_setter_t& setter)
      {
        _fallback_setter = setter;
        return *this;
      }

      /**
       * Register a method which is called upon setting an unknown member of
       * this ghost.
       *
       * @code{cpp}
       * class MyClass
       * {
       *   public:
       *     bool setMember( const std::string& key,
       *                     const std::string& value );
       * }
       *
       * Ghost<MyClassPtr>::init("Test")
       *   ._set(&MyClass::setMember);
       * @endcode
       */
      template<class Param>
      Ghost _set(bool (raw_type::*setter)(const std::string&, Param))
      {
        // Setter signature: bool( naContext,
        //                         raw_type&,
        //                         const std::string&,
        //                         naRef )
        return _set(boost::bind(
          setter,
          _2,
          _3,
          boost::bind(from_nasal_ptr<Param>::get(), _1, _4)
        ));
      }

      /**
       * Register anything that accepts an object instance and a
       * nasal::CallContext and returns naRef as method.
       *
       * @code{cpp}
       * class MyClass
       * {
       *   public:
       *     naRef myMethod(const nasal::CallContext& ctx);
       * }
       *
       * Ghost<MyClassPtr>::init("Test")
       *   .method("myMethod", &MyClass::myMethod);
       * @endcode
       */
      Ghost& method(const std::string& name, const method_t& func)
      {
        _members[name].func = new MethodHolder(func);
        return *this;
      }

      /**
       * Register anything that accepts an object instance and a
       * nasal::CallContext whith automatic conversion of the return type to
       * Nasal.
       *
       * @code{cpp}
       * class MyClass;
       * void doIt(const MyClass& c, const nasal::CallContext& ctx);
       *
       * Ghost<MyClassPtr>::init("Test")
       *   .method("doIt", &doIt);
       * @endcode
       */
      template<class Ret>
      Ghost& method
      (
        const std::string& name,
        const boost::function<Ret (raw_type&, const CallContext&)>& func
      )
      {
        return method(name, boost::bind(method_invoker<Ret>, func, _1, _2));
      }

      // Build dependency for CMake, gcc, etc.
#define SG_DONT_DO_ANYTHING
# include <simgear/nasal/cppbind/detail/functor_templates.hxx>
#undef SG_DONT_DO_ANYTHING

#define BOOST_PP_ITERATION_LIMITS (0, 9)
#define BOOST_PP_FILENAME_1 <simgear/nasal/cppbind/detail/functor_templates.hxx>
#include BOOST_PP_ITERATE()

      // TODO use variadic template when supporting C++11
      // TODO check if default constructor exists
//      static naRef create( naContext c )
//      {
//        return makeGhost(c, createInstance());
//      }

      /**
       * Create a Nasal instance of this ghost.
       *
       * @param c   Active Nasal context
       * @param a1  Parameter used for creating new instance
       */
      template<class A1>
      static naRef create( naContext c, const A1& a1 )
      {
        return makeGhost(c, createInstance(a1));
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
          return getPtr( naGhost_ptr(me) );

        // Now if it is derived from a ghost (hash with ghost in parent vector)
        else if( naIsHash(me) )
        {
          naRef na_parents = naHash_cget(me, const_cast<char*>("parents"));
          if( !naIsVector(na_parents) )
            return pointer();

          typedef std::vector<naRef> naRefs;
          naRefs parents = from_nasal<naRefs>(c, na_parents);
          for( naRefs::const_iterator parent = parents.begin();
                                      parent != parents.end();
                                    ++parent )
          {
            pointer ptr = fromNasal(c, *parent);
            if( get_pointer(ptr) )
              return ptr;
          }
        }

        return pointer();
      }

    private:

      template<class>
      friend class Ghost;

      static naGhostType _ghost_type;

      typedef naGhostType* (*type_checker_t)(const raw_type*);
      typedef std::vector<type_checker_t> DerivedList;
      DerivedList _derived_types;

      /**
       * Create a shared pointer on the heap to handle the reference counting
       * for the passed shared pointer while it is used in Nasal space.
       */
      static pointer* createInstance(const pointer& ptr)
      {
        return get_pointer(ptr) ? new pointer(ptr) : 0;
      }

      static pointer getPtr(void* ptr)
      {
        if( ptr )
          return *static_cast<pointer*>(ptr);
        else
          return pointer();
      }

      static raw_type* getRawPtr(void* ptr)
      {
        if( ptr )
          return get_pointer(*static_cast<pointer*>(ptr));
        else
          return 0;
      }

      static raw_type* getRawPtr(const pointer& ptr)
      {
        return get_pointer(ptr);
      }

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

        if( !getSingletonPtr() )
        {
          SG_LOG
          (
            SG_NASAL,
            SG_INFO,
            "Ghost::getTypeFor: can not get type for unregistered ghost"
          );
          return 0;
        }

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
        return &_ghost_type;
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
        return &BaseGhost::_ghost_type;
      }

      static Ghost* getSingletonPtr()
      {
        return getSingletonHolder().get();
      }

      static raw_type& requireObject(naContext c, naRef me)
      {
        raw_type* obj = getRawPtr( fromNasal(c, me) );
        naGhostType* ghost_type = naGhost_type(me);

        if( !obj )
          naRuntimeError
          (
            c,
            "method called on object of wrong type: is '%s' expected '%s'",
            naIsNil(me) ? "nil" : (ghost_type ? ghost_type->name : "unknown"),
            _ghost_type.name
          );

        return *obj;
      }

      template<class Ret>
      getter_t to_getter(Ret (raw_type::*getter)() const)
      {
        typedef typename boost::call_traits<Ret>::param_type param_type;
        naRef(*to_nasal_)(naContext, param_type) = &to_nasal;

        // Getter signature: naRef(naContext, raw_type&)
        return boost::bind
        (
          to_nasal_,
          _1,
          boost::bind(getter, _2)
        );
      }

      template<class Param>
      setter_t to_setter(void (raw_type::*setter)(Param))
      {
        // Setter signature: void(naContext, raw_type&, naRef)
        return boost::bind
        (
          setter,
          _2,
          boost::bind(from_nasal_ptr<Param>::get(), _1, _3)
        );
      }


      /**
       * Invoke a method which returns a value and convert it to Nasal.
       */
      template<class Ret>
      static
      typename boost::disable_if<boost::is_void<Ret>, naRef>::type
      method_invoker
      (
        const boost::function<Ret (raw_type&, const CallContext&)>& func,
        raw_type& obj,
        const CallContext& ctx
      )
      {
        return (*to_nasal_ptr<Ret>::get())(ctx.c, func(obj, ctx));
      };

      /**
       * Invoke a method which returns void and "convert" it to nil.
       */
      template<class Ret>
      static
      typename boost::enable_if<boost::is_void<Ret>, naRef>::type
      method_invoker
      (
        const boost::function<Ret (raw_type&, const CallContext&)>& func,
        raw_type& obj,
        const CallContext& ctx
      )
      {
        func(obj, ctx);
        return naNil();
      };

      /**
       * Extract argument by index from nasal::CallContext and convert to given
       * type.
       */
      template<class Arg>
      static
      typename boost::disable_if<
        internal::reduced_is_same<Arg, CallContext>,
        typename from_nasal_ptr<Arg>::return_type
      >::type
      arg_from_nasal(const CallContext& ctx, size_t index)
      {
        return ctx.requireArg<Arg>(index);
      };

      /**
       * Specialization to pass through nasal::CallContext.
       */
      template<class Arg>
      static
      typename boost::enable_if<
        internal::reduced_is_same<Arg, CallContext>,
        typename from_nasal_ptr<Arg>::return_type
      >::type
      arg_from_nasal(const CallContext& ctx, size_t)
      {
        // Either const CallContext& or CallContext, non-const reference
        // does not make sense.
        BOOST_STATIC_ASSERT( (!boost::is_same<Arg, CallContext&>::value) );
        return ctx;
      };

      typedef std::auto_ptr<Ghost> GhostPtr;
      MemberMap         _members;
      fallback_setter_t _fallback_setter;

      explicit Ghost(const std::string& name):
        GhostMetadata(name, &_ghost_type)
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
        if( getRawPtr(ptr) )
        {
          // We are wrapping shared pointers to already existing objects which
          // will then be hold be a new shared pointer. We therefore have to
          // check for the dynamic type of the object as it might differ from
          // the passed static type.
          naGhostType* ghost_type = getTypeFor<Ghost>( getRawPtr(ptr) );

          if( ghost_type )
            return naNewGhost2(c, ghost_type, ptr);
        }

        destroyGhost(ptr);
        return naNil();
      }

      static void destroyGhost(void *ptr)
      {
        delete static_cast<pointer*>(ptr);
      }

      /**
       * Callback for retrieving a ghost member.
       */
      static const char* getMember(naContext c, void* g, naRef key, naRef* out)
      {
        const std::string key_str = nasal::from_nasal<std::string>(c, key);
        // TODO merge instance parents with static class parents
//        if( key_str == "parents" )
//        {
//          if( getSingletonPtr()->_parents.empty() )
//            return 0;
//
//          *out = getSingletonPtr()->getParents(c);
//          return "";
//        }

        typename MemberMap::iterator member =
          getSingletonPtr()->_members.find(key_str);

        if( member == getSingletonPtr()->_members.end() )
          return 0;

        if( member->second.func )
          *out = member->second.func->get_naRef(c);
        else if( !member->second.getter.empty() )
          *out = member->second.getter(c, *getRawPtr(g));
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
        const MemberMap& members = getSingletonPtr()->_members;

        typename MemberMap::const_iterator member = members.find(key);
        if( member == members.end() )
        {
          fallback_setter_t fallback_set = getSingletonPtr()->_fallback_setter;
          if( !fallback_set )
            naRuntimeError(c, "ghost: No such member: %s", key.c_str());
          else if( !fallback_set(c, *getRawPtr(g), key, val) )
            naRuntimeError(c, "ghost: Failed to write (_set: %s)", key.c_str());
        }
        else if( member->second.setter.empty() )
          naRuntimeError(c, "ghost: Write protected member: %s", key.c_str());
        else if( member->second.func )
          naRuntimeError(c, "ghost: Write to function: %s", key.c_str());
        else
          member->second.setter(c, *getRawPtr(g), val);
      }
  };

  template<class T>
  naGhostType Ghost<T>::_ghost_type;

} // namespace nasal

// Needs to be outside any namespace to make ADL work
/**
 * Convert every shared pointer to a ghost.
 */
template<class T>
typename boost::enable_if<
  nasal::internal::has_element_type<
    typename nasal::internal::reduced_type<T>::type
  >,
  naRef
>::type
to_nasal_helper(naContext c, T ptr)
{
  return nasal::Ghost<T>::create(c, ptr);
}

/**
 * Convert nasal ghosts/hashes to shared pointer (of a ghost).
 */
template<class T>
typename boost::enable_if<
  nasal::internal::has_element_type<
    typename nasal::internal::reduced_type<T>::type
  >,
  T
>::type
from_nasal_helper(naContext c, naRef ref, const T*)
{
  return nasal::Ghost<T>::fromNasal(c, ref);
}

/**
 * Convert any pointer to a SGReference based object to a ghost.
 */
template<class T>
typename boost::enable_if<boost::is_base_of<SGReferenced, T>, naRef>::type
to_nasal_helper(naContext c, T* ptr)
{
  return nasal::Ghost<SGSharedPtr<T> >::create(c, SGSharedPtr<T>(ptr));
}

/**
 * Convert nasal ghosts/hashes to pointer (of a SGReference based ghost).
 */
template<class T>
typename boost::enable_if<
  boost::is_base_of<SGReferenced, typename boost::remove_pointer<T>::type>,
  T
>::type
from_nasal_helper(naContext c, naRef ref, const T*)
{
  typedef SGSharedPtr<typename boost::remove_pointer<T>::type> TypeRef;
  return nasal::Ghost<TypeRef>::fromNasal(c, ref).release();
}

#endif /* SG_NASAL_GHOST_HXX_ */
