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

#ifndef SG_IO_UNTAR_HXX
#define SG_IO_UNTAR_HXX

#include <memory>

#include <cstdlib>
#include <cstdint>

#include <simgear/misc/sg_path.hxx>

namespace simgear
{

class ArchiveExtractorPrivate;

class ArchiveExtractor
{
public:
	ArchiveExtractor(const SGPath& rootPath);
	virtual ~ArchiveExtractor();

    enum DetermineResult {
        Invalid,
        InsufficientData,
        TarData,
        ZipData,
        GZData, // Gzipped-tar
        XZData  // XZ (aka LZMA) tar
    };

    static DetermineResult determineType(const uint8_t* bytes, size_t count);

	/**
	 * @brief API to extract a local zip or tar.gz 
	 */
	void extractLocalFile(const SGPath& archiveFile);

	/**
	 * @brief API to extract from memory - this can be called multiple
	 * times for streamking from a network socket etc
	 */
    void extractBytes(const uint8_t* bytes, size_t count);

	void flush();

    bool isAtEndOfArchive() const;

    bool hasError() const;

	enum PathResult {
		Accepted,
		Skipped,
		Modified,
		Stop
	};

    SGPath rootPath() const
    {
        return _rootPath;
    }

protected:


    virtual PathResult filterPath(std::string& pathToExtract);
private:
	static DetermineResult isTarData(const uint8_t* bytes, size_t count);

    friend class ArchiveExtractorPrivate;
    std::unique_ptr<ArchiveExtractorPrivate> d;

	SGPath _rootPath;
	std::string _prebuffer; // store bytes before type is determined
	bool _invalidDataType = false;
};

} // of namespace simgear

#endif // of SG_IO_UNTAR_HXX
