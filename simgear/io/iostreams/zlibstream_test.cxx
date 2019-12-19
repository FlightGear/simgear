// -*- coding: utf-8 -*-
//
// zlibstream_test.cxx --- Automated tests for zlibstream.cxx / zlibstream.hxx
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
#include <array>
#include <random>
#include <memory>               // std::unique_ptr
#include <utility>              // std::move()
#include <limits>               // std::numeric_limits
#include <type_traits>          // std::make_unsigned()
#include <functional>           // std::bind()
#include <cassert>
#include <cstdlib>              // EXIT_SUCCESS
#include <cstddef>              // std::size_t
#include <cstring>              // strcmp()

#include <zlib.h>               // Z_BEST_COMPRESSION

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
// I suggest reading test_IStreamConstructorWithSinkSemantics() below to see
// how to use the classes efficiently.

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
    text_ss, SGPath(), 8, simgear::ZLibCompressionFormat::ZLIB,
    simgear::ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
    nullptr, compInBufSize, nullptr, compOutBufSize, compPutbackSize);
  std::stringstream compressedOutput_ss;
  compressedOutput_ss << &compSBuf;

  static constexpr std::size_t decompInBufSize = 5;
  static constexpr std::size_t decompOutBufSize = 4;
  static constexpr std::size_t decompPutbackSize = 2;
  simgear::ZlibDecompressorIStreambuf decompSBuf(
    compressedOutput_ss, SGPath(), simgear::ZLibCompressionFormat::ZLIB,
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

// Test simgear::ZlibDecompressorIStreambuf::[x]sgetn(), asking the largest
// possible amount of chars every time it is called (i.e., the largest value
// that can be represented by std::streamsize).
void test_ZlibDecompressorIStreambuf_readLargestPossibleAmount()
{
  // Nothing special with these values
  constexpr std::size_t maxDataSize = 8192;
  std::istringstream input_ss(randomString(4096, maxDataSize));

  simgear::ZlibCompressorIStream compIStream(
    input_ss,                   // input stream
    SGPath(),                   // this stream is not associated to a file
    9,                          // compression level
    simgear::ZLibCompressionFormat::ZLIB,
    simgear::ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
    nullptr,                    // dynamically allocate the input buffer
    230,                        // input buffer size
    nullptr,                    // dynamically allocate the output buffer
    120,                        // output buffer size
    1                           // putback size
    );

  // Decompressor stream buffer (std::streambuf subclass) that gets input data
  // from our compressor 'compIStream' (std::istream subclass)
  simgear::ZlibDecompressorIStreambuf decompSBuf(
    compIStream, SGPath(), simgear::ZLibCompressionFormat::ZLIB,
    nullptr, 150, nullptr, 175, 2);

  std::unique_ptr<char[]> buf(new char[maxDataSize]);
  std::ostringstream roundTripResult_ss;
  std::streamsize totalCharsToRead = input_ss.str().size();

  while (totalCharsToRead > 0) {
    // Ask sgetn() the largest possible amount of chars. Of course, we know we
    // can't get more than maxDataSize, but this does exercise the code in
    // interesting ways due to the various types involved (zlib's uInt,
    // std::size_t and std::streamsize, which have various sizes depending on
    // the platform).
    std::streamsize nbCharsRead = decompSBuf.sgetn(
      &buf[0], std::numeric_limits<std::streamsize>::max());
    if (nbCharsRead == 0) {
      break;                    // no more data
    }

    // The conversion to std::size_t is safe because decompSBuf.sgetn()
    // returned a non-negative value which, in this case, can't exceed
    // maxDataSize.
    roundTripResult_ss << string(&buf[0], streamsizeToSize_t((nbCharsRead)));
  }

  SG_CHECK_EQUAL(decompSBuf.sgetc(), EOF);
  SG_CHECK_EQUAL(roundTripResult_ss.str(), input_ss.str());
}

void test_formattedInputFromDecompressor()
{
  cerr << "Testing ZlibDecompressorIStream >> std::string\n";

  static char inBuf[6];
  static char outBuf[15];
  string compressed = compress(
    lipsum, simgear::ZLibCompressionFormat::ZLIB, Z_BEST_COMPRESSION,
    simgear::ZLibMemoryStrategy::FAVOR_MEMORY_OVER_SPEED, /* putback size */ 0);
  std::istringstream compressed_ss(compressed);

  simgear::ZlibDecompressorIStream decompressor(
    compressed_ss, SGPath(), simgear::ZLibCompressionFormat::ZLIB,
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
    simgear::ZLibCompressionFormat::ZLIB,
    simgear::ZLibMemoryStrategy::FAVOR_MEMORY_OVER_SPEED,
    compInBuf, sizeof(compInBuf), compOutBuf, sizeof(compOutBuf),
    /* putback size */ 0);
  compressor.exceptions(std::ios_base::badbit); // throw if badbit is set

  // Use the compressor (subclass of std::istream) as input to the decompressor
  simgear::ZlibDecompressorIStream decompressor(
    compressor, SGPath(), simgear::ZLibCompressionFormat::ZLIB,
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
    } catch (const std::ios_base::failure&) {
      gotException = true;
    } catch (const std::exception& e) {
      // gcc fails to catch std::ios_base::failure due to an inconsistent C++11
      // ABI between headers and libraries. See bug#66145 for more details.
      // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66145
      if (!strcmp(e.what(), "basic_ios::clear"))
        gotException = true;
      else
        throw e;
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
    simgear::ZLibCompressionFormat::AUTODETECT : compressionFormat;

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

    for (auto format: {simgear::ZLibCompressionFormat::ZLIB,
                       simgear::ZLibCompressionFormat::GZIP}) {
      for (int compressionLevel: {1, 4, 7, 9}) {
        for (auto memStrategy: {
            simgear::ZLibMemoryStrategy::FAVOR_MEMORY_OVER_SPEED,
            simgear::ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY}) {
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
    const auto format = simgear::ZLibCompressionFormat::ZLIB;
    const int compressionLevel = Z_DEFAULT_COMPRESSION;
    const auto memStrategy =
      simgear::ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY;

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
          simgear::ZLibCompressionFormat::ZLIB :
          simgear::ZLibCompressionFormat::GZIP;

        roundTripWithIStreams(
          compFormat, Z_BEST_COMPRESSION,
          simgear::ZLibMemoryStrategy::FAVOR_MEMORY_OVER_SPEED,
          compInBufSize, compOutBufSize, decompInBufSize, decompOutBufSize,
          compPutbackSize, decompPutbackSize,
          /* automatic format detection for decompression */ true);
      }
    }
  }
}

// Utility function showing how to return a (unique_ptr to a)
// ZlibCompressorIStream instance, that keeps a reference to its data source
// as long as the ZlibCompressorIStream instance is alive. Thus, calling code
// doesn't have to worry about the lifetime of said data source (here, an
// std::istringstream instance).
std::unique_ptr<simgear::ZlibCompressorIStream>
IStreamConstructorWithSinkSemantics_compressorFactory(const string& str)
{
  std::unique_ptr<std::istringstream> iss(new std::istringstream(str));

  // The returned compressor object retains a “reference” (of unique_ptr type)
  // to the std::istringstream object pointed to by 'iss' as long as it is
  // alive. When the returned compressor object (wrapped in a unique_ptr) is
  // destroyed, this std::istringstream object will be automatically
  // destroyed too.
  //
  // Note: it's an implementation detail, but this test also indirectly
  //       exercises the ZlibCompressorIStreambuf constructor taking an
  //       argument of type std::unique_ptr<std::istream>.
  return std::unique_ptr<simgear::ZlibCompressorIStream>(
    new simgear::ZlibCompressorIStream(std::move(iss)));
}

void test_IStreamConstructorWithSinkSemantics()
{
  cerr << "Testing the unique_ptr-based ZlibCompressorIStream constructor\n";

  string someString = randomString(4096, 8192); // arbitrary values
  // This shows how to get a new compressor or decompressor object from a
  // factory function. Of course, we could create the object directly on the
  // stack without using a separate function!
  std::unique_ptr<simgear::ZlibCompressorIStream> compressor =
    IStreamConstructorWithSinkSemantics_compressorFactory(someString);
  compressor->exceptions(std::ios_base::badbit); // throw if badbit is set

  // Use the compressor as input to the decompressor (pipeline). The
  // decompressor uses read() with chunks that are as large as possible given
  // the available space in its input buffer. These read() calls are served by
  // ZlibCompressorIStreambuf::xsgetn(), which is efficient. We won't need the
  // compressor afterwards, so let's just std::move() its unique_ptr.
  simgear::ZlibDecompressorIStream decompressor(std::move(compressor));
  decompressor.exceptions(std::ios_base::badbit);

  std::ostringstream roundTripResult;
  // Of course, you may want to adjust bufSize depending on the application.
  static constexpr std::size_t bufSize = 1024;
  std::unique_ptr<char[]> buf(new char[bufSize]);

  // Relatively efficient way of reading from the decompressor (modulo
  // possible adjustments to 'bufSize', of course). The decompressed data is
  // first written to 'buf', then copied to 'roundTripResult'. There is no
  // other useless copy via, for instance, an intermediate std::string object,
  // as would be the case if we used std::string(buf.get(), bufSize).
  //
  // Of course, ideally 'roundTripResult' would directly pull from
  // 'decompressor' without going through 'buf', but I don't think this is
  // possible with std::stringstream and friends. Such an optimized data flow
  // is however straightforward to implement if you replace 'roundTripResult'
  // with a custom data sink that calls decompressor.read().
  do {
    decompressor.read(buf.get(), bufSize);
    if (decompressor.gcount() > 0) { // at least one char could be read
      roundTripResult.write(buf.get(), decompressor.gcount());
    }
  } while (decompressor && roundTripResult);

  // 1) If set, badbit would have caused an exception to be raised (see above).
  // 2) failbit doesn't necessarily indicate an error here: it is set as soon
  //    as the read() call can't provide the requested number of characters.
  SG_VERIFY(decompressor.eof() && !decompressor.bad());
  // Because of std::ostringstream::write(), 'roundTripResult' might have its
  // failbit or badbit set, either of which would indicate a real problem.
  SG_VERIFY(roundTripResult);
  SG_CHECK_EQUAL(roundTripResult.str(), someString);
}

int main(int argc, char** argv)
{
  test_pipeCompOrDecompIStreambufIntoOStream();
  test_StreambufBasicOperations();
  test_ZlibDecompressorIStreambuf_readLargestPossibleAmount();
  test_RoundTripMultiWithIStreams();
  test_formattedInputFromDecompressor();
  test_ZlibDecompressorIStream_readPutbackEtc();
  test_IStreamConstructorWithSinkSemantics();

  return EXIT_SUCCESS;
}
