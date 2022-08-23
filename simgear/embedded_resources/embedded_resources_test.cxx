// -*- coding: utf-8 -*-
//
// embedded_resources_test.cxx --- Automated tests for the embedded resources
//                                 system in SimGear
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

#include <string>
#include <memory>
#include <ios>                  // std::streamsize
#include <iostream>             // std::cout (used for progress info)
#include <limits>               // std::numeric_limits
#include <type_traits>          // std::make_unsigned()
#include <sstream>
#include <cstdlib>              // EXIT_SUCCESS
#include <cstddef>              // std::size_t

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/io/iostreams/CharArrayStream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/io/iostreams/zlibstream.hxx>
#include "EmbeddedResource.hxx"
#include "EmbeddedResourceManager.hxx"
#include "EmbeddedResourceProxy.hxx"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::unique_ptr;
using std::shared_ptr;
using simgear::AbstractEmbeddedResource;
using simgear::RawEmbeddedResource;
using simgear::ZlibEmbeddedResource;
using simgear::EmbeddedResourceManager;

typedef typename std::make_unsigned<std::streamsize>::type uStreamSize;

// Safely convert a non-negative std::streamsize into an std::size_t. If
// impossible, bail out.
std::size_t streamsizeToSize_t(std::streamsize n)
{
  SG_CHECK_GE(n, 0);
  SG_CHECK_LE(static_cast<uStreamSize>(n),
              std::numeric_limits<std::size_t>::max());

  return static_cast<std::size_t>(n);
}

// This array is null-terminated, but we'll declare the resource size as
// sizeof(res1Array) - 1 so that the null char is *not* part of it. This
// way allows one to treat text and binary resources exactly the same way,
// with the conversion to std::string via a simple
// std::string(res1Array, resourceSize) not producing a bizarre std::string
// instance whose last character would be '\0' (followed in memory by the same
// '\0' used as C-style string terminator this time!).
static const char res1Array[] = "This is a simple embedded resource test.";
static const char res1frArray[] = "Ceci est un petit test de ressource "
                                  "embarquée.";
static const char res1fr_FRArray[] = "Ceci est un petit test de ressource "
                                     "embarquée (variante fr_FR).";
static const string lipsum = "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque congue ornare\n\
congue. Mauris mollis est et porttitor condimentum. Vivamus laoreet blandit\n\
odio eget consectetur. Etiam quis magna eu enim luctus pretium. In et\n\
tristique nunc, non efficitur metus. Nullam efficitur tristique velit.\n\
Praesent et luctus nunc. Mauris eros eros, rutrum at molestie quis, egestas et\n\
lorem. Ut nulla turpis, eleifend sed mauris ac, faucibus molestie nulla.\n\
Quisque viverra vel turpis nec efficitur. Proin non rutrum velit. Nam sodales\n\
metus felis, eu pharetra velit posuere ut.";
// Should be enough to store the compressed lipsum (320 bytes are required
// with zlib 1.2.8, keeping some room to account for possible future format
// changes in the zlib output...). In any case, there is no risk of buffer
// overflow, because simgear::CharArrayOStream prevents this by design.
static char res2Array[350];
static const char res2frArray[] = "Un lorem ipsum un peu plus court...";


// Read data from a string and write it in compressed form to the specified
// buffer.
std::size_t writeCompressedDataToBuffer(const string& inputString,
                                        char *outBuf,
                                        std::size_t outBufSize)
{
  simgear::CharArrayOStream res2Writer(outBuf, outBufSize);
  std::istringstream iss(inputString);
  simgear::ZlibCompressorIStream compressor(
    iss,
    SGPath(), /* no associated file */
    9         /* highest compression level */);
  static constexpr std::size_t bufSize = 1024;
  unique_ptr<char[]> buf(new char[bufSize]);
  std::size_t res2Size = 0;

  do {
    compressor.read(buf.get(), bufSize);
    std::streamsize nBytes = compressor.gcount();
    if (nBytes > 0) {           // at least one char could be read
      res2Writer.write(buf.get(), nBytes);
      res2Size += nBytes;
    }
  } while (compressor && res2Writer);

  SG_VERIFY(compressor.eof());  // all the compressed data has been read
  // This would fail (among other causes) if the output buffer were too small
  // to hold all of the compressed data.
  SG_VERIFY(res2Writer);

  return res2Size;
}

