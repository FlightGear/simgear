// -*- coding: utf-8 -*-
//
// Unit tests for functions inside the strutils package

#include <string>
#include <vector>
#include <utility>              // std::move()
#include <fstream>              // std::ifstream
#include <cerrno>
#include <cstdlib>              // _set_errno() on Windows

#include <simgear/misc/test_macros.hxx>
#include <simgear/compiler.h>
#include <simgear/misc/strutils.hxx>

using std::string;
using std::vector;

namespace strutils = simgear::strutils;

void test_strip()
{
  string a("abcd");
  SG_CHECK_EQUAL(strutils::strip(a), a);
  SG_CHECK_EQUAL(strutils::strip(" a "), "a");
  SG_CHECK_EQUAL(strutils::lstrip(" a  "), "a  ");
  SG_CHECK_EQUAL(strutils::rstrip("\ta "), "\ta");

  // Check internal spacing is preserved
  SG_CHECK_EQUAL(strutils::strip("\t \na \t b\r \n "), "a \t b");
}

void test_stripTrailingNewlines()
{
  SG_CHECK_EQUAL(strutils::stripTrailingNewlines("\rfoobar\n\r\n\n\r\r"),
                 "\rfoobar");
  SG_CHECK_EQUAL(strutils::stripTrailingNewlines("\rfoobar\r"), "\rfoobar");
  SG_CHECK_EQUAL(strutils::stripTrailingNewlines("\rfoobar"), "\rfoobar");
  SG_CHECK_EQUAL(strutils::stripTrailingNewlines(""), "");
}

void test_stripTrailingNewlines_inplace()
{
  string s = "\r\n\r\rfoo\n\r\rbar\n\r\n\r\r\n\r\r";
  strutils::stripTrailingNewlines_inplace(s);
  SG_CHECK_EQUAL(s, "\r\n\r\rfoo\n\r\rbar");

  s = "\rfoobar\r";
  strutils::stripTrailingNewlines_inplace(s);
  SG_CHECK_EQUAL(s, "\rfoobar");

  s = "\rfoobar";
  strutils::stripTrailingNewlines_inplace(s);
  SG_CHECK_EQUAL(s, "\rfoobar");

  s = "";
  strutils::stripTrailingNewlines_inplace(s);
  SG_CHECK_EQUAL(s, "");
}

void test_starts_with()
{
  SG_VERIFY(strutils::starts_with("banana", "ban"));
  SG_VERIFY(!strutils::starts_with("abanana", "ban"));
  // Pass - string starts with itself
  SG_VERIFY(strutils::starts_with("banana", "banana"));
  // Fail - original string is prefix of
  SG_VERIFY(!strutils::starts_with("ban", "banana"));
}

void test_ends_with()
{
  SG_VERIFY(strutils::ends_with("banana", "ana"));
  SG_VERIFY(strutils::ends_with("foo.text", ".text"));
  SG_VERIFY(!strutils::ends_with("foo.text", ".html"));
}

void test_simplify()
{
  SG_CHECK_EQUAL(strutils::simplify("\ta\t b  \nc\n\r \r\n"), "a b c");
  SG_CHECK_EQUAL(strutils::simplify("The quick  - brown dog!"),
                 "The quick - brown dog!");
  SG_CHECK_EQUAL(strutils::simplify("\r\n  \r\n   \t  \r"), "");
}

void test_to_int()
{
  SG_CHECK_EQUAL(strutils::to_int("999"), 999);
  SG_CHECK_EQUAL(strutils::to_int("0000000"), 0);
  SG_CHECK_EQUAL(strutils::to_int("-10000"), -10000);
}

