#include <unistd.h>

#include "sg_binobj.hxx"


int main( int argc, char **argv ) {
    int i, j;

    // check usage
    if ( argc != 2 ) {
        cout << "Usage: " << argv[0] << " binary_obj_file" << endl;
    }

    SGBinObject obj;
    bool result = obj.read_bin( argv[1] );
    if ( !result ) {
        cout << "error loading: " << argv[1] << endl;
        exit(-1);
    }

    cout << "# FGFS Scenery" << endl;
    cout << "# Version " << obj.get_version() << endl;
    cout << endl;

    printf("# gbs %.5f %.5f %.5f %.2f\n", obj.get_gbs_center().x(), 
           obj.get_gbs_center().y(), obj.get_gbs_center().z(),
           obj.get_gbs_radius());
    cout << endl;

    point_list nodes = obj.get_wgs84_nodes();
    cout << "# vertex list" << endl;
    for ( i = 0; i < (int)nodes.size(); ++i ) {
        printf("v %.5f %.5f %.5f\n", nodes[i].x(), nodes[i].y(), nodes[i].z() );
    }
    cout << endl;

    point_list normals = obj.get_normals();
    cout << "# vertex normal list" << endl;
    for ( i = 0; i < (int)normals.size(); ++i ) {
        printf("vn %.5f %.5f %.5f\n",
               normals[i].x(), normals[i].y(), normals[i].z() );
    }
    cout << endl;

    point_list texcoords = obj.get_texcoords();
    cout << "# texture coordinate list" << endl;
    for ( i = 0; i < (int)texcoords.size(); ++i ) {
        printf("vt %.5f %.5f\n",
               texcoords[i].x(), texcoords[i].y() );
    }
    cout << endl;

    cout << "# triangle groups" << endl;
    cout << endl;

    string material;
    int_list vertex_index;
    int_list tex_index;

    // generate triangles
    string_list tri_materials = obj.get_tri_materials();
    group_list tris_v = obj.get_tris_v();
    group_list tris_tc = obj.get_tris_tc();
    for ( i = 0; i < (int)tris_v.size(); ++i ) {
	material = tri_materials[i];
	vertex_index = tris_v[i];
	tex_index = tris_tc[i];
        cout << "# usemtl " << material << endl;
        cout << "f ";
        for ( j = 0; j < (int)vertex_index.size(); ++j ) {
            cout << vertex_index[j] << "/" << tex_index[j] << " ";
        }
        cout << endl;
    }

    // generate strips
    string_list strip_materials = obj.get_strip_materials();
    group_list strips_v = obj.get_strips_v();
    group_list strips_tc = obj.get_strips_tc();
    for ( i = 0; i < (int)strips_v.size(); ++i ) {
	material = strip_materials[i];
	vertex_index = strips_v[i];
	tex_index = strips_tc[i];
        cout << "# usemtl " << material << endl;
        cout << "ts ";
        for ( j = 0; j < (int)vertex_index.size(); ++j ) {
            cout << vertex_index[j] << "/" << tex_index[j] << " ";
        }
        cout << endl;
    }

    // generate fans
    string_list fan_materials = obj.get_fan_materials();
    group_list fans_v = obj.get_fans_v();
    group_list fans_tc = obj.get_fans_tc();
    for ( i = 0; i < (int)fans_v.size(); ++i ) {
	material = fan_materials[i];
	vertex_index = fans_v[i];
	tex_index = fans_tc[i];
        cout << "# usemtl " << material << endl;
        cout << "tf ";
        for ( j = 0; j < (int)vertex_index.size(); ++j ) {
            cout << vertex_index[j] << "/" << tex_index[j] << " ";
        }
        cout << endl;
    }
}
