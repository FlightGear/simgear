#! /bin/sh

echo ""
echo "running $0 to rebuild simgear include links"

# toast the old directory
rm -rf src/simgear
mkdir src/simgear

# build new links
ln -s ../include/compiler.h src/simgear/compiler.h
ln -s ../include/constants.h src/simgear/constants.h
ln -s ../debug/debug_types.h src/simgear/debug_types.h
ln -s ../math/fg_memory.h src/simgear/fg_memory.h
ln -s ../include/fg_traits.hxx src/simgear/fg_traits.hxx
ln -s ../math/fg_types.hxx src/simgear/fg_types.hxx
ln -s ../include/fg_zlib.h src/simgear/fg_zlib.h
ln -s ../misc/fgpath.hxx src/simgear/fgpath.hxx
ln -s ../misc/fgstream.hxx src/simgear/fgstream.hxx
ln -s ../debug/logstream.hxx src/simgear/logstream.hxx
ln -s ../math/mat3.h src/simgear/mat3.h
ln -s ../bucket/newbucket.hxx src/simgear/newbucket.hxx
ln -s ../math/point3d.hxx src/simgear/point3d.hxx
ln -s ../math/polar3d.hxx src/simgear/polar3d.hxx
ln -s ../xgl/xgl.h src/simgear/xgl.h
ln -s ../misc/zfstream.hxx src/simgear/zfstream.hxx
