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

    SG_VERIFY(ArchiveExtractor::determineType(buf, bufSize) == ArchiveExtractor::GZData);

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

void testFilterTar()
{
    SGPath p = SGPath(SRC_DIR);
    p.append("badTar.tgz");
    
    SGBinaryFile f(p);
    f.open(SG_IO_IN);
    
    SGPath extractDir = simgear::Dir::current().path() / "test_filter_tar";
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
    
    SG_VERIFY((extractDir / "tarWithBadContent/regular-file.txt").exists());
    SG_VERIFY(!(extractDir / "tarWithBadContent/symbolic-linked.png").exists());
    SG_VERIFY((extractDir / "tarWithBadContent/screenshot.png").exists());
    SG_VERIFY((extractDir / "tarWithBadContent/dirOne/subDirA").exists());
    SG_VERIFY(!(extractDir / "tarWithBadContent/dirOne/subDirA/linked.txt").exists());


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
    SGPath p = SGPath(SRC_DIR);
    p.append("pax-extended.tar");
    
    SGBinaryFile f(p);
    f.open(SG_IO_IN);
    
    SGPath extractDir = simgear::Dir::current().path() / "test_pax_extended";
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

}

void testExtractXZ()
{
    SGPath p = SGPath(SRC_DIR);
    p.append("test.tar.xz");

    SGBinaryFile f(p);
    f.open(SG_IO_IN);

    SGPath extractDir = simgear::Dir::current().path() / "test_extract_xz";
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

    SG_VERIFY((extractDir / "testDir/hello.c").exists());
    SG_VERIFY((extractDir / "testDir/foo.txt").exists());
}

int main(int ac, char ** av)
{
    testTarGz();
    testPlainTar();
    testFilterTar();
	testExtractStreamed();
	testExtractZip();
    testExtractXZ();

    // disabled to avoiding checking in large PAX archive
    // testPAXAttributes();
    
	std::cout << "all tests passed" << std::endl;
    return 0;
}
