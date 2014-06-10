/// Unit tests for utf8ToLatin1 conversion function
#define BOOST_TEST_MODULE misc
#include <BoostTestTargetConfig.h>

#include "strutils.hxx"
#include <string>

BOOST_AUTO_TEST_CASE( utf8_latin1_conversion )
{
  std::string utf8_string1 = "Zweibr\u00FCcken";
  //valid UTF-8, convertible to Latin-1
  std::string latin1_string1 = "Zweibr\374cken";
  //Latin-1, not valid UTF-8
  std::string utf8_string2 = "\u600f\U00010143";
  //valid UTF-8, out of range for Latin-1

  std::string output_string1u = simgear::strutils::utf8ToLatin1(utf8_string1);
  BOOST_CHECK_EQUAL(output_string1u, latin1_string1);

  std::string output_string1l = simgear::strutils::utf8ToLatin1(latin1_string1);
  BOOST_CHECK_EQUAL(output_string1l, latin1_string1);

  std::string output_string3 = simgear::strutils::utf8ToLatin1(utf8_string2);
  //we don't check the result of this one as there is no right answer,
  //just make sure it doesn't crash/hang
}
