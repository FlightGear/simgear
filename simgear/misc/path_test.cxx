
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

void test_dir()
{
    simgear::Dir temp = simgear::Dir::tempDir("foo");
    cout << "created:" << temp.path() << endl;
  
    VERIFY(temp.exists());
    VERIFY(temp.path().isDir());
    VERIFY(!temp.path().isFile());
    
    SGPath fileInDir = temp.file("foobaz");
    VERIFY(!fileInDir.exists());
    
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

    VERIFY( !SGPath::standardLocation(SGPath::HOME     ).isNull() );
    VERIFY( !SGPath::standardLocation(SGPath::DESKTOP  ).isNull() );
    VERIFY( !SGPath::standardLocation(SGPath::DOWNLOADS).isNull() );
    VERIFY( !SGPath::standardLocation(SGPath::DOCUMENTS).isNull() );
    VERIFY( !SGPath::standardLocation(SGPath::PICTURES ).isNull() );
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
	SGPath p(simgear::Dir::current().path());
	p.append("path_dir");
	simgear::Dir(p).remove(true);

	VERIFY(p.isAbsolute());
	COMPARE(p.create_dir(0755), 0);

	SGPath sub = p / "subA" / "subB";
	VERIFY(!sub.exists());

	SGPath subFile = sub / "fileABC.txt";
	COMPARE(subFile.create_dir(0755), 0);
	VERIFY(!subFile.exists());

	sub.set_cached(false);
	VERIFY(sub.exists());
	VERIFY(sub.isDir());

	SGPath sub2 = p / "subA" / "fileA";
	{
		sg_ofstream os(sub2);
        VERIFY(os.is_open());
		for (int i = 0; i < 50; ++i) {
			os << "ABCD" << endl;
		}
	}
	VERIFY(sub2.isFile());
	COMPARE(sub2.sizeInBytes(), 250);

    SGPath sub3 = p / "subß" / "file𝕽";
    sub3.create_dir(0755);

    {
        sg_ofstream os(sub3);
        VERIFY(os.is_open());
        for (int i = 0; i < 20; ++i) {
            os << "EFGH" << endl;
        }
    }

    sub3.set_cached(false);
    VERIFY(sub3.exists());
    COMPARE(sub3.sizeInBytes(), 100);
    COMPARE(sub3.file(), "file𝕽");

	simgear::Dir subD(p / "subA");
	simgear::PathList dirChildren = subD.children(simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);
	COMPARE(dirChildren.size(), 1);
	COMPARE(dirChildren[0], subD.path() / "subB");

	simgear::PathList fileChildren = subD.children(simgear::Dir::TYPE_FILE | simgear::Dir::NO_DOT_OR_DOTDOT);
	COMPARE(fileChildren.size(), 1);
	COMPARE(fileChildren[0], subD.path() / "fileA");

    simgear::Dir subS(sub3.dirPath());
    fileChildren = subS.children(simgear::Dir::TYPE_FILE | simgear::Dir::NO_DOT_OR_DOTDOT);
    COMPARE(fileChildren.size(), 1);
    COMPARE(fileChildren[0], subS.path() / "file𝕽");

}

