// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2016 James Turner <james@flightgear.org>

#pragma once

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
	 * times for streaming from a network socket etc
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

    // ignore the top-level directory, extracting the contents
    // directly into root-path. Needs for TerraSync archive extraction
    void setRemoveTopmostDirectory(bool doRemove);

    SGPath rootPath() const
    {
        return _rootPath;
    }

    void setCreateDirHashEntries(bool doCreate);

    SGPath mostRecentExtractedPath() const;

protected:


    virtual PathResult filterPath(std::string& pathToExtract);


private:
	static DetermineResult isTarData(const uint8_t* bytes, size_t count);

    friend class ArchiveExtractorPrivate;
    std::unique_ptr<ArchiveExtractorPrivate> d;

	SGPath _rootPath;
	std::string _prebuffer; // store bytes before type is determined
	bool _invalidDataType = false;
    bool _doCreateDirHashes = false;
    bool _removeTopmostDir = false;
};

} // of namespace simgear