void initResources()
{
  cout << "Creating the EmbeddedResourceManager instance and adding a few "
    "resources to it" << endl;
  const auto& resMgr = EmbeddedResourceManager::createInstance();

  // The resource will *not* consider the null terminator to be in.
  unique_ptr<const RawEmbeddedResource> res1(
    new RawEmbeddedResource(res1Array, sizeof(res1Array) - 1));
  resMgr->addResource("/path/to/resource1", std::move(res1));

  unique_ptr<const RawEmbeddedResource> res1fr(
    new RawEmbeddedResource(res1frArray, sizeof(res1frArray) - 1));
  resMgr->addResource("/path/to/resource1", std::move(res1fr), "fr");

  unique_ptr<const RawEmbeddedResource> res1fr_FR(
    new RawEmbeddedResource(res1fr_FRArray, sizeof(res1fr_FRArray) - 1));
  resMgr->addResource("/path/to/resource1", std::move(res1fr_FR), "fr_FR");

  // Write the contents of 'lipsum' in compressed form to the 'res2Array'
  // static buffer.
  std::size_t res2Size = writeCompressedDataToBuffer(lipsum, res2Array,
                                                     sizeof(res2Array));
  // Now we have a compressed resource to work with, plus the corresponding
  // uncompressed output -> perfect for tests!
  unique_ptr<const ZlibEmbeddedResource> res2(
    new ZlibEmbeddedResource(res2Array, res2Size, lipsum.size()));
  resMgr->addResource("/path/to/resource2", std::move(res2));

  unique_ptr<const RawEmbeddedResource> res2fr(
    new RawEmbeddedResource(res2frArray, sizeof(res2frArray) - 1));
  resMgr->addResource("/path/to/resource2", std::move(res2fr), "fr");

  // Explicitly select the default locale (typically, English). This is for
  // clarity, but isn't required.
  resMgr->selectLocale("");
}

// Auxiliary function for test_RawEmbeddedResource()
void auxTest_RawEmbeddedResource_streambuf()
{
  cout << "Testing EmbeddedResourceManager::getStreambuf()" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();

  unique_ptr<std::streambuf> sbuf(resMgr->getStreambuf("/path/to/resource1"));
  // Just to show an efficient algorithm. For real applications, use larger
  // buffer sizes!
  static constexpr std::size_t bufSize = 4;
  unique_ptr<char[]> buf(new char[bufSize]); // intermediate buffer
  std::streamsize nbCharsRead;
  string result;

  do {
    nbCharsRead = sbuf->sgetn(buf.get(), bufSize);
    // The conversion to std::size_t is safe because sbuf->sgetn() returned a
    // non-negative value which, in this case, can't exceed bufSize.
    result.append(buf.get(), streamsizeToSize_t((nbCharsRead)));
  } while (nbCharsRead == bufSize);

  SG_CHECK_EQUAL(result, "This is a simple embedded resource test.");
}

// Auxiliary function for test_RawEmbeddedResource()
void auxTest_RawEmbeddedResource_istream()
{
  cout << "Testing EmbeddedResourceManager::getIStream()" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();

  unique_ptr<std::istream> iStream(resMgr->getIStream("/path/to/resource1"));
  // This is convenient, but be aware that still in 2017, some buggy C++
  // compilers don't allow the exception to be caught: cf.
  // <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66145>.
  iStream->exceptions(std::ios_base::badbit);
  // Just to show an efficient algorithm. For real applications, use larger
  // buffer sizes!
  static constexpr std::size_t bufSize = 4;
  unique_ptr<char[]> buf(new char[bufSize]); // intermediate buffer
  string result;

  do {
    iStream->read(buf.get(), bufSize);
    result.append(buf.get(), iStream->gcount());
  } while (*iStream);           // iStream *points* to an std::istream

  // 1) If set, badbit would have caused an exception to be raised (see above).
  // 2) failbit doesn't necessarily indicate an error here: it is set as soon
  //    as the read() call can't provide the requested number of characters.
  SG_VERIFY(iStream->eof() && !iStream->bad());
  SG_CHECK_EQUAL(result, "This is a simple embedded resource test.");
}

