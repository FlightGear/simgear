// -*- coding: utf-8 -*-
//
// zlibstream_test.cxx --- Automated tests for zlibstream.cxx / zlibstream.hxx
//
// Copyright (C) 2017  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <ios>                  // std::basic_ios, std::streamsize...
#include <iostream>             // std::ios_base, std::cerr, etc.
#include <sstream>
#include <array>
#include <random>
#include <limits>               // std::numeric_limits
#include <type_traits>          // std::make_unsigned()
#include <functional>           // std::bind()
#include <cassert>
#include <cstdlib>              // EXIT_SUCCESS
#include <cstddef>              // std::size_t
#include <cstring>              // strcmp()

#include <simgear/misc/test_macros.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/io/iostreams/zlibstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>

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

// In many tests below, I use very small buffer sizes. Of course, this is bad
// for performance. The reason I do it this way is simply because it better
// exercises the code we want to *test* here (we are more likely to find bugs
// if the buffer(s) has/have to be refilled one or more times). Similarly, if
// you don't need the putback feature in non-test code, best performance is
// achieved with putback size = 0.
//
// I suggest you read roundTripWithIStreams() below to see how to use the
// classes most efficiently (especially the comments!).

static std::default_random_engine randomNumbersGenerator;

// Sample string for tests
static const string lipsum = "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque congue ornare\n\
congue. Mauris mollis est et porttitor condimentum. Vivamus laoreet blandit\n\
odio eget consectetur. Etiam quis magna eu enim luctus pretium. In et\n\
tristique nunc, non efficitur metus. Nullam efficitur tristique velit.\n\
Praesent et luctus nunc. Mauris eros eros, rutrum at molestie quis, egestas et\n\
lorem. Ut nulla turpis, eleifend sed mauris ac, faucibus molestie nulla.\n\
Quisque viverra vel turpis nec efficitur. Proin non rutrum velit. Nam sodales\n\
metus felis, eu pharetra velit posuere ut.\n\
\n\
Suspendisse pellentesque tincidunt ligula et pretium. Etiam id justo mauris.\n\
Aenean porta, sapien in suscipit tristique, metus diam malesuada dui, in porta\n\
felis mi non felis. Etiam vel aliquam leo, non vehicula magna. Proin justo\n\
massa, ultrices at porta eu, tempor in ligula. Duis enim ipsum, dictum quis\n\
ultricies et, tempus eu libero. Morbi vulputate libero ut dolor rutrum, a\n\
imperdiet nibh egestas. Phasellus finibus massa vel tempus hendrerit. Nulla\n\
lobortis est non ligula viverra, quis egestas ante hendrerit. Pellentesque\n\
finibus mollis blandit. In ac sollicitudin mauris, eget dignissim mauris.";

// Utility function: generate a random string whose length is in the
// [minLen, maxLen] range.
string randomString(string::size_type minLen, string::size_type maxLen)
{
  std::uniform_int_distribution<string::size_type> sLenDist(minLen, maxLen);
  std::uniform_int_distribution<int> byteDist(0, 255);
  auto randomByte = std::bind(byteDist, randomNumbersGenerator);

  string::size_type len = sLenDist(randomNumbersGenerator);
  string str;

  while (str.size() < len) {
    str += traits::to_char_type(randomByte());
  }

  return str;
}

