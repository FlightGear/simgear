
#include <simgear/compiler.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

using std::cout;
using std::cerr;
using std::endl;

#define COMPARE(a, b) \
    if ((a) != (b))  { \
        cerr << "failed:" << #a << " != " << #b << endl; \
        exit(1); \
    }

#define VERIFY(a) \
    if (!(a))  { \
        cerr << "failed:" << #a << endl; \
        exit(1); \
    }
    
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>

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

int main(int argc, char* argv[])
{
    SGPath pa;
    VERIFY(pa.isNull());
    COMPARE(pa.exists(), false);
    
// test basic parsing
    SGPath pb("/Foo/bar/something.png");
    COMPARE(pb.str(), std::string("/Foo/bar/something.png"));
    COMPARE(strcmp(pb.c_str(), "/Foo/bar/something.png"), 0);
    COMPARE(pb.dir(), std::string("/Foo/bar"));
    COMPARE(pb.file(), std::string("something.png"));
    COMPARE(pb.base(), std::string("/Foo/bar/something"));
    COMPARE(pb.file_base(), std::string("something"));
    COMPARE(pb.extension(), std::string("png"));
    VERIFY(pb.isAbsolute());
    VERIFY(!pb.isRelative());
    
// relative paths
    SGPath ra("where/to/begin.txt");
    COMPARE(ra.str(), std::string("where/to/begin.txt"));
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
    COMPARE(rd.str(), std::string("where/to/begin.txt"));
    
// test modification
// append
    SGPath d1("/usr/local");
    SGPath pc = d1;
    COMPARE(pc.str(), std::string("/usr/local"));
    pc.append("include");
    
    COMPARE(pc.str(), std::string("/usr/local/include"));
    COMPARE(pc.file(), std::string("include"));
    
// add
    pc.add("/opt/local");
    COMPARE(pc.str(), std::string("/usr/local/include/:/opt/local"));
    
// concat
    SGPath pd = pb;
    pd.concat("-1");
    COMPARE(pd.str(), std::string("/Foo/bar/something.png-1"));
    
// create with relative path
    SGPath rb(d1, "include/foo");
    COMPARE(rb.str(), std::string("/usr/local/include/foo"));
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
    COMPARE(d1.str_native(), std::string("\\usr\\local"));
    
    SGPath winAbs("C:\\Windows\\System32");
    COMPARE(winAbs.str(), std::string("C:/Windows/System32"));
#else
    COMPARE(d1.str_native(), std::string("/usr/local"));
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

    pp.setPermissonChecker(&validateRead);
    COMPARE(pp.canRead(), true);
    COMPARE(pp.canWrite(), false);
    COMPARE(pp.create_dir(0700), -3);

    pp.setPermissonChecker(&validateWrite);
    COMPARE(pp.canRead(), false);
    COMPARE(pp.canWrite(), true);

    test_dir();
    
    cout << "all tests passed OK" << endl;
    return 0; // passed
}

