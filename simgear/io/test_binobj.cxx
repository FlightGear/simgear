
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>
#include <cstdlib>
#include <cstdio>

#if defined _MSC_VER || defined _WIN32_WINNT
#   define  random  rand
#endif

#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/test_macros.hxx>

#include "sg_binobj.hxx"

using std::cout;
using std::cerr;
using std::endl;
using std::string;


void generate_points(int count, std::vector<SGVec3d>& vec)
{
    for (int i=0; i<count; ++i) {
        vec.push_back(SGVec3d(i * 0.5, i * i, i * 4));
    }
}

void generate_normals(int count, std::vector<SGVec3f>& vec)
{
    for (int i=0; i<count; ++i) {
        vec.push_back(normalize(SGVec3f(i, i * 2, i * -4)));
    }
}

void generate_tcs(int count, std::vector<SGVec2f>& vec)
{
    for (int i=0; i<count; ++i) {
        vec.push_back(SGVec2f(1.0 / i, 16.0 / i));
    }
}
    
void test_empty()
{
    SGBinObject empty;
    SGPath path(simgear::Dir::current().file("empty.btg.gz"));
    bool ok = empty.write_bin_file(path);
    SG_VERIFY(ok);
    SGBinObject rd;
    ok = rd.read_bin(path) ;
    SG_VERIFY(ok);

    SG_CHECK_EQUAL(rd.get_wgs84_nodes().size(), 0);
    SG_VERIFY(rd.get_pt_materials().empty());
}

void comparePoints(const SGBinObject& rd, const std::vector<SGVec3d>& b)
{
    for (unsigned int i=1; i<b.size(); i += 10) {
        SGVec3d pos = rd.get_wgs84_nodes()[i];
        pos += rd.get_gbs_center();
     
       if (!equivalent(pos, b[i], 0.1)) {
            cout << "i=" << i << endl;
            cout << b[i] << endl;
            cout << pos << endl;
        }
        
        SG_VERIFY(equivalent(pos, b[i], 0.1));        
    }
}

void compareTexCoords(const SGBinObject& rd, const std::vector<SGVec2f>& b)
{
    for (unsigned int i=1; i<b.size(); i += 10) {
        SGVec2f pos = rd.get_texcoords()[i];
        SG_VERIFY(equivalent(pos, b[i], 0.001f));        
    }
}

int_list make_tri(int maxIndex)
{
    int_list r;
    int a, b, c;
        
    bool valid = false;
    int retry  = 10;
    
    while(!valid && retry--) {
        a = (random() % maxIndex);
        b = (random() % maxIndex);
        c = (random() % maxIndex);
    
        valid = ( (a!=b) && (b!=c) && (c!=a) );
    }
    
    if (!valid) {
        cerr << "can't generate valid triangle" << endl;
        exit(1);
    }
    
    r.push_back(a);
    r.push_back(b);
    r.push_back(c);
    
    return r;
}

tci_list make_tri_tcs(int maxIndex)
{
    tci_list tci;
    tci[0].push_back(random() % maxIndex);
    tci[0].push_back(random() % maxIndex);
    tci[0].push_back(random() % maxIndex);
    return tci;
}

void compareTris(const SGBinObject& a, const SGBinObject& b)
{
    unsigned int count = a.get_tri_materials().size();
    for (unsigned int i=0; i<count; i += 39) {
        const int_list& vA(a.get_tris_v()[i]);
        const int_list& vB(b.get_tris_v()[i]);
        SG_VERIFY(vA == vB);

        SG_CHECK_EQUAL(a.get_tri_materials()[i], b.get_tri_materials()[i]);

        const int_list& tA(a.get_tris_tcs()[i][0]);
        const int_list& tB(b.get_tris_tcs()[i][0]);
        SG_VERIFY(tA == tB);
    }
}

void generate_tris(SGBinObject& b, int count)
{
    group_list v, n;
    group_tci_list tc;
    string_list materials;

    int maxVertices = b.get_wgs84_nodes().size();
    int maxNormals = b.get_normals().size();
    int maxTCs = b.get_texcoords().size();
    
    SGBinObjectTriangle sgboTri;
    for (int t=0; t<count; ++t) {
        sgboTri.material = "material1";    
        sgboTri.v_list = make_tri(maxVertices);
        sgboTri.n_list = make_tri(maxNormals);
        sgboTri.tc_list[0] = make_tri(maxTCs);

        b.add_triangle( sgboTri );
    }
}
 
void test_basic()
{
    SGBinObject basic;
    SGPath path(simgear::Dir::current().file("basic.btg.gz"));
    
    SGVec3d center(1, 2, 3);
    basic.set_gbs_center(center);
    basic.set_gbs_radius(12345);
    
    std::vector<SGVec3d> points;
    generate_points(1024, points);
    std::vector<SGVec3f> normals;
    generate_normals(1024, normals);
    std::vector<SGVec2f> texCoords;
    generate_tcs(10000, texCoords);
    
    basic.set_wgs84_nodes(points);
    basic.set_normals(normals);
    basic.set_texcoords(texCoords);
    
    bool ok = basic.write_bin_file(path);
    SG_VERIFY( ok );
    
    SGBinObject rd;
   ok = rd.read_bin(path) ;
   SG_VERIFY( ok);
   // Should be version 7 since indices are < 2^16
   SG_CHECK_EQUAL(rd.get_version(), 7);
   SG_CHECK_EQUAL(rd.get_gbs_center(), center);
   SG_CHECK_EQUAL(rd.get_gbs_radius(), 12345);
   SG_CHECK_EQUAL(rd.get_wgs84_nodes().size(), points.size());

   comparePoints(rd, points);
   compareTexCoords(rd, texCoords);
}
        