// Utility function: perform a compression-decompression cycle on the input
// string using the stream buffer classes:
//
//   1) Compress the input string, write the result to a temporary file using
//      std::ostream& operator<<(streambuf* sb), where 'sb' points to a
//      ZlibCompressorIStreambuf instance.
//
//   2) Close the temporary file to make sure all the data is flushed.
//
//   3) Create a ZlibDecompressorIStreambuf instance reading from the
//      temporary file, then decompress to an std::ostringstream using
//      std::ostream& operator<<(streambuf* sb), where 'sb' points to our
//      ZlibDecompressorIStreambuf instance.
//
//   4) Compare the result with the input string.
//
// Of course, it is possible to do this without any temporary file, even
// without any std::stringstream or such to hold the intermediate, compressed
// data. See the other tests for actual compressor + decompressor pipelines.
void pipeCompOrDecompIStreambufIntoOStream(const string& input)
{
  // Test “std::ostream << &ZlibCompressorIStreambuf”
  std::istringstream input_ss(input);
  simgear::ZlibCompressorIStreambuf compSBuf(input_ss, SGPath(), 9);
  simgear::Dir tmpDir = simgear::Dir::tempDir("FlightGear");
  tmpDir.setRemoveOnDestroy();
  SGPath path = tmpDir.path() / "testFile.dat";
  sg_ofstream oFile(path,
                    std::ios::binary | std::ios::trunc | std::ios::out);

  oFile << &compSBuf;
  // Make sure the compressed stream is flushed, otherwise when we read the
  // file back, decompression is likely to fail with Z_BUF_ERROR.
  oFile.close();
  SG_CHECK_EQUAL(compSBuf.sgetc(), EOF);

  // Read back and decompress the data we've written to 'path', testing
  // “std::ostream << &ZlibDecompressorIStreambuf”
  sg_ifstream iFile(path, std::ios::binary | std::ios::in);
  simgear::ZlibDecompressorIStreambuf decompSBuf(iFile, path);
  std::ostringstream roundTripResult_ss;
  // This is also possible, though maybe not as good for error detection:
  //
  //   decompSBuf >> roundTripResult_ss.rdbuf();
  roundTripResult_ss << &decompSBuf;
  SG_CHECK_EQUAL(decompSBuf.sgetc(), EOF);
  SG_CHECK_EQUAL(roundTripResult_ss.str(), input);
}

// Perform a compression-decompression cycle on bunch of strings, using the
// stream buffer classes directly (not the std::istream subclasses).
void test_pipeCompOrDecompIStreambufIntoOStream()
{
  cerr << "Compression-decompression cycles using the stream buffer classes "
    "directly\n";

  pipeCompOrDecompIStreambufIntoOStream(""); // empty string
  pipeCompOrDecompIStreambufIntoOStream("a");
  pipeCompOrDecompIStreambufIntoOStream("lorem ipsum");

  for (int i=0; i < 10; i++) {
    pipeCompOrDecompIStreambufIntoOStream(randomString(0, 20));
  }

  for (int i=0; i < 10; i++) {
    pipeCompOrDecompIStreambufIntoOStream(randomString(21, 1000));
  }

  assert(std::numeric_limits<string::size_type>::max() >= 65535);
  for (int i=0; i < 10; i++) {
    pipeCompOrDecompIStreambufIntoOStream(randomString(1000, 65535));
  }
}

