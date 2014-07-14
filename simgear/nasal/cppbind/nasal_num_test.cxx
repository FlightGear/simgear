#define BOOST_TEST_MODULE cppbind
#include <BoostTestTargetConfig.h>

#include "NasalCallContext.hxx"

class TestContext:
  public nasal::CallContext
{
  public:
    TestContext():
      CallContext(naNewContext(), 0, 0)
    {}

    ~TestContext()
    {
      naFreeContext(c);
    }

    template<class T>
    T from_str(const std::string& str)
    {
      return from_nasal<T>(to_nasal(str));
    }

    naRef exec(const std::string& code_str, nasal::Me me)
    {
      int err_line = -1;
      naRef code = naParseCode( c, to_nasal("<TextContext::exec>"), 0,
                                (char*)code_str.c_str(), code_str.length(),
                                &err_line );
      if( !naIsCode(code) )
        throw std::runtime_error("Failed to parse code: " + code_str);

      return naCallMethod(code, me, 0, 0, naNil());
    }

    template<class T>
    T exec(const std::string& code)
    {
      return from_nasal<T>(exec(code, naNil()));
    }

    template<class T>
    T convert(const std::string& str)
    {
      return from_nasal<T>(to_nasal(str));
    }
};

static void runNumTests( double (TestContext::*test_double)(const std::string&),
                         int (TestContext::*test_int)(const std::string&) )
{
  TestContext c;

  BOOST_CHECK_CLOSE((c.*test_double)("0.5"), 0.5, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)(".6"),  0.6, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("-.7"), -0.7, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("-0.8"), -0.8, 1e-5);
  BOOST_CHECK_SMALL((c.*test_double)("0.0"), 1e-5);
  BOOST_CHECK_SMALL((c.*test_double)("-.0"), 1e-5);

  BOOST_CHECK_CLOSE((c.*test_double)("1.23e4"),  1.23e4, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("1.23e-4"), 1.23e-4, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("-1.23e4"),  -1.23e4, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("-1.23e-4"), -1.23e-4, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("1e-4"), 1e-4, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_double)("-1e-4"), -1e-4, 1e-5);

  BOOST_CHECK_EQUAL((c.*test_int)("123"), 123);
  BOOST_CHECK_EQUAL((c.*test_int)("-958"), -958);

  BOOST_CHECK_CLOSE((c.*test_int)("-1e7"), -1e7, 1e-5);
  BOOST_CHECK_CLOSE((c.*test_int)("2E07"), 2e07, 1e-5);

  BOOST_CHECK_EQUAL((c.*test_int)("0755"), 755);
  BOOST_CHECK_EQUAL((c.*test_int)("0055"), 55);
  BOOST_CHECK_EQUAL((c.*test_int)("-0155"), -155);

  BOOST_CHECK_EQUAL((c.*test_int)("0o755"), 0755);
  BOOST_CHECK_EQUAL((c.*test_int)("0o055"), 055);
  BOOST_CHECK_EQUAL((c.*test_int)("-0o155"), -0155);

  BOOST_CHECK_EQUAL((c.*test_int)("0x755"), 0x755);
  BOOST_CHECK_EQUAL((c.*test_int)("0x055"), 0x55);
  BOOST_CHECK_EQUAL((c.*test_int)("-0x155"), -0x155);
}

BOOST_AUTO_TEST_CASE( parse_num )
{
  runNumTests(&TestContext::convert<double>, &TestContext::convert<int>);
}

BOOST_AUTO_TEST_CASE( lex_num )
{
  runNumTests(&TestContext::exec<double>, &TestContext::exec<int>);
}
