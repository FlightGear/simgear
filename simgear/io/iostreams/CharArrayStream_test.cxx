// -*- coding: utf-8 -*-
//
// CharArrayStream_test.cxx --- Automated tests for CharArrayStream.cxx
//
// Copyright (C) 2017  Florent Rougon
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA.

#include <simgear_config.h>

#include <ios>                  // std::basic_ios, std::streamsize...
#include <iostream>             // std::ios_base, std::cerr, etc.
#include <sstream>
#include <limits>               // std::numeric_limits
#include <type_traits>          // std::make_unsigned()
#include <memory>               // std::unique_ptr
#include <cassert>
#include <cstdlib>              // EXIT_SUCCESS
#include <cstddef>              // std::size_t
#include <cstring>              // std::strlen()
#include <algorithm>            // std::fill_n()
#include <vector>

#include <simgear/misc/test_macros.hxx>
#include "CharArrayStream.hxx"

using std::string;
using std::cout;
using std::cerr;
using traits = std::char_traits<char>;

typedef typename std::make_unsigned<std::streamsize>::type uStreamSize;


// Safely convert a non-negative std::streamsize into an std::size_t. If
// impossible, bail out.
static std::size_t streamsizeToSize_t(std::streamsize n)
{
  SG_CHECK_GE(n, 0);
  SG_CHECK_LE(static_cast<uStreamSize>(n),
              std::numeric_limits<std::size_t>::max());

  return static_cast<std::size_t>(n);
}