void test_StreambufBasicOperations()
{
  cerr << "Testing basic operations on ZlibDecompressorIStreambuf\n";

  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  std::istringstream text_ss(text);

  static constexpr std::size_t compInBufSize = 6;
  static constexpr std::size_t compOutBufSize = 4;
  static constexpr std::size_t compPutbackSize = 0;
  simgear::ZlibCompressorIStreambuf compSBuf(
    text_ss, SGPath(), 8, simgear::ZLIB_COMPRESSION_FORMAT_ZLIB,
    simgear::ZLIB_FAVOR_SPEED_OVER_MEMORY, nullptr, compInBufSize, nullptr,
    compOutBufSize, compPutbackSize);
  std::stringstream compressedOutput_ss;
  compressedOutput_ss << &compSBuf;

  static constexpr std::size_t decompInBufSize = 5;
  static constexpr std::size_t decompOutBufSize = 4;
  static constexpr std::size_t decompPutbackSize = 2;
  simgear::ZlibDecompressorIStreambuf decompSBuf(
    compressedOutput_ss, SGPath(), simgear::ZLIB_COMPRESSION_FORMAT_ZLIB,
    nullptr, decompInBufSize, nullptr, decompOutBufSize, decompPutbackSize);

  int ch = decompSBuf.sgetc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '0');
  ch = decompSBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '0');
  ch = decompSBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '1');
  ch = decompSBuf.snextc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '3');
  ch = decompSBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '3');
  ch = decompSBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '4');
  ch = decompSBuf.sputbackc('4');
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '4');
  ch = decompSBuf.sputbackc('u'); // doesn't match what we read from the stream
  SG_VERIFY(ch == EOF);
  ch = decompSBuf.sputbackc('3'); // this one does
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == '3');

  static constexpr std::streamsize bufSize = 10;
  char buf[bufSize];
  // Most efficient way (with the underlying xsgetn()) to read several chars
  // at once.
  std::streamsize n = decompSBuf.sgetn(buf, bufSize);
  SG_CHECK_EQUAL(n, bufSize);
  SG_CHECK_EQUAL(string(buf, static_cast<std::size_t>(bufSize)),
                 "3456789abc");

  ch = decompSBuf.sungetc(); // same as sputbackc(), except no value to check
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'c');
  ch = decompSBuf.sungetc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'b');
  ch = decompSBuf.sbumpc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'b');
  ch = decompSBuf.sputbackc('b'); // this one does
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'b');

  n = decompSBuf.sgetn(buf, bufSize);
  SG_CHECK_EQUAL(n, bufSize);
  SG_CHECK_EQUAL(string(buf, static_cast<std::size_t>(bufSize)),
                 "bcdefghijk");

  ch = decompSBuf.sungetc();
  SG_VERIFY(ch != EOF && traits::to_char_type(ch) == 'k');

  static char buf2[64];
  n = decompSBuf.sgetn(buf2, sizeof(buf2));
  SG_CHECK_EQUAL(n, 36);
  SG_CHECK_EQUAL(string(buf2, 36), "klmnopqrstuvwxyz\nABCDEF\nGHIJK LMNOPQ");

  ch = decompSBuf.sbumpc();
  SG_CHECK_EQUAL(ch, EOF);
}

// Utility function: take a string as input to compress and return the
// compressed output as a string.
string compress(const string& dataToCompress,
                simgear::ZLibCompressionFormat compressionFormat,
                int compressionLevel,
                simgear::ZLibMemoryStrategy memStrategy,
                std::size_t putbackSize,
                SGPath path = SGPath())
{
  // Static storage is only okay for very small sizes like these, and because
  // we won't call the function from several threads at the same time. Plain
  // char arrays would be fine here, but I'll use std::array to show it can be
  // used here too.
  static std::array<char, 7> inBuf;
  static std::array<char, 7> outBuf;

  std::istringstream iss(dataToCompress);
  simgear::ZlibCompressorIStream compressor(
    iss, path, compressionLevel, compressionFormat, memStrategy,
    &inBuf.front(), inBuf.size(), &outBuf.front(), outBuf.size(),
    putbackSize);
  compressor.exceptions(std::ios_base::badbit); // throw if badbit is set
  std::ostringstream compressedData_ss;
  compressor >> compressedData_ss.rdbuf();

  return compressedData_ss.str();
}

void test_formattedInputFromDecompressor()
{
  cerr << "Testing ZlibDecompressorIStream >> std::string\n";

  static char inBuf[6];
  static char outBuf[15];
  string compressed = compress(
    lipsum, simgear::ZLIB_COMPRESSION_FORMAT_ZLIB, Z_BEST_COMPRESSION,
    simgear::ZLIB_FAVOR_MEMORY_OVER_SPEED, /* putback size */ 0);
  std::istringstream compressed_ss(compressed);

  simgear::ZlibDecompressorIStream decompressor(
    compressed_ss, SGPath(), simgear::ZLIB_COMPRESSION_FORMAT_ZLIB,
    inBuf, sizeof(inBuf), outBuf, sizeof(outBuf), /* putback size */ 1);
  decompressor.exceptions(std::ios_base::badbit); // throw if badbit is set

  int count = 0;
  string word;

  while (decompressor >> word) {
    count++;
  }

  SG_CHECK_EQUAL(count, 175);   // Number of words in 'lipsum'
  // If set, badbit would have caused an exception to be raised
  SG_VERIFY(!decompressor.bad());

  if (!decompressor.eof()) {
    assert(decompressor.fail());
    cerr << "Decompressor: stream extraction (operator>>) failed before "
      "reaching EOF.\n";
    SG_TEST_FAIL("Did not expect operator>> to fail with an std::string "
                 "argument.");
  }
}