void test_split()
{
  string_list l = strutils::split("zero one two three four five");
  SG_CHECK_EQUAL(l[2], "two");
  SG_CHECK_EQUAL(l[5], "five");
  SG_CHECK_EQUAL(l.size(), 6);

  string j = strutils::join(l, "&");
  SG_CHECK_EQUAL(j, "zero&one&two&three&four&five");

  l = strutils::split("alpha:beta:gamma:delta", ":", 2);
  SG_CHECK_EQUAL(l.size(), 3);
  SG_CHECK_EQUAL(l[0], "alpha");
  SG_CHECK_EQUAL(l[1], "beta");
  SG_CHECK_EQUAL(l[2], "gamma:delta");

  l = strutils::split("", ",");
  SG_CHECK_EQUAL(l.size(), 1);
  SG_CHECK_EQUAL(l[0], "");

  l = strutils::split(",", ",");
  SG_CHECK_EQUAL(l.size(), 2);
  SG_CHECK_EQUAL(l[0], "");
  SG_CHECK_EQUAL(l[1], "");

  l = strutils::split(",,", ",");
  SG_CHECK_EQUAL(l.size(), 3);
  SG_CHECK_EQUAL(l[0], "");
  SG_CHECK_EQUAL(l[1], "");
  SG_CHECK_EQUAL(l[2], "");

  l = strutils::split("  ", ",");
  SG_CHECK_EQUAL(l.size(), 1);
  SG_CHECK_EQUAL(l[0], "  ");


    const char* testCases[] = {
        "alpha,bravo, charlie\tdelta\n\recho,    \t\r\nfoxtrot golf,\n\t\n \n",
        "  alpha  bravo \t charlie,,,delta    echo\nfoxtrot\rgolf"
    };

    for (const char* data: testCases) {
        l = strutils::split_on_any_of(data, "\n\t\r ,");
        SG_CHECK_EQUAL(l.size(), 7);
        SG_CHECK_EQUAL(l[0], "alpha");
        SG_CHECK_EQUAL(l[1], "bravo");
        SG_CHECK_EQUAL(l[2], "charlie");
        SG_CHECK_EQUAL(l[3], "delta");
        SG_CHECK_EQUAL(l[4], "echo");
        SG_CHECK_EQUAL(l[5], "foxtrot");
        SG_CHECK_EQUAL(l[6], "golf");
    }
}

void test_escape()
{
  SG_CHECK_EQUAL(strutils::escape(""), "");
  SG_CHECK_EQUAL(strutils::escape("\\"), "\\\\");
  SG_CHECK_EQUAL(strutils::escape("\""), "\\\"");
  SG_CHECK_EQUAL(strutils::escape("\\n"), "\\\\n");
  SG_CHECK_EQUAL(strutils::escape("n\\"), "n\\\\");
  SG_CHECK_EQUAL(strutils::escape(" ab\nc \\def\t\r \\ ghi\\"),
                 " ab\\nc \\\\def\\t\\r \\\\ ghi\\\\");
  // U+0152 is LATIN CAPITAL LIGATURE OE. The last word is Egg translated in
  // French and encoded in UTF-8 ('Œuf' if you can read UTF-8).
  SG_CHECK_EQUAL(strutils::escape("Un \"Bel\" '\u0152uf'"),
                 "Un \\\"Bel\\\" '\\305\\222uf'");
  SG_CHECK_EQUAL(strutils::escape("\a\b\f\n\r\t\v"),
                 "\\a\\b\\f\\n\\r\\t\\v");

  // Test with non-printable characters
  //
  // - 'prefix' is an std::string that *contains* a NUL character.
  // - \012 is \n (LINE FEED).
  // - \037 (\x1F) is the last non-printable ASCII character before \040 (\x20),
  //   which is the space.
  // - \176 (\x7E) is '~', the last printable ASCII character.
  // - \377 is \xFF. Higher char values (> 255) are not faithfully encoded by
  //   strutils::escape(): only the lowest 8 bits are used; higher-order bits
  //   are ignored (for people who use chars with more than 8 bits...).
  const string prefix = string("abc") + '\000';
  SG_CHECK_EQUAL(strutils::escape(prefix +
                                  "\003def\012\037\040\176\177\376\377"),
                 "abc\\000\\003def\\n\\037 ~\\177\\376\\377");

  SG_CHECK_EQUAL(strutils::escape(" \n\tAOa"), " \\n\\tAOa");
}

void test_unescape()
{
  SG_CHECK_EQUAL(strutils::unescape("\\ \\n\\t\\x41\\117a"), " \n\tAOa");
  // Two chars: '\033' (ESC) followed by '2'
  SG_CHECK_EQUAL(strutils::unescape("\\0332"), "\0332");
  // Hex escapes have no length limit and terminate at the first character
  // that is not a valid hexadecimal digit.
  SG_CHECK_EQUAL(strutils::unescape("\\x00020|"), " |");
  SG_CHECK_EQUAL(strutils::unescape("\\xA"), "\n");
  SG_CHECK_EQUAL(strutils::unescape("\\xA-"), "\n-");
}

void aux_escapeAndUnescapeRoundTripTest(const string& testString)
{
  SG_CHECK_EQUAL(strutils::unescape(strutils::escape(testString)), testString);
}

void test_escapeAndUnescapeRoundTrips()
{
  // "\0332" contains two chars: '\033' (ESC) followed by '2'.
  // Ditto for "\0402": it's a space ('\040') followed by a '2'.
  vector<string> stringsToTest(
    {"", "\\", "\n", "\\\\", "\"\'\?\t\rAG\v\a \b\f\\", "\x23\xf8",
     "\0332", "\0402", "\uab42", "\U12345678"});

  const string withBinary = (string("abc") + '\000' +
                             "\003def\012\037\040\176\177\376\377");
  stringsToTest.push_back(std::move(withBinary));

  for (const string& s: stringsToTest) {
    aux_escapeAndUnescapeRoundTripTest(s);
  }
}

