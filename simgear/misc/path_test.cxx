
#include <simgear/compiler.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

using std::cout;
using std::cerr;
using std::endl;

#include <simgear/misc/test_macros.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sgstream.hxx>

#if defined(SG_WINDOWS)
#else
    #include <sys/stat.h>
#endif

void test_dir()
{
    simgear::Dir temp = simgear::Dir::tempDir("foo");
    cout << "created:" << temp.path() << endl;
  
    SG_VERIFY(temp.exists());
    SG_VERIFY(temp.path().isDir());
    SG_VERIFY(!temp.path().isFile());
    
    SGPath fileInDir = temp.file("foobaz");
    SG_VERIFY(!fileInDir.exists());
    
    if (!temp.remove(true)) {
        cout << "remove failed!" << endl;
    }
    
    cout << temp.path().modTime() << endl;

    std::cout << "Standard Locations:"
              << "\n - Home:      " << SGPath::standardLocation(SGPath::HOME)
              << "\n - Desktop:   " << SGPath::standardLocation(SGPath::DESKTOP)
              << "\n - Downloads: " << SGPath::standardLocation(SGPath::DOWNLOADS)
              << "\n - Documents: " << SGPath::standardLocation(SGPath::DOCUMENTS)
              << "\n - Pictures:  " << SGPath::standardLocation(SGPath::PICTURES)
              << std::endl;

    SG_VERIFY( !SGPath::standardLocation(SGPath::HOME     ).isNull() );
    SG_VERIFY( !SGPath::standardLocation(SGPath::DESKTOP  ).isNull() );
    SG_VERIFY( !SGPath::standardLocation(SGPath::DOWNLOADS).isNull() );
    SG_VERIFY( !SGPath::standardLocation(SGPath::DOCUMENTS).isNull() );
    SG_VERIFY( !SGPath::standardLocation(SGPath::PICTURES ).isNull() );
}

SGPath::Permissions validateNone(const SGPath&)
{
  SGPath::Permissions p;
  p.read = false;
  p.write = false;
  return p;
}

SGPath::Permissions validateRead(const SGPath&)
{
  SGPath::Permissions p;
  p.read = true;
  p.write = false;
  return p;
}

SGPath::Permissions validateWrite(const SGPath&)
{
  SGPath::Permissions p;
  p.read = false;
  p.write = true;
  return p;
}

void test_path_dir()
{
	simgear::Dir temp = simgear::Dir::tempDir("path_dir");
	temp.remove(true);
    SGPath p = temp.path();

	SG_VERIFY(p.isAbsolute());
	SG_CHECK_EQUAL(p.create_dir(0755), 0);

	SGPath sub = p / "subA" / "subB";
	SG_VERIFY(!sub.exists());

	SGPath subFile = sub / "fileABC.txt";
	SG_CHECK_EQUAL(subFile.create_dir(0755), 0);
	SG_VERIFY(!subFile.exists());

	sub.set_cached(false);
	SG_VERIFY(sub.exists());
	SG_VERIFY(sub.isDir());

	SGPath sub2 = p / "subA" / "fileA";
	{
		sg_ofstream os(sub2);
        SG_VERIFY(os.is_open());
		for (int i = 0; i < 50; ++i) {
			os << "ABCD" << endl;
		}
	}
	SG_VERIFY(sub2.isFile());
	SG_CHECK_EQUAL(sub2.sizeInBytes(), 250);

    SGPath sub3 = p / "subß" / "file𝕽";
    sub3.create_dir(0755);

    {
        sg_ofstream os(sub3);
        SG_VERIFY(os.is_open());
        for (int i = 0; i < 20; ++i) {
            os << "EFGH" << endl;
        }
    }

    sub3.set_cached(false);
    SG_VERIFY(sub3.exists());
    SG_CHECK_EQUAL(sub3.sizeInBytes(), 100);
    SG_CHECK_EQUAL(sub3.file(), "file𝕽");

	simgear::Dir subD(p / "subA");
	simgear::PathList dirChildren = subD.children(simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);
	SG_CHECK_EQUAL(dirChildren.size(), 1);
	SG_CHECK_EQUAL(dirChildren[0], subD.path() / "subB");

	simgear::PathList fileChildren = subD.children(simgear::Dir::TYPE_FILE | simgear::Dir::NO_DOT_OR_DOTDOT);
	SG_CHECK_EQUAL(fileChildren.size(), 1);
	SG_CHECK_EQUAL(fileChildren[0], subD.path() / "fileA");

    simgear::Dir subS(sub3.dirPath());
    fileChildren = subS.children(simgear::Dir::TYPE_FILE | simgear::Dir::NO_DOT_OR_DOTDOT);
    SG_CHECK_EQUAL(fileChildren.size(), 1);
    SG_CHECK_EQUAL(fileChildren[0], subS.path() / "file𝕽");

}

