#define BOOST_TEST_MODULE nasal
#include <BoostTestTargetConfig.h>

#include "TestContext.hxx"

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
  
  BOOST_CHECK_CLOSE((c.*test_double)("2.000000953656983160"),
  2.000000953656983160, 1e-5);
  /* this value has bit pattern 0x400000007fff6789L,
  * so will look like a pointer if the endianness is set wrong
  * (see naref.h, data.h)*/
}

BOOST_AUTO_TEST_CASE( parse_num )
{
  runNumTests(&TestContext::convert<double>, &TestContext::convert<int>);
}

BOOST_AUTO_TEST_CASE( lex_num )
{
  runNumTests(&TestContext::exec<double>, &TestContext::exec<int>);
}
