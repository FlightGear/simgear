#! /bin/sh

echo ""
echo "running $0 to rebuild simgear include links"

# toast the old directory
rm -rf src/simgear
mkdir src/simgear
mkdir src/simgear/bucket
mkdir src/simgear/debug
mkdir src/simgear/math
mkdir src/simgear/misc
mkdir src/simgear/screen
mkdir src/simgear/serial
mkdir src/simgear/xgl
mkdir src/simgear/zlib

# build new links
ln -s ../include/compiler.h src/simgear/compiler.h
ln -s ../include/constants.h src/simgear/constants.h
ln -s ../include/fg_traits.hxx src/simgear/fg_traits.hxx
ln -s ../include/fg_zlib.h src/simgear/fg_zlib.h

ln -s ../../bucket/newbucket.hxx src/simgear/bucket/newbucket.hxx

ln -s ../../debug/debug_types.h src/simgear/debug/debug_types.h
ln -s ../../debug/logstream.hxx src/simgear/debug/logstream.hxx

ln -s ../../math/fg_memory.h src/simgear/math/fg_memory.h
ln -s ../../math/fg_types.hxx src/simgear/math/fg_types.hxx
ln -s ../../math/mat3.h src/simgear/math/mat3.h
ln -s ../../math/point3d.hxx src/simgear/math/point3d.hxx
ln -s ../../math/polar3d.hxx src/simgear/math/polar3d.hxx

ln -s ../../misc/fgpath.hxx src/simgear/misc/fgpath.hxx
ln -s ../../misc/fgstream.hxx src/simgear/misc/fgstream.hxx
ln -s ../../misc/zfstream.hxx src/simgear/misc/zfstream.hxx

ln -s ../../xgl/xgl.h src/simgear/xgl/xgl.h