void test_ZlibDecompressorIStream_readPutbackEtc()
{
  cerr << "Testing many operations on ZlibDecompressorIStream (read(), "
    "putback(), etc.\n";

  static char compInBuf[4];
  static char compOutBuf[6];
  static char decompInBuf[8];
  static char decompOutBuf[5];
  const string text = "0123456789abcdefghijklmnopqrstuvwxyz\nABCDEF\nGHIJK "
    "LMNOPQ";
  std::istringstream text_ss(text);

  simgear::ZlibCompressorIStream compressor(
    text_ss, SGPath(), Z_BEST_COMPRESSION,
    simgear::ZLIB_COMPRESSION_FORMAT_ZLIB,
    simgear::ZLIB_FAVOR_MEMORY_OVER_SPEED,
    compInBuf, sizeof(compInBuf), compOutBuf, sizeof(compOutBuf),
    /* putback size */ 0);
  compressor.exceptions(std::ios_base::badbit); // throw if badbit is set

  // Use the compressor (subclass of std::istream) as input to the decompressor
  simgear::ZlibDecompressorIStream decompressor(
    compressor, SGPath(), simgear::ZLIB_COMPRESSION_FORMAT_ZLIB,
    decompInBuf, sizeof(decompInBuf), decompOutBuf, sizeof(decompOutBuf),
    /* putback size */ 3);
  decompressor.exceptions(std::ios_base::badbit);

  {
    static std::array<char, 17> buf;
    decompressor.read(&buf.front(), 10);
    SG_VERIFY(decompressor.good() && decompressor.gcount() == 10);
    SG_CHECK_EQUAL(string(&buf.front(), 10), "0123456789");

    SG_VERIFY(decompressor.putback('9'));
    SG_VERIFY(decompressor.putback('8'));
    SG_VERIFY(decompressor.putback('7'));

    SG_VERIFY(decompressor.get(buf[10]));
    SG_VERIFY(decompressor.read(&buf[11], 6));
    SG_CHECK_EQUAL(string(&buf.front(), 17), "0123456789789abcd");
  }

  {
    bool gotException = false;
    try {
      // 'Z' is not the last character read from the stream
      decompressor.putback('Z');
    } catch (std::ios_base::failure) {
      gotException = true;
    }
    SG_VERIFY(gotException && decompressor.bad());
  }

  {
    int c;

    decompressor.clear();
    // Check we can resume normally now that we've cleared the error state flags
    c = decompressor.get();
    SG_VERIFY(c != traits::eof() && traits::to_char_type(c) == 'e');

    // unget() and get() in sequence
    SG_VERIFY(decompressor.unget());
    c = decompressor.get();
    SG_VERIFY(c != traits::eof() && traits::to_char_type(c)== 'e');

    // peek() looks at, but doesn't consume
    c = decompressor.peek();
    SG_VERIFY(c != traits::eof() && traits::to_char_type(c) == 'f');
    c = decompressor.get();
    SG_VERIFY(c != traits::eof() && traits::to_char_type(c)== 'f');
  }

  {
    static std::array<char, 21> buf; // 20 chars + terminating NUL char

    // std::istream::getline() will be stopped by \n after reading 20 chars
    SG_VERIFY(decompressor.getline(&buf.front(), 25));
    SG_VERIFY(decompressor.gcount() == 21 &&
              !strcmp(&buf.front(), "ghijklmnopqrstuvwxyz"));

    string str;
    // std::getline() will be stopped by \n after 7 chars have been extracted,
    // namely 6 chars plus the \n which is extracted and discarded.
    SG_VERIFY(std::getline(decompressor, str));
    SG_CHECK_EQUAL(str, "ABCDEF");

    SG_VERIFY(decompressor.ignore(2));
    SG_VERIFY(decompressor >> str);
    SG_CHECK_EQUAL(str, "IJK");

    static char buf2[5];
    // Read up to sizeof(buf) chars, without waiting if not that many are
    // immediately avaiable.
    int nbCharsRead = decompressor.readsome(buf2, sizeof(buf2));

    string rest(buf2, nbCharsRead);
    do {
      decompressor.read(buf2, sizeof(buf2));
      // The conversion to std::size_t is safe because decompressor.read()
      // returns a non-negative value which, in this case, can't exceed
      // sizeof(buf2).
      rest += string(buf2, streamsizeToSize_t(decompressor.gcount()));
    } while (decompressor);

    SG_CHECK_EQUAL(rest, " LMNOPQ");
  }

  // If set, badbit would have caused an exception to be raised, anyway
  SG_VERIFY(decompressor.eof() && !decompressor.bad());
}