void test_RawEmbeddedResource()
{
  cout << "Testing resource fetching methods of EmbeddedResourceManager with "
    "a RawEmbeddedResource" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();

  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1"),
                 string("This is a simple embedded resource test."));

  // Get a shared_ptr to a const AbstractEmbeddedResource
  const auto res1abs = resMgr->getResource("/path/to/resource1");
  // Okay because we know this resource is not a compressed one
  const auto res1 =
    std::dynamic_pointer_cast<const RawEmbeddedResource>(res1abs);
  SG_VERIFY(res1);

  // Print a representation of the resource metadata
  std::cout << "\n/path/to/resource1 -> " << *res1 << "\n\n";

  // The following methods would work the same with res1abs
  SG_CHECK_EQUAL_NOSTREAM(res1->compressionType(),
                          AbstractEmbeddedResource::CompressionType::NONE);
  SG_CHECK_EQUAL(res1->compressionDescr(), "none");

  SG_CHECK_EQUAL(res1->rawPtr(), res1Array);
  SG_CHECK_EQUAL(res1->rawSize(), sizeof(res1Array) - 1); // see above
  SG_CHECK_EQUAL(res1->str(),
                 string("This is a simple embedded resource test."));

  auxTest_RawEmbeddedResource_streambuf();
  auxTest_RawEmbeddedResource_istream();

  // Just reload and recheck the resource, because we can :)
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1"),
                 string("This is a simple embedded resource test."));
}

void test_ZlibEmbeddedResource()
{
  cout << "Testing resource fetching methods of EmbeddedResourceManager with "
    "a ZlibEmbeddedResource" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();

  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource2"),
                 lipsum);

  // Get a shared_ptr to a const AbstractEmbeddedResource
  const auto res2abs = resMgr->getResource("/path/to/resource2");
  // Okay because we know this resource is a Zlib-compressed one
  const auto res2 =
    std::dynamic_pointer_cast<const ZlibEmbeddedResource>(res2abs);
  SG_VERIFY(res2);

  SG_CHECK_EQUAL(res2->uncompressedSize(), lipsum.size());

  // Print a representation of the resource metadata
  std::cout << "\n/path/to/resource2 -> " << *res2 << "\n\n";
  cout << "Resource 2 compression ratio: " <<
    static_cast<float>(res2->uncompressedSize()) /
    static_cast<float>(res2->rawSize()) << "\n";

  // Just reload and recheck the resource
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource2"), lipsum);
}

void test_getMissingResources()
{
  cout << "Testing the behavior of EmbeddedResourceManager when trying to "
    "fetch inexistent resources" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();
  SG_VERIFY(!resMgr->getResourceOrNullPtr("/inexistant/resource"));

  bool gotException = false;
  try {
    resMgr->getResource("/inexistant/resource");
  } catch (const sg_exception&) {
    gotException = true;
  }
  SG_VERIFY(gotException);

  gotException = false;
  try {
    resMgr->getString("/other/inexistant/resource");
  } catch (const sg_exception&) {
    gotException = true;
  }
  SG_VERIFY(gotException);
}

void test_addAlreadyExistingResource()
{
  cout << "Testing the behavior of EmbeddedResourceManager when trying to "
    "add an already existing resource" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();

  for (const string locale: {"", "fr", "fr_FR"}) {
    // For these tests, we don't care about the resource contents -> no need
    // to substract 1 from the result of sizeof() as we did above.
    unique_ptr<const RawEmbeddedResource> someRes(
      new RawEmbeddedResource(res1fr_FRArray, sizeof(res1fr_FRArray)));

    bool gotException = false;
    try {
      resMgr->addResource("/path/to/resource1", std::move(someRes), locale);
    } catch (const sg_error&) {
      gotException = true;
    }
    SG_VERIFY(gotException);
  }
}

