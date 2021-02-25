// Copyright (C) 2007 Tim Moore timoore@redhat.com
// Copyright (C) 2008 Till Busch buti@bux.at
// Copyright (C) 2011 Mathias Froehlich
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

#include <simgear_config.h>

#include <simgear/scene/util/OsgMath.hxx>

#include "SGReaderWriterOptions.hxx"

#include <osgDB/Registry>

namespace simgear
{

SGReaderWriterOptions::~SGReaderWriterOptions()
{
}

SGReaderWriterOptions*
SGReaderWriterOptions::copyOrCreate(const osgDB::Options* options)
{
    if (!options)
        options = osgDB::Registry::instance()->getOptions();
    if (!options)
        return new SGReaderWriterOptions;
    if (!dynamic_cast<const SGReaderWriterOptions*>(options))
        return new SGReaderWriterOptions(*options);
    return new SGReaderWriterOptions(*static_cast<const SGReaderWriterOptions*>(options));
}

SGReaderWriterOptions*
SGReaderWriterOptions::fromPath(const SGPath& path)
{
    SGReaderWriterOptions* options = copyOrCreate(nullptr);
    options->setDatabasePath(path.utf8Str());
    return options;
}

void SGReaderWriterOptions::addErrorContext(const std::string& key, const std::string& value)
{
    _errorContext[key] = value;
}


} // namespace simgear