// Utility function: parametrized round-trip test with a compressor +
// decompressor pipeline.
//
//
// Note: this is nice conceptually, allows to keep memory use constant even in
//       case an arbitrary amount of data is passed through, and exercises the
//       stream buffer classes well, however this technique is more than twice
//       as slow on my computer than using an intermediate buffer like this:
//
//         std::stringstream compressedData_ss;
//         compressor >> compressedData_ss.rdbuf();
//
//         simgear::ZlibDecompressorIStream decompressor(compressedData_ss,
//                                                       SGPath(), ...
void roundTripWithIStreams(
  simgear::ZLibCompressionFormat compressionFormat,
  int compressionLevel,
  simgear::ZLibMemoryStrategy memStrategy,
  std::size_t compInBufSize,
  std::size_t compOutBufSize,
  std::size_t decompInBufSize,
  std::size_t decompOutBufSize,
  std::size_t compPutbackSize,
  std::size_t decompPutbackSize,
  bool useAutoFormatForDecompression = false)
{
  const simgear::ZLibCompressionFormat decompFormat =
    (useAutoFormatForDecompression) ?
    simgear::ZLIB_COMPRESSION_FORMAT_AUTODETECT : compressionFormat;

  std::istringstream lipsum_ss(lipsum);
  // This tests the optional dynamic buffer allocation in ZlibAbstractIStreambuf
  simgear::ZlibCompressorIStream compressor(
    lipsum_ss, SGPath(), compressionLevel, compressionFormat, memStrategy,
    nullptr, compInBufSize, nullptr, compOutBufSize, compPutbackSize);
  compressor.exceptions(std::ios_base::badbit); // throw if badbit is set

  // Use the compressor as input to the decompressor (pipeline). The
  // decompressor uses read() with chunks that are as large as possible given
  // the available space in its input buffer. These read() calls are served by
  // ZlibCompressorIStreambuf::xsgetn(), which is efficient.
  simgear::ZlibDecompressorIStream decompressor(
    compressor, SGPath(), decompFormat,
    nullptr, decompInBufSize, nullptr, decompOutBufSize, decompPutbackSize);
  decompressor.exceptions(std::ios_base::badbit);
  std::ostringstream roundTripResult;
  // This, on the other hand, appears not to use xsgetn() (tested with the GNU
  // libstdc++ from g++ 6.3.0, 20170124). This causes useless copying of the
  // data to the decompressor output buffer, instead of writing it directly to
  // roundTripResult's internal buffer. This is simple and nice in appearance,
  // but if you are after performance, better use decompressor.read()
  // (typically in conjunction with gcount(), inside a loop).
  decompressor >> roundTripResult.rdbuf();
  SG_CHECK_EQUAL(roundTripResult.str(), lipsum);
}