void test_CharArrayStreambuf_basicOperations()
{
  cerr << "Testing basic operations on CharArrayStreambuf\n";

  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  std::istringstream text_ss(text);
  string canary = "YoY";
  // Reserve space for our little canary
  const std::size_t bufSize = text.size() + canary.size();
  std::unique_ptr<char[]> buf(new char[bufSize]);
  int ch;
  std::streamsize n;
  std::streampos pos;

  // Only allow arraySBuf to read from, and write to the first text.size()
  // chars of 'buf'
  simgear::CharArrayStreambuf arraySBuf(&buf[0], text.size());

  // 1) Write a copy of the 'text' string at buf.get(), testing various write
  //    and seek methods.
  //    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Write "01" with two sputc() calls
  ch = arraySBuf.sputc(text[0]);
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '0' && buf[0] == '0');
  ch = arraySBuf.sputc(text[1]);
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '1' && buf[1] == '1');
  // Write the 34 following chars of 'text' with one sputn() call
  n = arraySBuf.sputn(&text[2], 34);
  SG_CHECK_EQUAL(n, 34);
  SG_CHECK_EQUAL(string(&buf[2], 34), "23456789abcdefghijklmnopqrstuvwxyz");

  // Indirect test of seekpos(): position the write stream pointer a bit further
  pos = arraySBuf.pubseekpos(43, std::ios_base::out);
  SG_CHECK_EQUAL(pos, std::streampos(43));
  // Write 7 more chars with sputn()
  n = arraySBuf.sputn(&text[43], 7);
  SG_CHECK_EQUAL(n, 7);
  SG_CHECK_EQUAL(string(&buf[43], 7), "\nGHIJK ");
  // Indirect test of seekoff(): seek backwards relatively to the current write
  // pointer position
  pos = arraySBuf.pubseekoff(-std::streamoff(std::strlen("\nABCDEF\nGHIJK ")),
                             std::ios_base::cur, std::ios_base::out);
  // 10 + 26, i.e., after the lowercase alphabet
  SG_CHECK_EQUAL(pos, std::streampos(36));
  // Write "\nABCD" to buf in one sputn() call
  n = arraySBuf.sputn(&text[36], 5);
  // Now write "EF" in two sputc() calls
  ch = arraySBuf.sputc(text[41]);
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'E' && buf[41] == 'E');
  ch = arraySBuf.sputc(text[42]);
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'F' && buf[42] == 'F');

  // Place a canary to check that arraySBuf doesn't write beyond the end of buf
  std::copy(canary.begin(), canary.end(), &buf[text.size()]);

  // Check seeking from arraySBuf's end (which is at offset text.size(), *not*
  // bufSize: cf. the construction of arraySBuf!).
  pos = arraySBuf.pubseekoff(-std::streamoff(std::strlen("LMNOPQ")),
                             std::ios_base::end, std::ios_base::out);
  SG_CHECK_EQUAL(pos, std::streampos(text.size() - std::strlen("LMNOPQ")));
  // Write "LMNOPQ" to buf in one sputn() call. The other characters won't be
  // written, because they would go past the end of the buffer managed by
  // 'arraySBuf' (i.e., the first text.size() chars of 'buf').
  static const char someChars[] = "LMNOPQ+buffer overrun that will be blocked";
  n = arraySBuf.sputn(someChars, sizeof(someChars));

  // Check the number of chars actually written
  SG_CHECK_EQUAL(n, std::strlen("LMNOPQ"));
  // Check that our canary starting at buf[text.size()] is still there and
  // intact
  SG_CHECK_EQUAL(string(&buf[text.size()], canary.size()), canary);
  // Check that we now have an exact copy of 'text' in the managed buffer
  SG_CHECK_EQUAL(string(&buf[0], text.size()), text);

  // 2) Read back the copy of 'text' in 'buf', using various read and seek
  //    methods.
  //    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  ch = arraySBuf.sgetc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '0');
  ch = arraySBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '0');
  ch = arraySBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '1');
  ch = arraySBuf.snextc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '3');
  ch = arraySBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '3');
  ch = arraySBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '4');
  ch = arraySBuf.sputbackc('4');
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '4');
  ch = arraySBuf.sputbackc('u'); // doesn't match what we read from the stream
  SG_VERIFY(ch == EOF);
  ch = arraySBuf.sputbackc('3'); // this one does
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '3');

  static constexpr std::streamsize buf2Size = 10;
  char buf2[buf2Size];
  // Most efficient way (with the underlying xsgetn()) to read several chars
  // at once.
  n = arraySBuf.sgetn(buf2, buf2Size);
  SG_CHECK_EQUAL(n, buf2Size);
  SG_CHECK_EQUAL(string(buf2, static_cast<std::size_t>(buf2Size)),
                 "3456789abc");

  ch = arraySBuf.sungetc(); // same as sputbackc(), except no value to check
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'c');
  ch = arraySBuf.sungetc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'b');
  ch = arraySBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'b');
  ch = arraySBuf.sputbackc('b'); // this one does
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'b');

  n = arraySBuf.sgetn(buf2, buf2Size);
  SG_CHECK_EQUAL(n, buf2Size);
  SG_CHECK_EQUAL(string(buf2, static_cast<std::size_t>(buf2Size)),
                 "bcdefghijk");

  ch = arraySBuf.sungetc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'k');

  static char buf3[64];
  n = arraySBuf.sgetn(buf3, sizeof(buf3));
  SG_CHECK_EQUAL(n, 36);
  SG_CHECK_EQUAL(string(buf3, 36), "klmnopqrstuvwxyz\nABCDEF\nGHIJK LMNOPQ");

  SG_CHECK_EQUAL(arraySBuf.sbumpc(), EOF);

  // Check we can independently set the read and write pointers for arraySBuf
  pos = arraySBuf.pubseekpos(10, std::ios_base::in);
  SG_CHECK_EQUAL(pos, std::streampos(10));
  pos = arraySBuf.pubseekpos(13, std::ios_base::out);
  SG_CHECK_EQUAL(pos, std::streampos(13));

  // Write "DEF" where there is currently "def" in 'buf'.
  for (int i = 0; i < 3; i++) {
    char c = 'D' + i;
    ch = arraySBuf.sputc(c);
    SG_VERIFY(ch != EOF && traits::to_char_type(ch) == c && buf[i+13] == c);
  }

  n = arraySBuf.sgetn(buf3, 6);
  SG_CHECK_EQUAL(n, 6);
  SG_CHECK_EQUAL(string(buf3, 6), "abcDEF");

  // Set both stream pointers at once (read and write)
  pos = arraySBuf.pubseekpos(10, std::ios_base::in | std::ios_base::out);
  SG_VERIFY(pos == std::streampos(10));

  // Write "ABC" to buf in one sputn() call
  n = arraySBuf.sputn("ABC", 3);
  SG_CHECK_EQUAL(n, 3);
  SG_CHECK_EQUAL(string(&buf[10], 3), "ABC");

  // Indirect test of seekoff(): seek backwards relatively to the current read
  // pointer position
  pos = arraySBuf.pubseekoff(-3, std::ios_base::cur, std::ios_base::in);
  SG_CHECK_EQUAL(pos, std::streampos(7));

  n = arraySBuf.sgetn(buf3, 12);
  SG_CHECK_EQUAL(n, 12);
  SG_CHECK_EQUAL(string(buf3, 12), "789ABCDEFghi");
}

