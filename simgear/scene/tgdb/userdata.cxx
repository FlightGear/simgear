// userdata.hxx -- two classes for populating ssg user data slots in association
//                 with our implimenation of random surface objects.
//
// Written by David Megginson, started December 2001.
//
// Copyright (C) 2001 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osgDB/Registry>

#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matmodel.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>

#include "userdata.hxx"
#include "SGReaderWriterBTG.hxx"
#include "ReaderWriterSPT.hxx"
#include "ReaderWriterSTG.hxx"
#include <simgear/scene/dem/ReaderWriterPGT.hxx>
#include <simgear/scene/model/ReaderWriterGLTF.hxx>

// the following are static values needed by the runtime object
// loader.  However, the loading is done via a call back so these
// values cannot be passed as parameters.  The calling application
// needs to call sgUserDataInit() with the appropriate values before
// building / drawing any scenery.

static bool _inited = false;
static SGPropertyNode *root_props = NULL;

// Because BTG files are now loaded through the osgDB::Registry, there
// are no symbols referenced by FlightGear in this library other than
// sgUserDataInit. But the libraries are all statically linked, so
// none of the other object files in this library would be included in
// the executable! Sticking the static proxy here forces the BTG code
// to be sucked in.
namespace {
osgDB::RegisterReaderWriterProxy<SGReaderWriterBTG> g_readerWriter_BTG_Proxy;

osgDB::RegisterReaderWriterProxy<simgear::ReaderWriterSTG> g_readerWriterSTGProxy;
simgear::ModelRegistryCallbackProxy<simgear::LoadOnlyCallback> g_stgCallbackProxy("stg");

osgDB::RegisterReaderWriterProxy<simgear::ReaderWriterSPT> g_readerWriterSPTProxy;
simgear::ModelRegistryCallbackProxy<simgear::LoadOnlyCallback> g_sptCallbackProxy("spt");

#ifdef ENABLE_GDAL
osgDB::RegisterReaderWriterProxy<simgear::ReaderWriterPGT> g_readerWriterPGTProxy;
simgear::ModelRegistryCallbackProxy<simgear::LoadOnlyCallback> g_pgtCallbackProxy("pgt");
#endif

osgDB::RegisterReaderWriterProxy<simgear::ReaderWriterGLTF> g_readerWriterGLTFProxy;
simgear::ModelRegistryCallbackProxy<simgear::LoadOnlyCallback> g_gltfCallbackProxy("gltf");
}

void sgUserDataInit( SGPropertyNode *p ) {
    _inited = true;
    root_props = p;
}

namespace simgear
{
SGPropertyNode* getPropertyRoot()
{
    return root_props;
}
}