// Round-trip conversion with simgear::ZlibCompressorIStream and
// simgear::ZlibDecompressorIStream, using various parameter combinations.
void test_RoundTripMultiWithIStreams()
{
  cerr <<
    "Compression-decompression cycles using the std::istream subclasses (many\n"
    "combinations of buffer sizes and compression parameters tested)\n";

  { // More variations on these later
    const std::size_t compPutbackSize = 1;
    const std::size_t decompPutbackSize = 1;

    for (auto format: {simgear::ZLIB_COMPRESSION_FORMAT_ZLIB,
          simgear::ZLIB_COMPRESSION_FORMAT_GZIP}) {
      for (int compressionLevel: {1, 4, 7, 9}) {
        for (auto memStrategy: {simgear::ZLIB_FAVOR_MEMORY_OVER_SPEED,
              simgear::ZLIB_FAVOR_SPEED_OVER_MEMORY}) {
          for (std::size_t compInBufSize: {3, 4}) {
            for (std::size_t compOutBufSize: {3, 5}) {
              for (std::size_t decompInBufSize: {3, 4}) {
                for (std::size_t decompOutBufSize: {3, 4,}) {
                  roundTripWithIStreams(
                    format, compressionLevel, memStrategy, compInBufSize,
                    compOutBufSize, decompInBufSize, decompOutBufSize,
                    compPutbackSize, decompPutbackSize);
                }
              }
            }
          }
        }
      }
    }
  }

  {
    const auto format = simgear::ZLIB_COMPRESSION_FORMAT_ZLIB;
    const int compressionLevel = Z_DEFAULT_COMPRESSION;
    const auto memStrategy = simgear::ZLIB_FAVOR_SPEED_OVER_MEMORY;

    for (std::size_t compInBufSize: {3, 4, 31, 256, 19475}) {
      for (std::size_t compOutBufSize: {3, 5, 9, 74, 4568}) {
        for (std::size_t decompInBufSize: {3, 4, 256, 24568}) {
          for (std::size_t decompOutBufSize: {3, 5, 42, 4568}) {
            for (std::size_t compPutbackSize: {0, 1, 2}) {
              for (std::size_t decompPutbackSize: {0, 1, 2}) {
                roundTripWithIStreams(
                  format, compressionLevel, memStrategy, compInBufSize,
                  compOutBufSize, decompInBufSize, decompOutBufSize,
                  compPutbackSize, decompPutbackSize);
              }
            }
          }
        }
      }
    }
  }

  {
    const std::size_t compInBufSize = 5;
    const std::size_t compOutBufSize = 107;
    const std::size_t decompInBufSize = 65536;
    const std::size_t decompOutBufSize = 84;
    int i = 0;

    for (std::size_t compPutbackSize: {25, 40, 105}) {
      for (std::size_t decompPutbackSize: {30, 60, 81}) {
        const simgear::ZLibCompressionFormat compFormat = (i++ % 2) ?
          simgear::ZLIB_COMPRESSION_FORMAT_ZLIB :
          simgear::ZLIB_COMPRESSION_FORMAT_GZIP;

        roundTripWithIStreams(
          compFormat, Z_BEST_COMPRESSION, simgear::ZLIB_FAVOR_MEMORY_OVER_SPEED,
          compInBufSize, compOutBufSize, decompInBufSize, decompOutBufSize,
          compPutbackSize, decompPutbackSize,
          /* automatic format detection for decompression */ true);
      }
    }
  }
}

int main(int argc, char** argv)
{
  test_pipeCompOrDecompIStreambufIntoOStream();
  test_StreambufBasicOperations();
  test_RoundTripMultiWithIStreams();
  test_formattedInputFromDecompressor();
  test_ZlibDecompressorIStream_readPutbackEtc();

  return EXIT_SUCCESS;
}