int main(int argc, char* argv[])
{
    SGPath pa;
    VERIFY(pa.isNull());
    COMPARE(pa.exists(), false);
    
// test basic parsing
    SGPath pb("/Foo/bar/something.png");
    COMPARE(pb.utf8Str(), std::string("/Foo/bar/something.png"));
    COMPARE(pb.local8BitStr(), std::string("/Foo/bar/something.png"));
    COMPARE(pb.dir(), std::string("/Foo/bar"));
    COMPARE(pb.file(), std::string("something.png"));
    COMPARE(pb.base(), std::string("/Foo/bar/something"));
    COMPARE(pb.file_base(), std::string("something"));
    COMPARE(pb.extension(), std::string("png"));
    VERIFY(pb.isAbsolute());
    VERIFY(!pb.isRelative());
    
// relative paths
    SGPath ra("where/to/begin.txt");
    COMPARE(ra.utf8Str(), std::string("where/to/begin.txt"));
    COMPARE(ra.local8BitStr(), std::string("where/to/begin.txt"));
    COMPARE(ra.dir(), std::string("where/to"));
    COMPARE(ra.file(), std::string("begin.txt"));
    COMPARE(ra.file_base(), std::string("begin"));
    VERIFY(!ra.isAbsolute());
    VERIFY(ra.isRelative());
    
// dots in paths / missing extensions
    SGPath pk("/Foo/bar.dot/thing");
    COMPARE(pk.dir(), std::string("/Foo/bar.dot"));
    COMPARE(pk.file(), std::string("thing"));
    COMPARE(pk.base(), std::string("/Foo/bar.dot/thing"));
    COMPARE(pk.file_base(), std::string("thing"));
    COMPARE(pk.extension(), std::string());
    
// multiple file extensions
    SGPath pj("/Foo/zot.dot/thing.tar.gz");
    COMPARE(pj.dir(), std::string("/Foo/zot.dot"));
    COMPARE(pj.file(), std::string("thing.tar.gz"));
    COMPARE(pj.base(), std::string("/Foo/zot.dot/thing.tar"));
    COMPARE(pj.file_base(), std::string("thing"));
    COMPARE(pj.extension(), std::string("gz"));
    COMPARE(pj.complete_lower_extension(), std::string("tar.gz"));
    
// path fixing
    SGPath rd("where\\to\\begin.txt");
    COMPARE(rd.utf8Str(), std::string("where/to/begin.txt"));
    
// test modification
// append
    SGPath d1("/usr/local");
    SGPath pc = d1;
    COMPARE(pc.utf8Str(), std::string("/usr/local"));
    pc.append("include");
    
    COMPARE(pc.utf8Str(), std::string("/usr/local/include"));
    COMPARE(pc.file(), std::string("include"));

// concat
    SGPath pd = pb;
    pd.concat("-1");
    COMPARE(pd.utf8Str(), std::string("/Foo/bar/something.png-1"));
    
// create with relative path
    SGPath rb(d1, "include/foo");
    COMPARE(rb.utf8Str(), std::string("/usr/local/include/foo"));
    VERIFY(rb.isAbsolute());
    
// lower-casing of file extensions
    SGPath extA("FOO.ZIP");
    COMPARE(extA.base(), "FOO");
    COMPARE(extA.extension(), "ZIP");
    COMPARE(extA.lower_extension(), "zip");
    COMPARE(extA.complete_lower_extension(), "zip");
    
    SGPath extB("BAH/FOO.HTML.GZ");
    COMPARE(extB.extension(), "GZ");
    COMPARE(extB.base(), "BAH/FOO.HTML");
    COMPARE(extB.lower_extension(), "gz");
    COMPARE(extB.complete_lower_extension(), "html.gz");
#ifdef _WIN32
    SGPath winAbs("C:\\Windows\\System32");
    COMPARE(winAbs.local8BitStr(), std::string("C:/Windows/System32"));

#endif
  
// paths with only the file components
    SGPath pf("something.txt.gz");
    COMPARE(pf.base(), "something.txt");
    COMPARE(pf.file(), "something.txt.gz");
    COMPARE(pf.dir(), "");
    COMPARE(pf.lower_extension(), "gz");
    COMPARE(pf.complete_lower_extension(), "txt.gz");

    COMPARE(pf.canRead(), true);
    COMPARE(pf.canWrite(), true);

    SGPath pp(&validateNone);
    COMPARE(pp.canRead(), false);
    COMPARE(pp.canWrite(), false);

    pp.append("./test-dir/file.txt");
    COMPARE(pp.create_dir(0700), -3);

    pp.setPermissionChecker(&validateRead);
    COMPARE(pp.canRead(), true);
    COMPARE(pp.canWrite(), false);
    COMPARE(pp.create_dir(0700), -3);

    pp.setPermissionChecker(&validateWrite);
    COMPARE(pp.canRead(), false);
    COMPARE(pp.canWrite(), true);

    test_dir();
    
	test_path_dir();

    cout << "all tests passed OK" << endl;
    return 0; // passed
}

