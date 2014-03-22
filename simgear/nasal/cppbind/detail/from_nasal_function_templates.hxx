#ifndef SG_FROM_NASAL_HELPER_HXX_
# error Nasal cppbind - do not include this file!
#endif

#ifndef SG_DONT_DO_ANYTHING
#define n BOOST_PP_ITERATION()

#ifndef SG_BOOST_FUNCTION_FROM_NASAL_FWD
# define SG_CALL_TRAITS_PARAM(z, n, dummy)\
    typename boost::call_traits<A##n>::param_type a##n
# define SG_CALL_ARG(z, n, dummy)\
      to_nasal<typename boost::call_traits<A##n>::param_type>(ctx, a##n)

  template<
    class Ret
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  typename boost::disable_if<boost::is_void<Ret>, Ret>::type
  callNasalMethod( const ObjectHolder<SGReferenced>* holder,
                   Me self
                   BOOST_PP_ENUM_TRAILING(n, SG_CALL_TRAITS_PARAM, 0) )
  {
    naContext ctx = naNewContext();
#if n
    naRef args[n] = {
      BOOST_PP_ENUM(n, SG_CALL_ARG, 0)
    };
#else
    naRef* args = NULL;
#endif

    naRef result =
      naCallMethodCtx(ctx, holder->get_naRef(), self, n, args, naNil());

    const char* error = naGetError(ctx);
    std::string error_str(error ? error : "");

    Ret r = Ret();
    if( !error )
      r = from_nasal_helper(ctx, result, static_cast<Ret*>(0));

    naFreeContext(ctx);

    if( error )
      throw std::runtime_error(error_str);

    return r;
  }

  template<
    class Ret
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  typename boost::enable_if<boost::is_void<Ret>, Ret>::type
  callNasalMethod( const ObjectHolder<SGReferenced>* holder,
                   Me self
                   BOOST_PP_ENUM_TRAILING(n, SG_CALL_TRAITS_PARAM, 0) )
  {
    callNasalMethod<
      naRef // do not do any conversion and just ignore the return value
            // TODO warn if something different to nil is returned?
      BOOST_PP_COMMA_IF(n)
      BOOST_PP_ENUM_PARAMS(n, A)
    >
    (
      holder,
      self
      BOOST_PP_ENUM_TRAILING_PARAMS(n, a)
    );
  }

# undef SG_CALL_TRAITS_PARAM
# undef SG_CALL_ARG
#endif

  template<
    class Ret
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  typename boost::disable_if<
    // free function if first argument is not nasal::Me or no argument at all
    boost::is_same<BOOST_PP_IF(n, A0, void), Me>,
    boost::function<Ret (BOOST_PP_ENUM_PARAMS(n, A))>
  >::type
  boostFunctionFromNasal(naRef code, Ret (*)(BOOST_PP_ENUM_PARAMS(n, A)))
#ifdef SG_BOOST_FUNCTION_FROM_NASAL_FWD
  ;
#else
  {
    return boost::bind
    (
      &callNasalMethod<Ret BOOST_PP_ENUM_TRAILING_PARAMS(n, A)>,
      ObjectHolder<SGReferenced>::makeShared(code),
      boost::bind(naNil)
      BOOST_PP_COMMA_IF(n)
      BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), _)
    );
  }
#endif

#if n > 0
  template<
    class Ret
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  typename boost::enable_if<
    // method if type of first argument is nasal::Me
    boost::is_same<A0, Me>,
    boost::function<Ret (BOOST_PP_ENUM_PARAMS(n, A))>
  >::type
  boostFunctionFromNasal(naRef code, Ret (*)(BOOST_PP_ENUM_PARAMS(n, A)))
#ifdef SG_BOOST_FUNCTION_FROM_NASAL_FWD
  ;
#else
  {
    return boost::bind
    (
      &callNasalMethod<
        Ret
        BOOST_PP_COMMA_IF(BOOST_PP_DEC(n))
        BOOST_PP_ENUM_SHIFTED_PARAMS(n, A)
      >,
      ObjectHolder<SGReferenced>::makeShared(code),
      BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), _)
    );
  }
#endif
#endif

#undef n
#endif // SG_DONT_DO_ANYTHING