void test_permissions()
{
    // start with an empty dir
    SGPath p = simgear::Dir::current().path() / "path_test_permissions";
    simgear::Dir pd(p);
    if (pd.exists()) {
        pd.removeChildren();
    } else {
        pd.create(0700);
    }
    
    SGPath fileInRO = p / "read-only" / "fileA";
    SG_CHECK_EQUAL(fileInRO.create_dir(0500), 0);

    SG_VERIFY(!fileInRO.exists());
    SG_VERIFY(!fileInRO.canWrite());

    fileInRO.setPermissionChecker(&validateRead);
    SG_CHECK_EQUAL(fileInRO.canRead(), false);
    SG_CHECK_EQUAL(fileInRO.canWrite(), false);

    fileInRO.setPermissionChecker(&validateWrite);
    SG_CHECK_EQUAL(fileInRO.canRead(), false);
    SG_CHECK_EQUAL(fileInRO.canWrite(), false);

    /////////
    SGPath fileInRW = p / "read-write" / "fileA";

    fileInRW.setPermissionChecker(&validateRead);
    SG_CHECK_EQUAL(fileInRW.canRead(), false);
    SG_CHECK_EQUAL(fileInRW.canWrite(), false);
    SG_CHECK_EQUAL(fileInRW.create_dir(0700), -3); // not permitted

    fileInRW.setPermissionChecker(nullptr);
    SG_CHECK_EQUAL(fileInRW.create_dir(0700), 0);

    SG_VERIFY(!fileInRW.exists());
    SG_VERIFY(fileInRW.canWrite());

    fileInRW.setPermissionChecker(&validateRead);
    SG_CHECK_EQUAL(fileInRW.canRead(), false);
    SG_CHECK_EQUAL(fileInRW.canWrite(), false);

    fileInRW.setPermissionChecker(&validateWrite);
    SG_CHECK_EQUAL(fileInRW.canRead(), false);
    SG_CHECK_EQUAL(fileInRW.canWrite(), true);


    // create the file
    {
        sg_ofstream os(fileInRW);
        SG_VERIFY(os.is_open());
        os << "nonense" << endl;
    }

    // should now be readable + writeable
    fileInRW.set_cached(false);
    fileInRW.setPermissionChecker(nullptr);
    SG_VERIFY(fileInRW.exists());
    SG_VERIFY(fileInRW.canWrite());
    SG_VERIFY(fileInRW.canRead());

    fileInRW.setPermissionChecker(&validateRead);
    SG_CHECK_EQUAL(fileInRW.canRead(), true);
    SG_CHECK_EQUAL(fileInRW.canWrite(), false);

    fileInRW.setPermissionChecker(&validateWrite);
    SG_CHECK_EQUAL(fileInRW.canRead(), false);
    SG_CHECK_EQUAL(fileInRW.canWrite(), true);

    // mark the file as read-only
#if defined(SG_WINDOWS)

#else
    ::chmod(fileInRW.c_str(), 0400);
#endif

    fileInRW.set_cached(false);
    fileInRW.setPermissionChecker(nullptr);

    SG_VERIFY(fileInRW.exists());
    SG_CHECK_EQUAL(fileInRW.canWrite(), false);
    SG_VERIFY(fileInRW.canRead());

    fileInRW.setPermissionChecker(&validateRead);
    SG_CHECK_EQUAL(fileInRW.canRead(), true);
    SG_CHECK_EQUAL(fileInRW.canWrite(), false);

    fileInRW.setPermissionChecker(&validateWrite);
    SG_CHECK_EQUAL(fileInRW.canRead(), false);
    SG_CHECK_EQUAL(fileInRW.canWrite(), false);
}

