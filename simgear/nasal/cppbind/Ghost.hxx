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

namespace osg
{
  template<class T>
  inline T* get_pointer(observer_ptr<T> const& p)
  {
    ref_ptr<T> ref;
    p.lock(ref);
    return ref.get();
  }
}

template<class T>
inline typename boost::enable_if<
  boost::is_pointer<T>,
  T
>::type
get_pointer(T ptr)
{
  return ptr;
}

// Both ways of retrieving the address of a static member function
// should be legal but not all compilers know this.
// g++-4.4.7+ has been tested to work with both versions
#if defined(SG_GCC_VERSION) && SG_GCC_VERSION < 40407
  // The old version of g++ used on Jenkins (16.11.2012) only compiles
  // this version.
# define SG_GET_TEMPLATE_MEMBER(type, member) &member

  // With old g++ on Jenkins (21.07.2014), ADL for static_pointer_cast does not
  // work.
  using boost::static_pointer_cast;
  using osg::static_pointer_cast;
#else
  // VS (2008, 2010, ... ?) only allow this version.
# define SG_GET_TEMPLATE_MEMBER(type, member) &type::template member
#endif

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
        typedef void(*Deleter)(void*);
        typedef std::vector<std::pair<Deleter, void*> > DestroyList;

        static DestroyList _destroy_list;

        /**
         * Add a nasal base class to the ghost. Will be available in the ghosts
         * parents array.
         */
        void addNasalBase(const naRef& parent);

        bool isInstance(naGhostType* ghost_type, bool& is_weak) const;

      protected:

        const std::string   _name_strong,
                            _name_weak;
        const naGhostType  *_ghost_type_strong_ptr,
                           *_ghost_type_weak_ptr;
        std::vector<naRef>  _parents;

        GhostMetadata( const std::string& name,
                       const naGhostType* ghost_type_strong,
                       const naGhostType* ghost_type_weak );

        void addDerived(const GhostMetadata* derived);

        naRef getParents(naContext c);
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

  /** @brief Destroy all ghost queued for deletion.
   *
   * This needs can not be done directly during garbage collection, as
   * destructors may execute Nasal code which requires creating new Nasal
   * contexts. Creating a Nasal context during garbage collection is not
   * possible and leads to a dead lock.
   */
  void ghostProcessDestroyList();

  typedef SGSharedPtr<internal::MethodHolder> MethodHolderPtr;
  typedef SGWeakPtr<internal::MethodHolder> MethodHolderWeakPtr;

  // Dummy template to create shorter and easy to understand compile errors if
  // trying to wrap the wrong type as a Ghost.
  template<class T, class Enable = void>
  class Ghost
  {
    public:
      BOOST_STATIC_ASSERT(("Ghost can only wrap shared pointer!"
        && is_strong_ref<T>::value
      ));

      static Ghost& init(const std::string& name);
      static bool isInit();
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
  class Ghost<T, typename boost::enable_if<is_strong_ref<T> >::type>:
    public internal::GhostMetadata
  {
      // Shared pointer required for Ghost (no weak pointer!)
      BOOST_STATIC_ASSERT((is_strong_ref<T>::value));

    public:
      typedef typename T::element_type                              raw_type;
      typedef typename shared_ptr_traits<T>::strong_ref             strong_ref;
      typedef typename shared_ptr_traits<T>::weak_ref               weak_ref;
      typedef naRef (raw_type::*member_func_t)(const CallContext&);
      typedef naRef (*free_func_t)(raw_type&, const CallContext&);
      typedef boost::function<naRef(raw_type&, naContext)>          getter_t;
      typedef boost::function<void( raw_type&, naContext, naRef)>   setter_t;
      typedef boost::function<naRef(raw_type&, const CallContext&)> method_t;
      typedef boost::function<bool( raw_type&,
                                    naContext,
                                    const std::string&,
                                    naRef& )>              fallback_getter_t;
      typedef boost::function<bool( raw_type&,
                                    naContext,
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

              // Keep reference for duration of call to prevent expiring
              // TODO not needed for strong referenced ghost
              strong_ref ref = fromNasal(c, me);
              if( !ref )
              {
                naGhostType* ghost_type = naGhost_type(me);
                naRuntimeError
                (
                  c,
                  "method called on object of wrong type: "
                  "is '%s' expected '%s'",
                  naIsNil(me) ? "nil"
                              : (ghost_type ? ghost_type->name : "unknown"),
                  _ghost_type_strong.name
                );
              }

              return holder->_method
              (
                *get_pointer(ref),
                CallContext(c, me, argc, args)
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

        typedef typename BaseGhost::strong_ref base_ref;

        BaseGhost* base = BaseGhost::getSingletonPtr();
        base->addDerived(
          this,
          SG_GET_TEMPLATE_MEMBER(Ghost, toNasal<BaseGhost>),
          SG_GET_TEMPLATE_MEMBER(Ghost, fromNasalWithCast<base_ref>)
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
        if( !_fallback_getter )
          _fallback_getter = base->_fallback_getter;

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

      template<class Param>
      Ghost& member( const std::string& field,
                     const getter_t& getter,
                     void (raw_type::*setter)(Param) )
      {
        return member(field, getter, to_setter(setter));
      }

      template<class Ret>
      Ghost& member( const std::string& field,
                     Ret (raw_type::*getter)() const,
                     const setter_t& setter )
      {
        return member(field, to_getter(getter), setter);
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

      Ghost& member( const std::string& field,
                     naRef (*getter)(const raw_type&, naContext) )
      {
        return member(field, getter_t(getter), setter_t());
      }

      Ghost& member( const std::string& field,
                     naRef (*getter)(raw_type&, naContext) )
      {
        return member(field, getter_t(getter), setter_t());
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

      Ghost& member( const std::string& field,
                     void (*setter)(raw_type&, naContext, naRef) )
      {
        return member(field, getter_t(), setter_t(setter));
      }

      Ghost& member( const std::string& field,
                     const setter_t& setter )
      {
        return member(field, getter_t(), setter);
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
       * Register a function which is called upon retrieving an unknown member
       * of this ghost.
       */
      Ghost& _get(const fallback_getter_t& getter)
      {
        _fallback_getter = getter;
        return *this;
      }

      /**
       * Register a function which is called upon retrieving an unknown member
       * of this ghost, and convert it to the given @a Param type.
       */
      template<class Param>
      Ghost& _get( const boost::function<bool ( raw_type&,
                                                const std::string&,
                                                Param& )>& getter )
      {
        return _get(boost::bind(
          convert_param_invoker<Param>, getter, _1, _2, _3, _4
        ));
      }

      /**
       * Register a method which is called upon retrieving an unknown member of
       * this ghost.
       *
       * @code{cpp}
       * class MyClass
       * {
       *   public:
       *     bool getMember( const std::string& key,
       *                     std::string& value_out ) const;
       * }
       *
       * Ghost<MyClassPtr>::init("Test")
       *   ._get(&MyClass::getMember);
       * @endcode
       */
      template<class Param>
      Ghost& _get(bool (raw_type::*getter)(const std::string&, Param&) const)
      {
        return _get(
          boost::function<bool (raw_type&, const std::string&, Param&)>(getter)
        );
      }

      /**
       * Register a method which is called upon retrieving an unknown member of
       * this ghost.
       *
       * @code{cpp}
       * class MyClass
       * {
       *   public:
       *     bool getMember( naContext c,
       *                     const std::string& key,
       *                     naRef& value_out );
       * }
       *
       * Ghost<MyClassPtr>::init("Test")
       *   ._get(&MyClass::getMember);
       * @endcode
       */
      Ghost& _get(bool (raw_type::*getter)( naContext,
                                            const std::string&,
                                            naRef& ) const)
      {
        return _get( fallback_getter_t(getter) );
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
       * Register a function which is called upon setting an unknown member of
       * this ghost, and convert it to the given @a Param type.
       */
      template<class Param>
      Ghost& _set(const boost::function<bool ( raw_type&,
                                               const std::string&,
                                               Param )>& setter)
      {
        // Setter signature: bool( raw_type&,
        //                         naContext,
        //                         const std::string&,
        //                         naRef )
        return _set(boost::bind(
          setter,
          _1,
          _3,
          boost::bind(from_nasal_ptr<Param>::get(), _2, _4)
        ));
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
      Ghost& _set(bool (raw_type::*setter)(const std::string&, Param))
      {
        return _set(
          boost::function<bool (raw_type&, const std::string&, Param)>(setter)
        );
      }

      /**
       * Register a method which is called upon setting an unknown member of
       * this ghost.
       *
       * @code{cpp}
       * class MyClass
       * {
       *   public:
       *     bool setMember( naContext c,
       *                     const std::string& key,
       *                     naRef value );
       * }
       *
       * Ghost<MyClassPtr>::init("Test")
       *   ._set(&MyClass::setMember);
       * @endcode
       */
      Ghost& _set(bool (raw_type::*setter)( naContext,
                                            const std::string&,
                                            naRef ))
      {
        return _set( fallback_setter_t(setter) );
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

      /**
       * Create a shared pointer on the heap to handle the reference counting
       * for the passed shared pointer while it is used in Nasal space.
       */
      template<class RefType>
      static
      typename boost::enable_if_c<
           boost::is_same<RefType, strong_ref>::value
        || boost::is_same<RefType, weak_ref>::value,
        naRef
      >::type
      makeGhost(naContext c, RefType const& ref_ptr)
      {
        strong_ref ref(ref_ptr);
        raw_type* ptr = get_pointer(ref);
        if( !ptr )
          return naNil();

        // We are wrapping shared pointers to already existing objects which
        // will then be hold be a new shared pointer. We therefore have to
        // check for the dynamic type of the object as it might differ from
        // the passed static type.
        return toNasal<Ghost>(
          c,
          ref,
          shared_ptr_traits<RefType>::is_strong::value
        );
      }

      /**
       * Convert Nasal object to C++ object. To get a valid object the passed
       * Nasal objects has to be derived class of the target class (Either
       * derived in C++ or in Nasal using a 'parents' vector)
       */
      static strong_ref fromNasal(naContext c, naRef me)
      {
        naGhostType* ghost_type = naGhost_type(me);
        if( ghost_type )
        {
          // Check if we got an instance of this class
          bool is_weak = false;
          if( isInstance(ghost_type, is_weak) )
          {
            return is_weak ? getPtr<strong_ref, true>(naGhost_ptr(me))
                           : getPtr<strong_ref, false>(naGhost_ptr(me));
          }

          // otherwise try the derived classes
          for( typename DerivedList::reverse_iterator
                 derived = getSingletonPtr()->_derived_types.rbegin();
                 derived != getSingletonPtr()->_derived_types.rend();
               ++derived )
          {
            strong_ref ref = (derived->from_nasal)(c, me);

            if( get_pointer(ref) )
              return ref;
          }
        }
        else if( naIsHash(me) )
        {
          naRef na_parents = naHash_cget(me, const_cast<char*>("parents"));
          if( !naIsVector(na_parents) )
            return strong_ref();

          typedef std::vector<naRef> naRefs;
          naRefs parents = from_nasal<naRefs>(c, na_parents);
          for( naRefs::const_iterator parent = parents.begin();
                                      parent != parents.end();
                                    ++parent )
          {
            strong_ref ref = fromNasal(c, *parent);
            if( get_pointer(ref) )
              return ref;
          }
        }

        return strong_ref();
      }

      /**
       * Convert Nasal object to C++ object and check if it has the correct
       * type.
       *
       * @see fromNasal
       */
      static strong_ref fromNasalChecked(naContext c, naRef me)
      {
        strong_ref obj = fromNasal(c, me);
        if( obj )
          return obj;
        if( naIsNil(me) )
          return strong_ref();

        std::string msg = "Can not convert to '"
                        + getSingletonPtr()->_name_strong
                        + "': ";

        naGhostType* ghost_type = naGhost_type(me);
        if( ghost_type )
          msg += "not a derived class (or expired weak ref): "
                 "'" + std::string(ghost_type->name) + "'";
        else if( naIsHash(me) )
        {
          if( !naIsVector(naHash_cget(me, const_cast<char*>("parents"))) )
            msg += "missing parents vector";
          else
            msg += "not a derived hash";
        }
        else
          msg += "not an object";

        throw bad_nasal_cast(msg);
      }

    private:

      template<class, class>
      friend class Ghost;

      static naGhostType _ghost_type_strong, //!< Stored as shared pointer
                         _ghost_type_weak;   //!< Stored as weak shared pointer

      typedef naRef (*to_nasal_t)(naContext, const strong_ref&, bool);
      typedef strong_ref (*from_nasal_t)(naContext, naRef);
      struct DerivedInfo
      {
        to_nasal_t to_nasal;
        from_nasal_t from_nasal;

        DerivedInfo( to_nasal_t to_nasal_func,
                     from_nasal_t from_nasal_func ):
          to_nasal(to_nasal_func),
          from_nasal(from_nasal_func)
        {}
      };

      typedef std::vector<DerivedInfo> DerivedList;
      DerivedList _derived_types;

      static bool isInstance(naGhostType* ghost_type, bool& is_weak)
      {
        if( !ghost_type || !getSingletonPtr() )
          return false;

        return getSingletonPtr()->GhostMetadata::isInstance( ghost_type,
                                                             is_weak );
      }

      template<class RefPtr, bool is_weak>
      static
      typename boost::enable_if_c<
        !is_weak,
        RefPtr
      >::type
      getPtr(void* ptr)
      {
        typedef shared_ptr_storage<strong_ref> storage_type;
        if( ptr )
          return storage_type::template get<RefPtr>(
            static_cast<typename storage_type::storage_type*>(ptr)
          );
        else
          return RefPtr();
      }

      template<class RefPtr, bool is_weak>
      static
      typename boost::enable_if_c<
        is_weak && supports_weak_ref<T>::value,
        RefPtr
      >::type
      getPtr(void* ptr)
      {
        typedef shared_ptr_storage<weak_ref> storage_type;
        if( ptr )
          return storage_type::template get<RefPtr>(
            static_cast<typename storage_type::storage_type*>(ptr)
          );
        else
          return RefPtr();
      }

      template<class RefPtr, bool is_weak>
      static
      typename boost::enable_if_c<
        is_weak && !supports_weak_ref<T>::value,
        RefPtr
      >::type
      getPtr(void* ptr)
      {
        return RefPtr();
      }

      void addDerived( const internal::GhostMetadata* derived_meta,
                       to_nasal_t to_nasal_func,
                       from_nasal_t from_nasal_func )
      {
        GhostMetadata::addDerived(derived_meta);
        _derived_types.push_back(DerivedInfo(to_nasal_func, from_nasal_func));
      }

      template<class BaseGhost>
      static
      typename boost::enable_if
        < boost::is_polymorphic<typename BaseGhost::raw_type>,
          naRef
        >::type
      toNasal( naContext c,
               const typename BaseGhost::strong_ref& base_ref,
               bool strong )
      {
        typename BaseGhost::raw_type* ptr = get_pointer(base_ref);

        // Check first if passed pointer can by converted to instance of class
        // this ghost wraps.
        if(   !boost::is_same
                 < typename BaseGhost::raw_type,
                   typename Ghost::raw_type
                 >::value
            && dynamic_cast<const typename Ghost::raw_type*>(ptr) != ptr )
          return naNil();

        if( !getSingletonPtr() )
        {
          SG_LOG
          (
            SG_NASAL,
            SG_INFO,
            "Ghost::getTypeFor: can not get type for unregistered ghost"
          );
          return naNil();
        }

        strong_ref ref =
          static_pointer_cast<typename Ghost::raw_type>(base_ref);

        // Now check if we can further downcast to one of our derived classes.
        for( typename DerivedList::reverse_iterator
               derived = getSingletonPtr()->_derived_types.rbegin();
               derived != getSingletonPtr()->_derived_types.rend();
             ++derived )
        {
          naRef ghost = (derived->to_nasal)(c, ref, strong);

          if( !naIsNil(ghost) )
            return ghost;
        }

        // If base is not an instance of any derived class, this class has to
        // be the dynamic type.
        return strong
             ? create<false>(c, ref)
             : create<true>(c, ref);
      }

      template<class BaseGhost>
      static
      typename boost::disable_if
        < boost::is_polymorphic<typename BaseGhost::raw_type>,
          naRef
        >::type
        toNasal( naContext c,
                 const typename BaseGhost::strong_ref& ref,
                 bool strong )
      {
        // For non polymorphic classes there is no possibility to get the actual
        // dynamic type, therefore we can only use its static type.
        return strong
             ? create<false>(c, ref)
             : create<true>(c, ref);
      }

      template<class Type>
      static Type fromNasalWithCast(naContext c, naRef me)
      {
        return Type(fromNasal(c, me));
      }

      static Ghost* getSingletonPtr()
      {
        return getSingletonHolder().get();
      }

      template<class Ret>
      getter_t to_getter(Ret (raw_type::*getter)() const)
      {
        typedef typename boost::call_traits<Ret>::param_type param_type;
        naRef(*to_nasal_)(naContext, param_type) = &to_nasal;

        // Getter signature: naRef(raw_type&, naContext)
        return boost::bind
        (
          to_nasal_,
          _2,
          boost::bind(getter, _1)
        );
      }

      template<class Param>
      setter_t to_setter(void (raw_type::*setter)(Param))
      {
        // Setter signature: void(raw_type&, naContext, naRef)
        return boost::bind
        (
          setter,
          _1,
          boost::bind(from_nasal_ptr<Param>::get(), _2, _3)
        );
      }

      /**
       * Invoke a method which writes the converted parameter to a reference
       */
      template<class Param>
      static
      bool convert_param_invoker
      (
        const boost::function<bool ( raw_type&,
                                     const std::string&,
                                     Param& )>& func,
        raw_type& obj,
        naContext c,
        const std::string& key,
        naRef& out
      )
      {
        Param p;
        if( !func(obj, key, p) )
          return false;

        out = to_nasal(c, p);
        return true;
      };

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
      fallback_getter_t _fallback_getter;
      fallback_setter_t _fallback_setter;

      explicit Ghost(const std::string& name):
        GhostMetadata( name,
                       &_ghost_type_strong,
                       supports_weak_ref<T>::value ? &_ghost_type_weak : NULL )
      {
        _ghost_type_strong.destroy =
          SG_GET_TEMPLATE_MEMBER(Ghost, queueDestroy<strong_ref>);
        _ghost_type_strong.name = _name_strong.c_str();
        _ghost_type_strong.get_member = &Ghost::getMemberStrong;
        _ghost_type_strong.set_member = &Ghost::setMemberStrong;

        _ghost_type_weak.destroy =
          SG_GET_TEMPLATE_MEMBER(Ghost, queueDestroy<weak_ref>);
        _ghost_type_weak.name = _name_weak.c_str();

        if( supports_weak_ref<T>::value )
        {
          _ghost_type_weak.get_member = &Ghost::getMemberWeak;
          _ghost_type_weak.set_member = &Ghost::setMemberWeak;
        }
        else
        {
          _ghost_type_weak.get_member = 0;
          _ghost_type_weak.set_member = 0;
        }
      }

      static GhostPtr& getSingletonHolder()
      {
        static GhostPtr instance;
        return instance;
      }

      template<bool is_weak>
      static
      typename boost::enable_if_c<
        !is_weak,
        naRef
      >::type
      create(naContext c, const strong_ref& ref_ptr)
      {
        typedef shared_ptr_storage<strong_ref> storage_type;
        return naNewGhost2( c,
                            &Ghost::_ghost_type_strong,
                            storage_type::ref(ref_ptr) );
      }

      template<bool is_weak>
      static
      typename boost::enable_if_c<
        is_weak && supports_weak_ref<T>::value,
        naRef
      >::type
      create(naContext c, const strong_ref& ref_ptr)
      {
        typedef shared_ptr_storage<weak_ref> storage_type;
        return naNewGhost2( c,
                            &Ghost::_ghost_type_weak,
                            storage_type::ref(ref_ptr) );
      }

      template<bool is_weak>
      static
      typename boost::enable_if_c<
        is_weak && !supports_weak_ref<T>::value,
        naRef
      >::type
      create(naContext, const strong_ref&)
      {
        return naNil();
      }

      template<class Type>
      static void destroy(void *ptr)
      {
        typedef shared_ptr_storage<Type> storage_type;
        storage_type::unref(
          static_cast<typename storage_type::storage_type*>(ptr)
        );
      }

      template<class Type>
      static void queueDestroy(void *ptr)
      {
        _destroy_list.push_back( DestroyList::value_type(&destroy<Type>, ptr) );
      }

      static void raiseErrorExpired(naContext c, const char* str)
      {
        Ghost* ghost_info = getSingletonPtr();
        naRuntimeError(
          c,
          "Ghost::%s: ghost has expired '%s'",
          str,
          ghost_info ? ghost_info->_name_strong.c_str() : "unknown"
        );
      }

      /**
       * Callback for retrieving a ghost member.
       */
      static const char* getMemberImpl( naContext c,
                                        raw_type& obj,
                                        naRef key,
                                        naRef* out )
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
        {
          fallback_getter_t fallback_get = getSingletonPtr()->_fallback_getter;
          if(    !fallback_get
              || !fallback_get(obj, c, key_str, *out) )
            return 0;
        }
        else if( member->second.func )
          *out = member->second.func->get_naRef(c);
        else if( !member->second.getter.empty() )
          *out = member->second.getter(obj, c);
        else
          return "Read-protected member";

        return "";
      }

      static const char*
      getMemberWeak(naContext c, void* ghost, naRef key, naRef* out)
      {
        // Keep a strong reference while retrieving member, to prevent deleting
        // object in between.
        strong_ref ref = getPtr<strong_ref, true>(ghost);
        if( !ref )
          raiseErrorExpired(c, "getMember");

        return getMemberImpl(c, *get_pointer(ref), key, out);
      }

      static const char*
      getMemberStrong(naContext c, void* ghost, naRef key, naRef* out)
      {
        // Just get a raw pointer as we are keeping a strong reference as ghost
        // anyhow.
        raw_type* ptr = getPtr<raw_type*, false>(ghost);
        return getMemberImpl(c, *ptr, key, out);
      }

      /**
       * Callback for writing to a ghost member.
       */
      static void setMemberImpl( naContext c,
                                 raw_type& obj,
                                 naRef field,
                                 naRef val )
      {
        const std::string key = nasal::from_nasal<std::string>(c, field);
        const MemberMap& members = getSingletonPtr()->_members;

        typename MemberMap::const_iterator member = members.find(key);
        if( member == members.end() )
        {
          fallback_setter_t fallback_set = getSingletonPtr()->_fallback_setter;
          if( !fallback_set )
            naRuntimeError(c, "ghost: No such member: %s", key.c_str());
          else if( !fallback_set(obj, c, key, val) )
            naRuntimeError(c, "ghost: Failed to write (_set: %s)", key.c_str());
        }
        else if( member->second.setter.empty() )
          naRuntimeError(c, "ghost: Write protected member: %s", key.c_str());
        else if( member->second.func )
          naRuntimeError(c, "ghost: Write to function: %s", key.c_str());
        else
          member->second.setter(obj, c, val);
      }

      static void
      setMemberWeak(naContext c, void* ghost, naRef field, naRef val)
      {
        // Keep a strong reference while retrieving member, to prevent deleting
        // object in between.
        strong_ref ref = getPtr<strong_ref, true>(ghost);
        if( !ref )
          raiseErrorExpired(c, "setMember");

        setMemberImpl(c, *get_pointer(ref), field, val);
      }

      static void
      setMemberStrong(naContext c, void* ghost, naRef field, naRef val)
      {
        // Just get a raw pointer as we are keeping a strong reference as ghost
        // anyhow.
        raw_type* ptr = getPtr<raw_type*, false>(ghost);
        setMemberImpl(c, *ptr, field, val);
      }
  };

  template<class T>
  naGhostType
  Ghost<T, typename boost::enable_if<is_strong_ref<T> >::type>
  ::_ghost_type_strong;

  template<class T>
  naGhostType
  Ghost<T, typename boost::enable_if<is_strong_ref<T> >::type>
  ::_ghost_type_weak;

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
  typedef typename nasal::shared_ptr_traits<T>::strong_ref strong_ref;
  return nasal::Ghost<strong_ref>::makeGhost(c, ptr);
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
  typedef typename nasal::shared_ptr_traits<T>::strong_ref strong_ref;
  return T(nasal::Ghost<strong_ref>::fromNasalChecked(c, ref));
}

/**
 * Convert any pointer to a SGReferenced based object to a ghost.
 */
template<class T>
typename boost::enable_if_c<
     boost::is_base_of<SGReferenced, T>::value
  || boost::is_base_of<SGWeakReferenced, T>::value,
  naRef
>::type
to_nasal_helper(naContext c, T* ptr)
{
  return nasal::Ghost<SGSharedPtr<T> >::makeGhost(c, SGSharedPtr<T>(ptr));
}

/**
 * Convert nasal ghosts/hashes to pointer (of a SGReferenced based ghost).
 */
template<class T>
typename boost::enable_if_c<
     boost::is_base_of<
       SGReferenced,
       typename boost::remove_pointer<T>::type
     >::value
  || boost::is_base_of<
       SGWeakReferenced,
       typename boost::remove_pointer<T>::type
     >::value,
  T
>::type
from_nasal_helper(naContext c, naRef ref, const T*)
{
  typedef SGSharedPtr<typename boost::remove_pointer<T>::type> TypeRef;
  return T(nasal::Ghost<TypeRef>::fromNasalChecked(c, ref));
}

/**
 * Convert any pointer to a osg::Referenced based object to a ghost.
 */
template<class T>
typename boost::enable_if<
  boost::is_base_of<osg::Referenced, T>,
  naRef
>::type
to_nasal_helper(naContext c, T* ptr)
{
  return nasal::Ghost<osg::ref_ptr<T> >::makeGhost(c, osg::ref_ptr<T>(ptr));
}

/**
 * Convert nasal ghosts/hashes to pointer (of a osg::Referenced based ghost).
 */
template<class T>
typename boost::enable_if<
  boost::is_base_of<osg::Referenced, typename boost::remove_pointer<T>::type>,
  T
>::type
from_nasal_helper(naContext c, naRef ref, const T*)
{
  typedef osg::ref_ptr<typename boost::remove_pointer<T>::type> TypeRef;
  return T(nasal::Ghost<TypeRef>::fromNasalChecked(c, ref));
}

#endif /* SG_NASAL_GHOST_HXX_ */
