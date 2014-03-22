#ifndef SG_NASAL_GHOST_HXX_
# error Nasal cppbind - do not include this file!
#endif

#ifndef SG_DONT_DO_ANYTHING
#define n BOOST_PP_ITERATION()

#define SG_GHOST_FUNC_TYPE\
  boost::function<Ret (raw_type& BOOST_PP_ENUM_TRAILING_PARAMS(n,A))>

  /**
   * Bind any callable entity accepting an instance of raw_type and an arbitrary
   * number of arguments as method.
   */
  template<
    class Ret
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  Ghost& method(const std::string& name, const SG_GHOST_FUNC_TYPE& func)
  {
#if defined(SG_GCC_VERSION) && SG_GCC_VERSION < 40407
    // The old version of g++ used on Jenkins (16.11.2012) only compiles this
    // version.
# define SG_GHOST_REQUIRE_ARG(z, n, dummy)\
    boost::bind(&arg_from_nasal<A##n>, _2, n)
#else
    // VS (2008, 2010, ... ?) only allow this version.
# define SG_GHOST_REQUIRE_ARG(z, n, dummy)\
    boost::bind(&Ghost::arg_from_nasal<A##n>, _2, n)
#endif

    return method<Ret>
    (
      name,
      typename boost::function<Ret (raw_type&, const CallContext&)>
      ( boost::bind(
        func,
        _1
        BOOST_PP_ENUM_TRAILING(n, SG_GHOST_REQUIRE_ARG, 0)
      ))
    );

#undef SG_GHOST_REQUIRE_ARG
  }

#define SG_GHOST_MEM_FN(cv)\
  /**\
   * Bind a member function with an arbitrary number of arguments as method.\
   */\
  template<\
    class Ret\
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)\
  >\
  Ghost& method\
  (\
    const std::string& name,\
    Ret (raw_type::*fn)(BOOST_PP_ENUM_PARAMS(n,A)) cv\
  )\
  {\
    return method<\
      Ret\
      BOOST_PP_ENUM_TRAILING_PARAMS(n,A)\
    >(name, SG_GHOST_FUNC_TYPE(fn));\
  }

// Work around MSVC warning C4003: not enough actual parameters for macro
// We really do not want to pass a parameter, even if MSVC can not believe it.
#define SG_GHOST_NO_CV

  SG_GHOST_MEM_FN(const)
  SG_GHOST_MEM_FN(SG_GHOST_NO_CV)

#undef SG_GHOST_MEM_FN
#undef SG_GHOST_NO_CV

  /**
   * Bind free function accepting an instance of raw_type and an arbitrary
   * number of arguments as method.
   */
  template<
    class Ret,
    class Type
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  Ghost& method
  (
    const std::string& name,
    Ret (*fn)(Type BOOST_PP_ENUM_TRAILING_PARAMS(n,A))
  )
  {
    BOOST_STATIC_ASSERT
    (( boost::is_convertible<raw_type&, Type>::value
    //|| boost::is_convertible<raw_type*, Type>::value
    // TODO check how to do it with pointer...
    ));
    return method<Ret>(name, SG_GHOST_FUNC_TYPE(fn));
  }

#undef n
#undef SG_GHOST_TYPEDEF_FUNC_TYPE
#endif // SG_DONT_DO_ANYTHING
