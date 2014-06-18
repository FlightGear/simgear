/// Unit tests for function inside strutils package
#define BOOST_TEST_MODULE misc
#include <BoostTestTargetConfig.h>

#include <simgear/compiler.h>
#include "strutils.hxx"

namespace strutils = simgear::strutils;

BOOST_AUTO_TEST_CASE( strutils_functions )
{
  std::string a("abcd");
  BOOST_CHECK_EQUAL(strutils::strip(a), a);
  BOOST_CHECK_EQUAL(strutils::strip(" a "), "a");
  BOOST_CHECK_EQUAL(strutils::lstrip(" a  "), "a  ");
  BOOST_CHECK_EQUAL(strutils::rstrip("\ta "), "\ta");

  // check internal spacing is preserved
  BOOST_CHECK_EQUAL(strutils::strip("\t \na \t b\r \n "), "a \t b");


  BOOST_CHECK(strutils::starts_with("banana", "ban"));
  BOOST_CHECK(!strutils::starts_with("abanana", "ban"));
  BOOST_CHECK(strutils::starts_with("banana", "banana")); // pass - string starts with itself
  BOOST_CHECK(!strutils::starts_with("ban", "banana")); // fail - original string is prefix of

  BOOST_CHECK(strutils::ends_with("banana", "ana"));
  BOOST_CHECK(strutils::ends_with("foo.text", ".text"));
  BOOST_CHECK(!strutils::ends_with("foo.text", ".html"));

  BOOST_CHECK_EQUAL(strutils::simplify("\ta\t b  \nc\n\r \r\n"), "a b c");
  BOOST_CHECK_EQUAL(strutils::simplify("The quick  - brown dog!"), "The quick - brown dog!");
  BOOST_CHECK_EQUAL(strutils::simplify("\r\n  \r\n   \t  \r"), "");

  BOOST_CHECK_EQUAL(strutils::to_int("999"), 999);
  BOOST_CHECK_EQUAL(strutils::to_int("0000000"), 0);
  BOOST_CHECK_EQUAL(strutils::to_int("-10000"), -10000);

  string_list la = strutils::split("zero one two three four five");
  BOOST_CHECK_EQUAL(la[2], "two");
  BOOST_CHECK_EQUAL(la[5], "five");
  BOOST_CHECK_EQUAL(la.size(), 6);

  string_list lb = strutils::split("alpha:beta:gamma:delta", ":", 2);
  BOOST_CHECK_EQUAL(lb.size(), 3);
  BOOST_CHECK_EQUAL(lb[0], "alpha");
  BOOST_CHECK_EQUAL(lb[1], "beta");
  BOOST_CHECK_EQUAL(lb[2], "gamma:delta");

  std::string j = strutils::join(la, "&");
  BOOST_CHECK_EQUAL(j, "zero&one&two&three&four&five");

  BOOST_CHECK_EQUAL(strutils::unescape("\\ \\n\\t\\x41\\117a"), " \n\tAOa");
}

BOOST_AUTO_TEST_CASE( compare_versions )
{
  BOOST_CHECK_LT(strutils::compare_versions("1.0.12", "1.1"), 0);
  BOOST_CHECK_GT(strutils::compare_versions("1.1", "1.0.12"), 0);
  BOOST_CHECK_EQUAL(strutils::compare_versions("10.6.7", "10.6.7"), 0);
  BOOST_CHECK_LT(strutils::compare_versions("2.0", "2.0.99"), 0);
  BOOST_CHECK_EQUAL(strutils::compare_versions("99", "99"), 0);
  BOOST_CHECK_GT(strutils::compare_versions("99", "98"), 0);

  // since we compare numerically, leasing zeros shouldn't matter
  BOOST_CHECK_EQUAL(strutils::compare_versions("0.06.7", "0.6.07"), 0);
}

BOOST_AUTO_TEST_CASE( md5_hex )
{
  // hex encoding
  unsigned char raw_data[] = {0x0f, 0x1a, 0xbc, 0xd2, 0xe3, 0x45, 0x67, 0x89};
  const std::string& hex_data =
    strutils::encodeHex(raw_data, sizeof(raw_data)/sizeof(raw_data[0]));
  BOOST_REQUIRE_EQUAL(hex_data, "0f1abcd2e3456789");
  BOOST_REQUIRE_EQUAL(strutils::encodeHex("abcde"), "6162636465");

  // md5
  BOOST_CHECK_EQUAL(strutils::md5("test"), "098f6bcd4621d373cade4e832627b4f6");
}
