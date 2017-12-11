////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <iostream>

#include "untar.hxx"

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_path.hxx>
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

    SG_VERIFY(TarExtractor::isTarData(buf, bufSize));

    f.close();
}

void testPlainTar()
{
    SGPath p = SGPath(SRC_DIR);
    p.append("test2.tar");

    SGBinaryFile f(p);
    f.open(SG_IO_IN);

    uint8_t* buf = (uint8_t*) alloca(8192);
    size_t bufSize = f.read((char*) buf, 8192);

    SG_VERIFY(TarExtractor::isTarData(buf, bufSize));

    f.close();
}

int main(int ac, char ** av)
{
    testTarGz();
    testPlainTar();

    return 0;
}
