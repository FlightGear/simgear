// Copyright (C) 2017  James Turner - zakalawe@mac.com
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

#include <simgear/package/Delegate.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>

namespace simgear
{

	namespace pkg
	{

		void Delegate::installStatusChanged(InstallRef aInstall, StatusCode aReason)
		{
		}

		void Delegate::dataForThumbnail(const std::string& aThumbnailUrl,
			size_t lenth, const uint8_t* bytes)
		{
		}

	} // of namespace pkg
} // of namespace simgear
