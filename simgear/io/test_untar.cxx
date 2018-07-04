////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <iostream>

#include "untar.hxx"

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/sg_file.hxx>


using std::cout;
using std::cerr;
using std::endl;

using namespace simgear;

void testTarGz()
{
    SGPath p = SGPath(SRC_DIR);
    p.append("test.tar.gz");

    SGBinaryFile f(p);
    f.open(SG_IO_IN);

    uint8_t* buf = (uint8_t*) alloca(8192);
    size_t bufSize = f.read((char*) buf, 8192);

    SG_VERIFY(ArchiveExtractor::determineType(buf, bufSize) == ArchiveExtractor::TarData);

    f.close();
}

void testPlainTar()
{
    SGPath p = SGPath(SRC_DIR);
    p.append("test2.tar");

    SGBinaryFile f(p);
    f.open(SG_IO_IN);

	uint8_t* buf = (uint8_t*)alloca(8192);
	size_t bufSize = f.read((char*) buf, 8192);

	SG_VERIFY(ArchiveExtractor::determineType(buf, bufSize) == ArchiveExtractor::TarData);

    f.close();
}

void testExtractStreamed()
{
	SGPath p = SGPath(SRC_DIR);
	p.append("test.tar.gz");

	SGBinaryFile f(p);
	f.open(SG_IO_IN);

	SGPath extractDir = simgear::Dir::current().path() / "test_extract_streamed";
	simgear::Dir pd(extractDir);
	pd.removeChildren();

	ArchiveExtractor ex(extractDir);

	uint8_t* buf = (uint8_t*) alloca(128);
	while (!f.eof()) {
		size_t bufSize = f.read((char*) buf, 128);
		ex.extractBytes(buf, bufSize);
	}

	ex.flush();
	SG_VERIFY(ex.isAtEndOfArchive());
	SG_VERIFY(ex.hasError() == false);

	SG_VERIFY((extractDir / "testDir/hello.c").exists());
	SG_VERIFY((extractDir / "testDir/foo.txt").exists());
}

void testExtractLocalFile()
{

}

void testExtractZip()
{
	SGPath p = SGPath(SRC_DIR);
	p.append("zippy.zip");

	SGBinaryFile f(p);
	f.open(SG_IO_IN);

	SGPath extractDir = simgear::Dir::current().path() / "test_extract_zip";
	simgear::Dir pd(extractDir);
	pd.removeChildren();

	ArchiveExtractor ex(extractDir);

	uint8_t* buf = (uint8_t*)alloca(128);
	while (!f.eof()) {
		size_t bufSize = f.read((char*)buf, 128);
		ex.extractBytes(buf, bufSize);
	}

	ex.flush();
	SG_VERIFY(ex.isAtEndOfArchive());
	SG_VERIFY(ex.hasError() == false);

	SG_VERIFY((extractDir / "zippy/dirA/hello.c").exists());
	SG_VERIFY((extractDir / "zippy/bar.xml").exists());
	SG_VERIFY((extractDir / "zippy/long-named.json").exists());
}

void testPAXAttributes()
{

}

int main(int ac, char ** av)
{
    testTarGz();
    testPlainTar();

	testExtractStreamed();
	testExtractZip();

	std::cout << "all tests passed" << std::endl;
    return 0;
}
