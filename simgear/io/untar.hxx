// Copyright (C) 2016  James Turner - <zakalawe@mac.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SG_PKG_UNTAR_HXX
#define SG_PKG_UNTAR_HXX

#include <memory>

#include <simgear/misc/sg_path.hxx>

namespace simgear
{

namespace pkg
{

class TarExtractorPrivate;

class TarExtractor
{
public:
    TarExtractor(const SGPath& rootPath);
	~TarExtractor();

    void extractBytes(const char* bytes, size_t count);

    bool isAtEndOfArchive() const;

    bool hasError() const;

private:
    std::auto_ptr<TarExtractorPrivate> d;
};

} // of namespace pkg

} // of namespace simgear

#endif