void test_many_tcs()
{
    SGBinObject basic;
    SGPath path(simgear::Dir::current().file("many_tex.btg.gz"));
    
    SGVec3d center(1, 2, 3);
    basic.set_gbs_center(center);
    basic.set_gbs_radius(12345);
    
    std::vector<SGVec3d> points;
    generate_points(10000, points);
    std::vector<SGVec3f> normals;
    generate_normals(1024, normals);
    std::vector<SGVec2f> texCoords;
    generate_tcs(100000, texCoords);
    
    basic.set_wgs84_nodes(points);
    basic.set_normals(normals);
    basic.set_texcoords(texCoords);
    
    generate_tris(basic, 20000);
    
    bool ok = basic.write_bin_file(path);
    SG_VERIFY( ok );
    
    SGBinObject rd;
   ok = rd.read_bin(path) ;
   SG_VERIFY( ok);
   // Should be version 10 since indices are > 2^16
   SG_CHECK_EQUAL(rd.get_version(), 10);
   SG_CHECK_EQUAL(rd.get_wgs84_nodes().size(), points.size());
   SG_CHECK_EQUAL(rd.get_texcoords().size(), texCoords.size());

   comparePoints(rd, points);
   compareTexCoords(rd, texCoords);
   compareTris(basic, rd);
}

void test_big()
{
    SGBinObject basic;
    SGPath path(simgear::Dir::current().file("big.btg.gz"));
    
    SGVec3d center(1, 2, 3);
    basic.set_gbs_center(center);
    basic.set_gbs_radius(12345);
    
    std::vector<SGVec3d> points;
    generate_points(200000, points);
    std::vector<SGVec3f> normals;
    generate_normals(1024, normals);
    std::vector<SGVec2f> texCoords;
    generate_tcs(300000, texCoords);
    
    basic.set_wgs84_nodes(points);
    basic.set_normals(normals);
    basic.set_texcoords(texCoords);
    
    generate_tris(basic, 200000);
    
    bool ok = basic.write_bin_file(path);
    SG_VERIFY( ok );
    
    SGBinObject rd;
   ok = rd.read_bin(path) ;
   SG_VERIFY( ok);
   // Should be version 10 since indices are > 2^16
   SG_CHECK_EQUAL(rd.get_version(), 10);
   SG_CHECK_EQUAL(rd.get_wgs84_nodes().size(), points.size());
   SG_CHECK_EQUAL(rd.get_texcoords().size(), texCoords.size());

   comparePoints(rd, points);
   compareTexCoords(rd, texCoords);
   compareTris(basic, rd);
}

void test_some_objects()
{
    SGBinObject basic;
    SGPath path(simgear::Dir::current().file("some_objects.btg.gz"));
    
    SGVec3d center(1, 2, 3);
    basic.set_gbs_center(center);
    basic.set_gbs_radius(12345);
    
    std::vector<SGVec3d> points;
    generate_points(10000, points);
    std::vector<SGVec3f> normals;
    generate_normals(1024, normals);
    std::vector<SGVec2f> texCoords;
    generate_tcs(20000, texCoords);
    
    basic.set_wgs84_nodes(points);
    basic.set_normals(normals);
    basic.set_texcoords(texCoords);
    
    generate_tris(basic, 30000); // a number smaller than 2^15!
    
    bool ok = basic.write_bin_file(path);
    SG_VERIFY( ok );
    
    SGBinObject rd;
    ok = rd.read_bin(path) ;
    SG_VERIFY( ok);
    SG_CHECK_EQUAL(rd.get_version(), 7); // since we have less than 2^15 tris
    SG_CHECK_EQUAL(rd.get_wgs84_nodes().size(), points.size());
    SG_CHECK_EQUAL(rd.get_texcoords().size(), texCoords.size());

    comparePoints(rd, points);
    compareTexCoords(rd, texCoords);
    compareTris(basic, rd);
}

void test_many_objects()
{
    SGBinObject basic;
    SGPath path(simgear::Dir::current().file("many_objects.btg.gz"));
    
    SGVec3d center(1, 2, 3);
    basic.set_gbs_center(center);
    basic.set_gbs_radius(12345);
    
    std::vector<SGVec3d> points;
    generate_points(10000, points);
    std::vector<SGVec3f> normals;
    generate_normals(1024, normals);
    std::vector<SGVec2f> texCoords;
    generate_tcs(20000, texCoords);
    
    basic.set_wgs84_nodes(points);
    basic.set_normals(normals);
    basic.set_texcoords(texCoords);
    
    generate_tris(basic, 200000);
    
    bool ok = basic.write_bin_file(path);
    SG_VERIFY( ok );
    
    SGBinObject rd;
    ok = rd.read_bin(path) ;
    SG_VERIFY( ok);
    // Should be version 10 since indices are > 2^16
    SG_CHECK_EQUAL(rd.get_version(), 10);
    SG_CHECK_EQUAL(rd.get_wgs84_nodes().size(), points.size());
    SG_CHECK_EQUAL(rd.get_texcoords().size(), texCoords.size());

    comparePoints(rd, points);
    compareTexCoords(rd, texCoords);
    compareTris(basic, rd);
}

int main(int argc, char* argv[])
{
    test_empty();
    test_basic();
    test_many_tcs();
    test_big();
    test_some_objects();
    test_many_objects();
    
    return 0;
}
