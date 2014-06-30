#ifndef SG_FUNCTION_LIST_HXX_
# error function_list - do not include this file!
#endif

#ifndef SG_DONT_DO_ANYTHING
#define n BOOST_PP_ITERATION()
#define SG_FUNC_TYPE boost::function<Ret (BOOST_PP_ENUM_PARAMS(n, A))>
#define SG_LIST_TYPE std::vector<SG_FUNC_TYPE >

template<class Ret BOOST_PP_ENUM_TRAILING_PARAMS(n, class A)>
class function_list<Ret (BOOST_PP_ENUM_PARAMS(n, A))>:
  public SG_LIST_TYPE
{
  public:
    typedef SG_FUNC_TYPE function_type;
    typedef typename SG_LIST_TYPE::iterator iterator;
    typedef typename SG_LIST_TYPE::const_iterator const_iterator;

    Ret operator()(BOOST_PP_ENUM_BINARY_PARAMS(n, A, a)) const
    {
      if( this->empty() )
        return Ret();

      const_iterator list_end = --this->end();
      for(const_iterator f = this->begin(); f != list_end; ++f)
        if( *f )
          (*f)(BOOST_PP_ENUM_PARAMS(n, a));

      return (*list_end) ? (*list_end)(BOOST_PP_ENUM_PARAMS(n, a)) : Ret();
    }
};

#undef n
#undef SG_FUNC_TYPE
#undef SG_LIST_TYPE

#endif // SG_DONT_DO_ANYTHING