int main(int argc, char* argv[])
{
    SGPath pa;
    SG_VERIFY(pa.isNull());
    SG_CHECK_EQUAL(pa.exists(), false);
    
// test basic parsing
    SGPath pb("/Foo/bar/something.png");
    SG_CHECK_EQUAL(pb.utf8Str(), std::string("/Foo/bar/something.png"));
    SG_CHECK_EQUAL(pb.local8BitStr(), std::string("/Foo/bar/something.png"));
    SG_CHECK_EQUAL(pb.dir(), std::string("/Foo/bar"));
    SG_CHECK_EQUAL(pb.file(), std::string("something.png"));
    SG_CHECK_EQUAL(pb.base(), std::string("/Foo/bar/something"));
    SG_CHECK_EQUAL(pb.file_base(), std::string("something"));
    SG_CHECK_EQUAL(pb.extension(), std::string("png"));
    SG_VERIFY(pb.isAbsolute());
    SG_VERIFY(!pb.isRelative());
    
// relative paths
    SGPath ra("where/to/begin.txt");
    SG_CHECK_EQUAL(ra.utf8Str(), std::string("where/to/begin.txt"));
    SG_CHECK_EQUAL(ra.local8BitStr(), std::string("where/to/begin.txt"));
    SG_CHECK_EQUAL(ra.dir(), std::string("where/to"));
    SG_CHECK_EQUAL(ra.file(), std::string("begin.txt"));
    SG_CHECK_EQUAL(ra.file_base(), std::string("begin"));
    SG_VERIFY(!ra.isAbsolute());
    SG_VERIFY(ra.isRelative());
    
// dots in paths / missing extensions
    SGPath pk("/Foo/bar.dot/thing");
    SG_CHECK_EQUAL(pk.dir(), std::string("/Foo/bar.dot"));
    SG_CHECK_EQUAL(pk.file(), std::string("thing"));
    SG_CHECK_EQUAL(pk.base(), std::string("/Foo/bar.dot/thing"));
    SG_CHECK_EQUAL(pk.file_base(), std::string("thing"));
    SG_CHECK_EQUAL(pk.extension(), std::string());
    
// multiple file extensions
    SGPath pj("/Foo/zot.dot/thing.tar.gz");
    SG_CHECK_EQUAL(pj.dir(), std::string("/Foo/zot.dot"));
    SG_CHECK_EQUAL(pj.file(), std::string("thing.tar.gz"));
    SG_CHECK_EQUAL(pj.base(), std::string("/Foo/zot.dot/thing.tar"));
    SG_CHECK_EQUAL(pj.file_base(), std::string("thing"));
    SG_CHECK_EQUAL(pj.extension(), std::string("gz"));
    SG_CHECK_EQUAL(pj.complete_lower_extension(), std::string("tar.gz"));
    
// path fixing
    SGPath rd("where\\to\\begin.txt");
    SG_CHECK_EQUAL(rd.utf8Str(), std::string("where/to/begin.txt"));
    
// test modification
// append
    SGPath d1("/usr/local");
    SGPath pc = d1;
    SG_CHECK_EQUAL(pc.utf8Str(), std::string("/usr/local"));
    pc.append("include");
    
    SG_CHECK_EQUAL(pc.utf8Str(), std::string("/usr/local/include"));
    SG_CHECK_EQUAL(pc.file(), std::string("include"));

// concat
    SGPath pd = pb;
    pd.concat("-1");
    SG_CHECK_EQUAL(pd.utf8Str(), std::string("/Foo/bar/something.png-1"));
    
// create with relative path
    SGPath rb(d1, "include/foo");
    SG_CHECK_EQUAL(rb.utf8Str(), std::string("/usr/local/include/foo"));
    SG_VERIFY(rb.isAbsolute());
    
// lower-casing of file extensions
    SGPath extA("FOO.ZIP");
    SG_CHECK_EQUAL(extA.base(), "FOO");
    SG_CHECK_EQUAL(extA.extension(), "ZIP");
    SG_CHECK_EQUAL(extA.lower_extension(), "zip");
    SG_CHECK_EQUAL(extA.complete_lower_extension(), "zip");
    
    SGPath extB("BAH/FOO.HTML.GZ");
    SG_CHECK_EQUAL(extB.extension(), "GZ");
    SG_CHECK_EQUAL(extB.base(), "BAH/FOO.HTML");
    SG_CHECK_EQUAL(extB.lower_extension(), "gz");
    SG_CHECK_EQUAL(extB.complete_lower_extension(), "html.gz");
#ifdef _WIN32
    SGPath winAbs("C:\\Windows\\System32");
    SG_CHECK_EQUAL(winAbs.local8BitStr(), std::string("C:/Windows/System32"));

#endif
  
// paths with only the file components
    SGPath pf("something.txt.gz");
    SG_CHECK_EQUAL(pf.base(), "something.txt");
    SG_CHECK_EQUAL(pf.file(), "something.txt.gz");
    SG_CHECK_EQUAL(pf.dir(), "");
    SG_CHECK_EQUAL(pf.lower_extension(), "gz");
    SG_CHECK_EQUAL(pf.complete_lower_extension(), "txt.gz");

    SG_CHECK_EQUAL(pf.canRead(), false);
    SG_CHECK_EQUAL(pf.canWrite(), false);

    SGPath pp(&validateNone);
    SG_CHECK_EQUAL(pp.canRead(), false);
    SG_CHECK_EQUAL(pp.canWrite(), false);

    test_dir();
    
	test_path_dir();

    test_permissions();

    cout << "all tests passed OK" << endl;
    return 0; // passed
}

