// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2017  James Turner - james@flightgear.org


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