void test_CharArrayStreambuf_readOrWriteLargestPossibleAmount()
{
  cerr << "Testing reading and writing from/to CharArrayStreambuf with the "
    "largest possible value passed as the 'n' argument for sgetn()/sputn() "
    "(number of chars to read or write)\n";

  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  string canary = "ZaZ";
  // Reserve space for our little canary
  const std::size_t bufSize = text.size() + canary.size();
  std::unique_ptr<char[]> buf(new char[bufSize]);
  std::streamsize n_s;
  std::size_t n;
  std::streampos pos;

  // Place a canary to check that arraySBuf doesn't write beyond the end of buf
  std::copy(canary.begin(), canary.end(), &buf[text.size()]);

  // Only allow arraySBuf to read from, and write to the first text.size()
  // chars of 'buf'
  simgear::CharArrayStreambuf arraySBuf(&buf[0], text.size());

  n_s = arraySBuf.sputn(text.c_str(),
                        std::numeric_limits<std::streamsize>::max());
  // The conversion to std::size_t is safe because arraySBuf.sputn() returns a
  // non-negative value which, in this case, can't exceed the size of the
  // buffer managed by 'arraySBuf', i.e. text.size().
  n = streamsizeToSize_t(n_s);
  SG_CHECK_EQUAL(n, arraySBuf.size());
  SG_CHECK_EQUAL(n, text.size());
  SG_CHECK_EQUAL(string(&buf[0], n), text);

  // Check that our canary starting at &buf[text.size()] is still there and
  // intact
  SG_CHECK_EQUAL(string(&buf[text.size()], canary.size()), canary);

  // The “get” stream pointer is still at the beginning of the buffer managed
  // by 'arraySBuf'. Let's ask for the maximum amount of chars from it to be
  // written to a new buffer, 'buf2'.
  std::unique_ptr<char[]> buf2(new char[text.size()]);
  n_s = arraySBuf.sgetn(&buf2[0],
                        std::numeric_limits<std::streamsize>::max());
  // The conversion to std::size_t is safe because arraySBuf.sgetn() returns a
  // non-negative value which, in this case, can't exceed the size of the
  // buffer managed by 'arraySBuf', i.e. text.size().
  n = streamsizeToSize_t(n_s);
  SG_CHECK_EQUAL(n, arraySBuf.size());
  SG_CHECK_EQUAL(string(&buf2[0], n), text);

  SG_CHECK_EQUAL(arraySBuf.sbumpc(), EOF);
}

void test_CharArrayIStream_simple()
{
  // This also tests ROCharArrayStreambuf, since it is used as
  // CharArrayIStream's stream buffer class.
  cerr << "Testing read operations from CharArrayIStream\n";

  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  std::unique_ptr<char[]> buf(new char[text.size()]);
  std::size_t n;

  simgear::CharArrayIStream caStream(&text[0], text.size());
  caStream.exceptions(std::ios_base::badbit); // throw if badbit is set

  SG_CHECK_EQUAL(caStream.data(), &text[0]);
  SG_CHECK_EQUAL(caStream.size(), text.size());

  SG_VERIFY(caStream.get(buf[0])); // get pointer = 1
  SG_CHECK_EQUAL(buf[0], text[0]);

  caStream.putback(buf[0]);        // get pointer = 0
  SG_CHECK_EQUAL(caStream.get(), traits::to_int_type(text[0])); // get ptr = 1

  // std::iostream::operator bool() will return false due to EOF being reached
  SG_VERIFY(!caStream.read(&buf[1],
                           std::numeric_limits<std::streamsize>::max()));
  // If badbit had been set, it would have caused an exception to be raised
  SG_VERIFY(caStream.eof() && caStream.fail() && !caStream.bad());

  // The conversion to std::size_t is safe because caStream.gcount() returns a
  // non-negative value which, in this case, can't exceed the size of the
  // buffer managed by caStream's associated stream buffer, i.e. text.size().
  n = streamsizeToSize_t(caStream.gcount());
  SG_CHECK_EQUAL(n, caStream.size() - 1);
  SG_CHECK_EQUAL(string(caStream.data(), caStream.size()), text);

  SG_CHECK_EQUAL(caStream.get(), EOF);
  SG_VERIFY(caStream.eof() && caStream.fail() && !caStream.bad());

  // Test stream extraction: operator>>()
  caStream.clear();             // clear the error state flags
  SG_VERIFY(caStream.seekg(0)); // rewind
  std::vector<string> expectedWords = {
    "0123456789abcdefghijklmnopqrstuvwxyz",
    "ABCDEF",
    "GHIJK",
    "LMNOPQ"
  };
  string str;

  for (int i = 0; caStream >> str; i++) {
    SG_CHECK_EQUAL(str, expectedWords[i]);
  }

  SG_VERIFY(caStream.eof() && caStream.fail() && !caStream.bad());
}

