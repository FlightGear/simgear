#! /bin/sh

echo ""
echo "running $0 to rebuild simgear include links"

# toast the old directory
rm -rf simgear
mkdir simgear

# build new links
ln -s ../include/compiler.h simgear/compiler.h
ln -s ../include/constants.h simgear/constants.h
ln -s ../debug/debug_types.h simgear/debug_types.h
ln -s ../math/fg_memory.h simgear/fg_memory.h
ln -s ../include/fg_traits.hxx simgear/fg_traits.hxx
ln -s ../math/fg_types.hxx simgear/fg_types.hxx
ln -s ../include/fg_zlib.h simgear/fg_zlib.h
ln -s ../misc/fgpath.hxx simgear/fgpath.hxx
ln -s ../debug/logstream.hxx simgear/logstream.hxx
ln -s ../math/mat3.h simgear/mat3.h
ln -s ../bucket/newbucket.hxx simgear/newbucket.hxx
ln -s ../math/point3d.hxx simgear/point3d.hxx
ln -s ../math/polar3d.hxx simgear/polar3d.hxx
ln -s ../xgl/xgl.h simgear/xgl.h
ln -s ../misc/zfstream.hxx simgear/zfstream.hxx