void test_localeDependencyOfResourceFetching()
{
  cout << "Testing the locale-dependency of resource fetching from "
    "EmbeddedResourceManager" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();
  resMgr->selectLocale("");     // select the default locale

  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1"),
                 "This is a simple embedded resource test.");

  // Switch to the 'fr_FR' locale (French from France)
  resMgr->selectLocale("fr_FR");
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1"),
                 "Ceci est un petit test de ressource embarquée (variante "
                 "fr_FR).");

  // This one is for the 'fr' “locale”, obtained as fallback since there is no
  // resource mapped to /path/to/resource2 for the 'fr_FR' “locale”.
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource2"),
                 "Un lorem ipsum un peu plus court...");

  // Explicitly ask for the resource in the default locale
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1", ""),
                 "This is a simple embedded resource test.");

  // Switch to the 'fr' locale (French)
  resMgr->selectLocale("fr");
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1"),
                 "Ceci est un petit test de ressource embarquée.");

  // Explicitly ask for the resource in the 'fr_FR' locale
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1", "fr_FR"),
                 "Ceci est un petit test de ressource embarquée "
                 "(variante fr_FR).");

  // Switch to the default locale
  resMgr->selectLocale("");
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1"),
                 "This is a simple embedded resource test.");

  // Explicitly ask for the resource in the 'fr' locale
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1", "fr"),
                 "Ceci est un petit test de ressource embarquée.");

  // Explicitly ask for the resource in the 'fr_FR' locale
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1", "fr_FR"),
                 "Ceci est un petit test de ressource embarquée "
                 "(variante fr_FR).");

  // Explicitly ask for the resource in the default locale
  SG_CHECK_EQUAL(resMgr->getString("/path/to/resource1", ""),
                 "This is a simple embedded resource test.");
}

void test_getLocaleAndSelectLocale()
{
  cout << "Testing the getLocale() and selectLocale() methods of "
    "EmbeddedResourceManager" << endl;
  const auto& resMgr = EmbeddedResourceManager::instance();

  for (const string locale: {"", "fr", "fr_FR", "de_DE"}) {
    // The important effects of setLocale() are tested in
    // test_localeDependencyOfResourceFetching()
    resMgr->selectLocale(locale);
    SG_CHECK_EQUAL(resMgr->getLocale(), locale);
  }
}

// Auxiliary function for test_EmbeddedResourceProxy()
void auxTest_EmbeddedResourceProxy_getIStream(unique_ptr<std::istream> iStream,
                                              const string& contents)
{
  cout << "Testing EmbeddedResourceProxy::getIStream()" << endl;

  iStream->exceptions(std::ios_base::badbit);
  static constexpr std::size_t bufSize = 65536;
  unique_ptr<char[]> buf(new char[bufSize]); // intermediate buffer
  string result;

  do {
    iStream->read(buf.get(), bufSize);
    result.append(buf.get(), iStream->gcount());
  } while (*iStream);           // iStream *points* to an std::istream

  // 1) If set, badbit would have caused an exception to be raised (see above).
  // 2) failbit doesn't necessarily indicate an error here: it is set as soon
  //    as the read() call can't provide the requested number of characters.
  SG_VERIFY(iStream->eof() && !iStream->bad());
  SG_CHECK_EQUAL(result, contents);
}

