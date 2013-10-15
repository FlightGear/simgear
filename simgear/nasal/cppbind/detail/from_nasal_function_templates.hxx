#ifndef SG_FROM_NASAL_HELPER_HXX_
# error Nasal cppbind - do not include this file!
#endif

#define n BOOST_PP_ITERATION()

#ifndef SG_BOOST_FUNCTION_FROM_NASAL_FWD
# define SG_CALL_TRAITS_PARAM(z, n, dummy)\
    typename boost::call_traits<A##n>::param_type a##n
# define SG_CALL_ARG(z, n, dummy)\
      to_nasal(ctx, a##n)

  template<
    class Ret
    BOOST_PP_COMMA_IF(n)
    BOOST_PP_ENUM_PARAMS(n, class A)
  >
  typename boost::disable_if<boost::is_void<Ret>, Ret>::type
  callNasalFunction( const ObjectHolder* holder
                     BOOST_PP_COMMA_IF(n)
                     BOOST_PP_ENUM(n, SG_CALL_TRAITS_PARAM, 0)
                   )
  {
    naContext ctx = naNewContext();
    naRef args[] = {
      BOOST_PP_ENUM(n, SG_CALL_ARG, 0)
    };
    const int num_args = sizeof(args)/sizeof(args[0]);

    naRef result =
      naCallMethod(holder->get_naRef(), naNil(), num_args, args, naNil());

    Ret r = from_nasal_helper(ctx, result, static_cast<Ret*>(0));
    naFreeContext(ctx);

    return r;
  }

  template<
    class Ret
    BOOST_PP_COMMA_IF(n)
    BOOST_PP_ENUM_PARAMS(n, class A)
  >
  typename boost::enable_if<boost::is_void<Ret>, Ret>::type
  callNasalFunction( const ObjectHolder* holder
                     BOOST_PP_COMMA_IF(n)
                     BOOST_PP_ENUM(n, SG_CALL_TRAITS_PARAM, 0)
                   )
  {
    naContext ctx = naNewContext();
    naRef args[] = {
      BOOST_PP_ENUM(n, SG_CALL_ARG, 0)
    };
    const int num_args = sizeof(args)/sizeof(args[0]);

    naCallMethod(holder->get_naRef(), naNil(), num_args, args, naNil());
    naFreeContext(ctx);
  }

# undef SG_CALL_TRAITS_PARAM
# undef SG_CALL_ARG
#endif

  template<
    class Ret
    BOOST_PP_COMMA_IF(n)
    BOOST_PP_ENUM_PARAMS(n, class A)
  >
  boost::function<Ret (BOOST_PP_ENUM_PARAMS(n, A))>
  boostFunctionFromNasal(naRef code, Ret (*)(BOOST_PP_ENUM_PARAMS(n, A)))
#ifdef SG_BOOST_FUNCTION_FROM_NASAL_FWD
  ;
#else
  {
    return boost::bind
    (
      &callNasalFunction<Ret BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, A)>,
      ObjectHolder::makeShared(code)
      BOOST_PP_COMMA_IF(n)
      BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), _)
    );
  }
#endif

#undef n