void test_CharArrayOStream_simple()
{
  cerr << "Testing write operations to CharArrayOStream\n";

  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  // One could also use an std::vector<char>, but then beware of reallocations!
  std::unique_ptr<char[]> buf(new char[text.size()]);
  std::fill_n(buf.get(), text.size(), '\0'); // to ensure reproducible results

  simgear::CharArrayOStream caStream(&buf[0], text.size());

  SG_CHECK_EQUAL(caStream.data(), &buf[0]);
  SG_CHECK_EQUAL(caStream.size(), text.size());

  SG_VERIFY(caStream.put(text[0])); // buf[0] = text[0], put pointer = 1
  SG_CHECK_EQUAL(buf[0], text[0]);

  SG_VERIFY(caStream.seekp(8)); // put pointer = 8
  // buf[8:23] = text[8:23] (meaning: buf[8] = text[8], ..., buf[22] = text[22])
  SG_VERIFY(caStream.write(&text[8], 15)); // put pointer = 23
  buf[1] = 'X';                 // write garbage to buf[1]
  buf[2] = 'Y';                 // and to buf[2]
  SG_VERIFY(caStream.seekp(-22, std::ios_base::cur)); // put pointer = 23-22 = 1
  SG_VERIFY(caStream.write(&text[1], 7)); // buf[1:8] = text[1:8]

  // The std::ios_base::beg argument is superfluous here---just for testing.
  SG_VERIFY(caStream.seekp(23, std::ios_base::beg)); // put pointer = 23
  // Test stream insertion: operator<<()
  SG_VERIFY(caStream << text.substr(23, 10));
  SG_VERIFY(caStream.write(&text[33], text.size() - 33)); // all that remains
  SG_VERIFY(!caStream.put('Z')); // doesn't fit in caStream's buffer
  SG_VERIFY(caStream.bad());     // put() set the stream's badbit flag

  SG_CHECK_EQUAL(string(caStream.data(), caStream.size()), text);
}

void test_CharArrayIOStream_readWriteSeekPutbackEtc()
{
  cerr << "Testing read, write, seek, putback... from/to CharArrayIOStream\n";

  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  std::unique_ptr<char[]> buf(new char[text.size()]);
  std::size_t n;
  char ch;

  simgear::CharArrayIOStream caStream(&buf[0], text.size());
  caStream.exceptions(std::ios_base::badbit); // throw if badbit is set

  SG_CHECK_EQUAL(caStream.data(), &buf[0]);
  SG_CHECK_EQUAL(caStream.size(), text.size());

  SG_VERIFY(caStream.put(text[0])); // buf[0] = text[0], put pointer = 1
  SG_CHECK_EQUAL(buf[0], text[0]);
  SG_VERIFY(caStream.get(ch)); // read it back from buf, get pointer = 1
  SG_CHECK_EQUAL(ch, text[0]);

  caStream.putback(buf[0]);     // get pointer = 0
  SG_CHECK_EQUAL(caStream.get(), traits::to_int_type(text[0])); // get ptr = 1

  SG_VERIFY(caStream.seekp(5));
  // buf[5:10] = text[5:10] (meaning: buf[5] = text[5], ..., buf[9] = text[9])
  SG_VERIFY(caStream.write(&text[5], 5)); // put pointer = 10
  buf[1] = 'X';                 // write garbage to buf[1]
  buf[2] = 'Y';                 // and to buf[2]
  SG_VERIFY(caStream.seekp(-9, std::ios_base::cur)); // put pointer = 10 - 9 = 1
  SG_VERIFY(caStream.write(&text[1], 4)); // buf[1:5] = text[1:5]

  SG_VERIFY(caStream.seekp(10)); // put pointer = 10
  // buf[10:] = text[10:]
  SG_VERIFY(caStream.write(&text[10], text.size() - 10));

  std::unique_ptr<char[]> buf2(new char[caStream.size() - 10]);
  SG_VERIFY(caStream.seekg(10)); // get pointer = 10
  // std::iostream::operator bool() will return false due to EOF being reached
  SG_VERIFY(!caStream.read(&buf2[0],
                           std::numeric_limits<std::streamsize>::max()));
  // If badbit had been set, it would have caused an exception to be raised
  SG_VERIFY(caStream.eof() && caStream.fail() && !caStream.bad());

  // The conversion to std::size_t is safe because caStream.gcount() returns a
  // non-negative value which, in this case, can't exceed the size of the
  // buffer managed by caStream's associated stream buffer, i.e. text.size().
  n = streamsizeToSize_t(caStream.gcount());
  SG_CHECK_EQUAL(n, caStream.size() - 10);
  SG_CHECK_EQUAL(caStream.get(), EOF);

  SG_CHECK_EQUAL(string(&buf2[0], caStream.size() - 10),
                 string(&text[10], text.size() - 10));
  SG_CHECK_EQUAL(string(caStream.data(), caStream.size()), text);
}

int main(int argc, char** argv)
{
  test_CharArrayStreambuf_basicOperations();
  test_CharArrayStreambuf_readOrWriteLargestPossibleAmount();
  test_CharArrayIStream_simple();
  test_CharArrayOStream_simple();
  test_CharArrayIOStream_readWriteSeekPutbackEtc();

  return EXIT_SUCCESS;
}