void test_compare_versions()
{
  SG_CHECK_LT(strutils::compare_versions("1.0.12", "1.1"), 0);
  SG_CHECK_GT(strutils::compare_versions("1.1", "1.0.12"), 0);
  SG_CHECK_EQUAL(strutils::compare_versions("10.6.7", "10.6.7"), 0);
  SG_CHECK_LT(strutils::compare_versions("2.0", "2.0.99"), 0);
  SG_CHECK_EQUAL(strutils::compare_versions("99", "99"), 0);
  SG_CHECK_GT(strutils::compare_versions("99", "98"), 0);

  // Since we compare numerically, leading zeros shouldn't matter
  SG_CHECK_EQUAL(strutils::compare_versions("0.06.7", "0.6.07"), 0);


  SG_CHECK_EQUAL(strutils::compare_versions("10.6.7", "10.6.8", 2), 0);
  SG_CHECK_GT(strutils::compare_versions("10.7.7", "10.6.8", 2), 0);
  SG_CHECK_EQUAL(strutils::compare_versions("10.8.7", "10.6.8", 1), 0);
}

void test_md5_hex()
{
  // hex encoding
  unsigned char raw_data[] = {0x0f, 0x1a, 0xbc, 0xd2, 0xe3, 0x45, 0x67, 0x89};
  const string& hex_data =
    strutils::encodeHex(raw_data, sizeof(raw_data)/sizeof(raw_data[0]));
  SG_CHECK_EQUAL(hex_data, "0f1abcd2e3456789");
  SG_CHECK_EQUAL(strutils::encodeHex("abcde"), "6162636465");

  // md5
  SG_CHECK_EQUAL(strutils::md5("test"), "098f6bcd4621d373cade4e832627b4f6");
}

void test_propPathMatch()
{
    const char* testTemplate1 = "/sim[*]/views[*]/render";
    SG_VERIFY(strutils::matchPropPathToTemplate("/sim[0]/views[50]/render-buildings[0]", testTemplate1));
    SG_VERIFY(strutils::matchPropPathToTemplate("/sim[1]/views[0]/rendering-enabled", testTemplate1));

    SG_VERIFY(!strutils::matchPropPathToTemplate("/sim[0]/views[50]/something-else", testTemplate1));
    SG_VERIFY(!strutils::matchPropPathToTemplate("/sim[0]/gui[0]/wibble", testTemplate1));

    // test explicit index matching
    const char* testTemplate2 = "/view[5]/*";
    SG_VERIFY(!strutils::matchPropPathToTemplate("/view[2]/render-buildings[0]", testTemplate2));
    SG_VERIFY(!strutils::matchPropPathToTemplate("/sim[1]/foo", testTemplate2));
    SG_VERIFY(!strutils::matchPropPathToTemplate("/view[50]/foo", testTemplate2));
    SG_VERIFY(!strutils::matchPropPathToTemplate("/view[55]/foo", testTemplate2));

    SG_VERIFY(strutils::matchPropPathToTemplate("/view[5]/foo", testTemplate2));
    SG_VERIFY(strutils::matchPropPathToTemplate("/view[5]/child[3]/bar", testTemplate2));


    const char* testTemplate3 = "/*[*]/fdm*[*]/aero*";

    SG_VERIFY(strutils::matchPropPathToTemplate("/position[2]/fdm-jsb[0]/aerodynamic", testTemplate3));
    SG_VERIFY(!strutils::matchPropPathToTemplate("/position[2]/foo[0]/aerodynamic", testTemplate3));
}

void test_error_string()
{
#if defined(_WIN32)
  _set_errno(0);
#else
  errno = 0;
#endif

  std::ifstream f("/\\/non-existent/file/a8f7bz97-3ffe-4f5b-b8db-38ccurJL-");

#if defined(_WIN32)
  errno_t saved_errno = errno;
#else
  int saved_errno = errno;
#endif

  SG_VERIFY(!f.is_open());
  SG_CHECK_NE(saved_errno, 0);
  SG_CHECK_GT(strutils::error_string(saved_errno).size(), 0);
}

int main(int argc, char* argv[])
{
    test_strip();
    test_stripTrailingNewlines();
    test_stripTrailingNewlines_inplace();
    test_starts_with();
    test_ends_with();
    test_simplify();
    test_to_int();
    test_split();
    test_escape();
    test_unescape();
    test_escapeAndUnescapeRoundTrips();
    test_compare_versions();
    test_md5_hex();
    test_error_string();
    test_propPathMatch();

    return EXIT_SUCCESS;
}