void test_EmbeddedResourceProxy()
{
  cout << "Testing the EmbeddedResourceProxy class" << endl;

  // Initialize stuff we need and create two files containing the contents of
  // the default-locale version of two embedded resources: those with virtual
  // paths '/path/to/resource1' and '/path/to/resource2'.
  const auto& resMgr = EmbeddedResourceManager::instance();
  simgear::Dir tmpDir = simgear::Dir::tempDir("FlightGear");
  tmpDir.setRemoveOnDestroy();

  const SGPath path1 = tmpDir.path() / "resource1";
  const SGPath path2 = tmpDir.path() / "resource2";

  sg_ofstream out1(path1);
  sg_ofstream out2(path2);
  const string s1 = resMgr->getString("/path/to/resource1", "");
  // To make sure in these tests that we can tell whether something came from
  // a real file or from an embedded resource.
  const string rs1 = s1 + " from real file";
  const string rlipsum = lipsum + " from real file";

  out1 << rs1;
  out1.close();
  if (!out1) {
    throw sg_io_exception("Error writing to file", sg_location(path1));
  }

  out2 << rlipsum;
  out2.close();
  if (!out2) {
    throw sg_io_exception("Error writing to file", sg_location(path2));
  }

  // 'proxy' defaults to using embedded resources
  const simgear::EmbeddedResourceProxy proxy(tmpDir.path(), "/path/to",
                                             /* useEmbeddedResourcesByDefault */
                                             true);
  simgear::EmbeddedResourceProxy rproxy(tmpDir.path(), "/path/to");
  // 'rproxy' defaults to using real files
  rproxy.setUseEmbeddedResources(false); // could be done from the ctor too

  // Test EmbeddedResourceProxy::getString()
  SG_CHECK_EQUAL(proxy.getStringDecideOnPrefix("/resource1"), rs1);
  SG_CHECK_EQUAL(proxy.getStringDecideOnPrefix(":/resource1"), s1);
  SG_CHECK_EQUAL(proxy.getString("/resource1", false), rs1);
  SG_CHECK_EQUAL(proxy.getString("/resource1", true), s1);
  SG_CHECK_EQUAL(proxy.getString("/resource1"), s1);
  SG_CHECK_EQUAL(rproxy.getString("/resource1"), rs1);

  SG_CHECK_EQUAL(proxy.getStringDecideOnPrefix("/resource2"), rlipsum);
  SG_CHECK_EQUAL(proxy.getStringDecideOnPrefix(":/resource2"), lipsum);
  SG_CHECK_EQUAL(proxy.getString("/resource2", false), rlipsum);
  SG_CHECK_EQUAL(proxy.getString("/resource2", true), lipsum);
  SG_CHECK_EQUAL(proxy.getString("/resource2"), lipsum);
  SG_CHECK_EQUAL(rproxy.getString("/resource2"), rlipsum);

  // Test EmbeddedResourceProxy::getIStream()
  auxTest_EmbeddedResourceProxy_getIStream(
    proxy.getIStreamDecideOnPrefix("/resource1"),
    rs1);
  auxTest_EmbeddedResourceProxy_getIStream(
    proxy.getIStreamDecideOnPrefix(":/resource1"),
    s1);
  auxTest_EmbeddedResourceProxy_getIStream(proxy.getIStream("/resource1"), s1);
  auxTest_EmbeddedResourceProxy_getIStream(rproxy.getIStream("/resource1"), rs1);

  auxTest_EmbeddedResourceProxy_getIStream(proxy.getIStream("/resource2", false),
                                           rlipsum);
  auxTest_EmbeddedResourceProxy_getIStream(proxy.getIStream("/resource2", true),
                                           lipsum);
  auxTest_EmbeddedResourceProxy_getIStream(proxy.getIStream("/resource2"),
                                           lipsum);
  auxTest_EmbeddedResourceProxy_getIStream(rproxy.getIStream("/resource2"),
                                           rlipsum);
}

int main(int argc, char **argv)
{
  // Initialize the EmbeddedResourceManager instance, add a few resources
  // to it and call its selectLocale() method.
  initResources();

  test_RawEmbeddedResource();
  test_ZlibEmbeddedResource();
  test_getMissingResources();
  test_addAlreadyExistingResource();
  test_localeDependencyOfResourceFetching();
  test_getLocaleAndSelectLocale();
  test_EmbeddedResourceProxy();

  return EXIT_SUCCESS;
}
