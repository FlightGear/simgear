#ifndef SG_NASAL_OBJECT_HXX_
# error Nasal cppbind - do not include this file!
#endif

#ifndef SG_DONT_DO_ANYTHING
#define n BOOST_PP_ITERATION()

#define SG_CALL_ARG(z, n, dummy)\
      to_nasal<typename boost::call_traits<A##n>::param_type>(ctx, a##n)

  template<
    class Ret
    BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)
  >
  Ret callMethod( const std::string& name
                  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, A, a) )
  {
    if( !_nasal_impl.valid() )
      return Ret();

    typedef boost::function<Ret (nasal::Me BOOST_PP_ENUM_TRAILING_PARAMS(n, A))>
            MemFunc;

    Context ctx;
    MemFunc f = get_member<MemFunc>(ctx, _nasal_impl.get_naRef(), name.c_str());
    if( f )
      return f(nasal::to_nasal(ctx, this) BOOST_PP_ENUM_TRAILING_PARAMS(n, a));

    return Ret();
  }

#undef SG_CALL_ARG

#undef n
#endif // SG_DONT_DO_ANYTHING
