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

#include <simgear_config.h>

#include "untar.hxx"

#include <cstdlib>
#include <cassert>
#include <stdint.h>
#include <cstring>
#include <cstddef>
#include <algorithm>

#include <zlib.h>

#include <simgear/sg_inlines.h>
#include <simgear/io/sg_file.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/strutils.hxx>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/package/unzip.h>
#include <simgear/structure/exception.hxx>

#include "ArchiveExtractor_private.hxx"

namespace simgear
{


typedef struct
{
    char fileName[100];
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
} UstarHeaderBlock;

    const size_t TAR_HEADER_BLOCK_SIZE = 512;

#define TMAGIC   "ustar"        /* ustar and a null */
#define TMAGLEN  5              // 5, not 6, becuase some files use 'ustar '
#define TVERSION "00"           /* 00 and no null */
#define TVERSLEN 2

    /* Values used in typeflag field.  */
#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

    const char PAX_GLOBAL_HEADER = 'g';
    const char PAX_FILE_ATTRIBUTES = 'x';


    ///////////////////////////////////////////////////////////////////////////////////////////////////

    const int ZLIB_DECOMPRESS_BUFFER_SIZE = 32 * 1024;
    const int ZLIB_INFLATE_WINDOW_BITS = MAX_WBITS;
    const int ZLIB_DECODE_GZIP_HEADER = 16;

    /* tar Header Block, from POSIX 1003.1-1990.  */


    class TarExtractorPrivate : public ArchiveExtractorPrivate
    {
    public:
        union {
            UstarHeaderBlock header;
            uint8_t headerBytes[TAR_HEADER_BLOCK_SIZE];
        };

        size_t bytesRemaining;
        std::unique_ptr<SGFile> currentFile;
        size_t currentFileSize;

        uint8_t* headerPtr;
        bool skipCurrentEntry = false;
        std::string paxAttributes;
        std::string paxPathName;

        TarExtractorPrivate(ArchiveExtractor* o) : ArchiveExtractorPrivate(o)
        {
            setState(TarExtractorPrivate::READING_HEADER);
        }

        ~TarExtractorPrivate() = default;

        void readPaddingIfRequired()
        {
            size_t pad = currentFileSize % TAR_HEADER_BLOCK_SIZE;
            if (pad) {
                bytesRemaining = TAR_HEADER_BLOCK_SIZE - pad;
                setState(READING_PADDING);
            } else {
                setState(READING_HEADER);
            }
        }

        void checkEndOfState()
        {
            if (bytesRemaining > 0) {
                return;
            }

            if (state == READING_FILE) {
                if (currentFile) {
                    currentFile->close();
                    currentFile.reset();
                }
                readPaddingIfRequired();
            } else if (state == READING_HEADER) {
                processHeader();
            } else if (state == PRE_END_OF_ARCHVE) {
                if (headerIsAllZeros()) {
                    setState(END_OF_ARCHIVE);
                } else {
                    // what does the spec say here?
                }
            } else if (state == READING_PAX_GLOBAL_ATTRIBUTES) {
                parsePAXAttributes(true);
                readPaddingIfRequired();
            } else if (state == READING_PAX_FILE_ATTRIBUTES) {
                parsePAXAttributes(false);
                readPaddingIfRequired();
            } else if (state == READING_PADDING) {
                setState(READING_HEADER);
            }
        }

        void setState(State newState)
        {
            if ((newState == READING_HEADER) || (newState == PRE_END_OF_ARCHVE)) {
                bytesRemaining = TAR_HEADER_BLOCK_SIZE;
                headerPtr = headerBytes;
            }

            state = newState;
        }

        void extractBytes(const uint8_t* bytes, size_t count) override
        {
            // uncompressed, just pass through directly
            processBytes((const char*)bytes, count);
        }

        void flush() override
        {
            // no-op for tar files, we process everything greedily
        }

        void processHeader()
        {
            if (headerIsAllZeros()) {
                if (state == PRE_END_OF_ARCHVE) {
                    setState(END_OF_ARCHIVE);
                } else {
                    setState(PRE_END_OF_ARCHVE);
                }
                return;
            }

            if (strncmp(header.magic, TMAGIC, TMAGLEN) != 0) {
                SG_LOG(SG_IO, SG_WARN, "Untar: magic is wrong");
                state = BAD_ARCHIVE;
                return;
            }

            skipCurrentEntry = false;
            std::string tarPath = std::string(header.prefix) + std::string(header.fileName);
            if (!paxPathName.empty()) {
                tarPath = paxPathName;
                paxPathName.clear(); // clear for next file
            }

            if (!isSafePath(tarPath)) {
                SG_LOG(SG_IO, SG_WARN, "unsafe tar path, skipping::" << tarPath);
                skipCurrentEntry = true;
            }

            auto result = filterPath(tarPath);
            if (result == ArchiveExtractor::Stop) {
                setState(FILTER_STOPPED);
                return;
            } else if (result == ArchiveExtractor::Skipped) {
                skipCurrentEntry = true;
            }

            SGPath p = extractRootPath() / tarPath;
            if (header.typeflag == DIRTYPE) {
                if (!skipCurrentEntry) {
                    Dir dir(p);
                    dir.create(0755);
                }
                setState(READING_HEADER);
            } else if ((header.typeflag == REGTYPE) || (header.typeflag == AREGTYPE)) {
                currentFileSize = ::strtol(header.size, NULL, 8);
                bytesRemaining = currentFileSize;
                if (!skipCurrentEntry) {
                    currentFile.reset(new SGBinaryFile(p));
                    currentFile->open(SG_IO_OUT);
                }
                setState(READING_FILE);
            } else if (header.typeflag == PAX_GLOBAL_HEADER) {
                setState(READING_PAX_GLOBAL_ATTRIBUTES);
                currentFileSize = ::strtol(header.size, NULL, 8);
                bytesRemaining = currentFileSize;
                paxAttributes.clear();
            } else if (header.typeflag == PAX_FILE_ATTRIBUTES) {
                setState(READING_PAX_FILE_ATTRIBUTES);
                currentFileSize = ::strtol(header.size, NULL, 8);
                bytesRemaining = currentFileSize;
                paxAttributes.clear();
            } else if ((header.typeflag == SYMTYPE) || (header.typeflag == LNKTYPE)) {
                SG_LOG(SG_IO, SG_WARN, "Tarball contains a link or symlink, will be skipped:" << tarPath);
                skipCurrentEntry = true;
                setState(READING_HEADER);
            } else {
                SG_LOG(SG_IO, SG_WARN, "Unsupported tar file type:" << header.typeflag);
                state = BAD_ARCHIVE;
            }
        }

        void processBytes(const char* bytes, size_t count)
        {
            if ((state >= ERROR_STATE) || (state == END_OF_ARCHIVE)) {
                return;
            }

            size_t curBytes = std::min(bytesRemaining, count);
            if (state == READING_FILE) {
                if (currentFile) {
                    currentFile->write(bytes, curBytes);
                }
                bytesRemaining -= curBytes;
            } else if ((state == READING_HEADER) || (state == PRE_END_OF_ARCHVE) || (state == END_OF_ARCHIVE)) {
                memcpy(headerPtr, bytes, curBytes);
                bytesRemaining -= curBytes;
                headerPtr += curBytes;
            } else if (state == READING_PADDING) {
                bytesRemaining -= curBytes;
            } else if ((state == READING_PAX_FILE_ATTRIBUTES) || (state == READING_PAX_GLOBAL_ATTRIBUTES)) {
                bytesRemaining -= curBytes;
                paxAttributes.append(bytes, curBytes);
            }

            checkEndOfState();
            if (count > curBytes) {
                // recurse with unprocessed bytes
                processBytes(bytes + curBytes, count - curBytes);
            }
        }

        bool headerIsAllZeros() const
        {
            char* headerAsChar = (char*)&header;
            for (size_t i = 0; i < offsetof(UstarHeaderBlock, magic); ++i) {
                if (*headerAsChar++ != 0) {
                    return false;
                }
            }

            return true;
        }

        // https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxa500/paxex.htm#paxex
        void parsePAXAttributes(bool areGlobal)
        {
            auto lineStart = 0;
            for (;;) {
                auto firstSpace = paxAttributes.find(' ', lineStart);
                auto firstEq = paxAttributes.find('=', lineStart);
                if ((firstEq == std::string::npos) || (firstSpace == std::string::npos)) {
                    SG_LOG(SG_IO, SG_WARN, "Malfroemd PAX attributes in tarfile");
                    break;
                }

                uint32_t lengthBytes = std::stoul(paxAttributes.substr(lineStart, firstSpace));
                uint32_t dataBytes = lengthBytes - (firstEq + 1);
                std::string name = paxAttributes.substr(firstSpace + 1, firstEq - (firstSpace + 1));

                // dataBytes - 1 here to trim off the trailing newline
                std::string data = paxAttributes.substr(firstEq + 1, dataBytes - 1);

                processPAXAttribute(areGlobal, name, data);

                lineStart += lengthBytes;
            }
        }

        void processPAXAttribute(bool isGlobalAttr, const std::string& attrName, const std::string& data)
        {
            if (!isGlobalAttr && (attrName == "path")) {
                // data is UTF-8 encoded path name
                paxPathName = data;
            }
        }
};

///////////////////////////////////////////////////////////////////////////////


class GZTarExtractor : public TarExtractorPrivate
{
public:
    GZTarExtractor(ArchiveExtractor* outer) : TarExtractorPrivate(outer)
    {
        memset(&zlibStream, 0, sizeof(z_stream));
        zlibOutput = (unsigned char*)malloc(ZLIB_DECOMPRESS_BUFFER_SIZE);
        zlibStream.zalloc = Z_NULL;
        zlibStream.zfree = Z_NULL;
        zlibStream.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
        zlibStream.next_out = zlibOutput;
    }

    ~GZTarExtractor()
    {
        if (haveInitedZLib) {
            inflateEnd(&zlibStream);
        }
        free(zlibOutput);
    }

    void extractBytes(const uint8_t* bytes, size_t count) override
    {
        zlibStream.next_in = (uint8_t*)bytes;
        zlibStream.avail_in = count;

        if (!haveInitedZLib) {
            // now we have data, see if we're dealing with GZ-compressed data or not
            if ((bytes[0] == 0x1f) && (bytes[1] == 0x8b)) {
                // GZIP identification bytes
                if (inflateInit2(&zlibStream, ZLIB_INFLATE_WINDOW_BITS | ZLIB_DECODE_GZIP_HEADER) != Z_OK) {
                    SG_LOG(SG_IO, SG_WARN, "inflateInit2 failed");
                    state = TarExtractorPrivate::BAD_DATA;
                    return;
                }
            } else {
                // set error state
                state = TarExtractorPrivate::BAD_DATA;
                return;
            }

            haveInitedZLib = true;
            setState(TarExtractorPrivate::READING_HEADER);
        } // of init on first-bytes case

        size_t writtenSize;
        // loop, running zlib() inflate and sending output bytes to
        // our request body handler. Keep calling inflate until no bytes are
        // written, and ZLIB has consumed all available input
        do {
            zlibStream.next_out = zlibOutput;
            zlibStream.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
            int result = inflate(&zlibStream, Z_NO_FLUSH);
            if (result == Z_OK || result == Z_STREAM_END) {
                // nothing to do

            } else if (result == Z_BUF_ERROR) {
                // transient error, fall through
            } else {
                //  _error = result;
                SG_LOG(SG_IO, SG_WARN, "Permanent ZLib error:" << zlibStream.msg);
                state = TarExtractorPrivate::BAD_DATA;
                return;
            }

            writtenSize = ZLIB_DECOMPRESS_BUFFER_SIZE - zlibStream.avail_out;
            if (writtenSize > 0) {
                processBytes((const char*)zlibOutput, writtenSize);
            }

            if (result == Z_STREAM_END) {
                break;
            }
        } while ((zlibStream.avail_in > 0) || (writtenSize > 0));
    }

private:
    z_stream zlibStream;
    uint8_t* zlibOutput;
    bool haveInitedZLib = false;
};

///////////////////////////////////////////////////////////////////////////////

#if 1 || defined(HAVE_XZ)

#include <lzma.h>

class XZTarExtractor : public TarExtractorPrivate
{
public:
    XZTarExtractor(ArchiveExtractor* outer) : TarExtractorPrivate(outer)
    {
        _xzStream = LZMA_STREAM_INIT;
        _outputBuffer = (uint8_t*)malloc(ZLIB_DECOMPRESS_BUFFER_SIZE);


        auto ret = lzma_stream_decoder(&_xzStream, UINT64_MAX, LZMA_TELL_ANY_CHECK);
        if (ret != LZMA_OK) {
            setState(BAD_ARCHIVE);
            return;
        }
    }

    ~XZTarExtractor()
    {
        free(_outputBuffer);
    }

    void extractBytes(const uint8_t* bytes, size_t count) override
    {
        lzma_action action = LZMA_RUN;
        _xzStream.next_in = bytes;
        _xzStream.avail_in = count;

        size_t writtenSize;
        do {
            _xzStream.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
            _xzStream.next_out = _outputBuffer;

            const auto ret = lzma_code(&_xzStream, action);

            writtenSize = ZLIB_DECOMPRESS_BUFFER_SIZE - _xzStream.avail_out;
            if (writtenSize > 0) {
                processBytes((const char*)_outputBuffer, writtenSize);
            }

            if (ret == LZMA_GET_CHECK) {
                //
            } else if (ret == LZMA_STREAM_END) {
                setState(END_OF_ARCHIVE);
                break;
            } else if (ret != LZMA_OK) {
                setState(BAD_ARCHIVE);
                break;
            }
        } while ((_xzStream.avail_in > 0) || (writtenSize > 0));
    }

    void flush() override
    {
        const auto ret = lzma_code(&_xzStream, LZMA_FINISH);
        if (ret != LZMA_STREAM_END) {
            setState(BAD_ARCHIVE);
        }
    }

private:
    lzma_stream _xzStream;
    uint8_t* _outputBuffer = nullptr;
};

#endif

///////////////////////////////////////////////////////////////////////////////

extern "C" {
	void fill_memory_filefunc(zlib_filefunc_def*);
}

class ZipExtractorPrivate : public ArchiveExtractorPrivate
{
public:
	std::string m_buffer;

	ZipExtractorPrivate(ArchiveExtractor* outer) :
		ArchiveExtractorPrivate(outer)
	{

	}

	~ZipExtractorPrivate()
	{

	}

	void extractBytes(const uint8_t* bytes, size_t count) override
	{
		// becuase the .zip central directory is at the end of the file,
		// we have no choice but to simply buffer bytes here until flush()
		// is called
		m_buffer.append((const char*) bytes, count);
	}

	void flush() override
	{
		zlib_filefunc_def memoryAccessFuncs;
		fill_memory_filefunc(&memoryAccessFuncs);

		char bufferName[128];
#if defined(SG_WINDOWS)
        ::snprintf(bufferName, 128, "%p+%llx", m_buffer.data(), m_buffer.size());
#else
		::snprintf(bufferName, 128, "%p+%lx", m_buffer.data(), m_buffer.size());
#endif
		unzFile zip = unzOpen2(bufferName, &memoryAccessFuncs);

		const size_t BUFFER_SIZE = 1024 * 1024;
		void* buf = malloc(BUFFER_SIZE);

        int result = unzGoToFirstFile(zip);
        if (result != UNZ_OK) {
            SG_LOG(SG_IO, SG_ALERT, outer->rootPath() << "failed to go to first file in archive:" << result);
            state = BAD_ARCHIVE;
            free(buf);
            unzClose(zip);
            return;
        }

        while (true) {
            extractCurrentFile(zip, (char*)buf, BUFFER_SIZE);
            if (state == FILTER_STOPPED) {
                state = END_OF_ARCHIVE;
                break;
            }

            result = unzGoToNextFile(zip);
            if (result == UNZ_END_OF_LIST_OF_FILE) {
                state = END_OF_ARCHIVE;
                break;
            } else if (result != UNZ_OK) {
                SG_LOG(SG_IO, SG_ALERT, outer->rootPath() << "failed to go to next file in archive:" << result);
                state = BAD_ARCHIVE;
                break;
            }
        }


        free(buf);
        unzClose(zip);
    }
    
    void extractCurrentFile(unzFile zip, char* buffer, size_t bufferSize)
    {
        unz_file_info fileInfo;
        int result = unzGetCurrentFileInfo(zip, &fileInfo,
                                           buffer, bufferSize,
                                           NULL, 0,  /* extra field */
                                           NULL, 0 /* comment field */);
        if (result != Z_OK) {
            throw sg_io_exception("Failed to get zip current file info");
        }
        
		std::string name(buffer);
		if (!isSafePath(name)) {
            SG_LOG(SG_IO, SG_WARN, "unsafe zip path, skipping::" << name);
            return;
		}

		auto filterResult = filterPath(name);
		if (filterResult == ArchiveExtractor::Stop) {
			state = FILTER_STOPPED;
			return;
		}
		else if (filterResult == ArchiveExtractor::Skipped) {
			return;
		}

		if (fileInfo.uncompressed_size == 0) {
			// assume it's a directory for now
			// since we create parent directories when extracting
			// a path, we're done here
			return;
		}

		result = unzOpenCurrentFile(zip);
		if (result != UNZ_OK) {
			throw sg_io_exception("opening current zip file failed", sg_location(name));
		}

		sg_ofstream outFile;
		bool eof = false;
		SGPath path = extractRootPath() / name;

		// create enclosing directory heirarchy as required
		Dir parentDir(path.dir());
		if (!parentDir.exists()) {
			bool ok = parentDir.create(0755);
			if (!ok) {
				throw sg_io_exception("failed to create directory heirarchy for extraction", path);
			}
		}

		outFile.open(path, std::ios::binary | std::ios::trunc | std::ios::out);
		if (outFile.fail()) {
			throw sg_io_exception("failed to open output file for writing:" + strutils::error_string(errno), path);
		}

		while (!eof) {
			int bytes = unzReadCurrentFile(zip, buffer, bufferSize);
			if (bytes < 0) {
				throw sg_io_exception("unzip failure reading curent archive", sg_location(name));
			}
			else if (bytes == 0) {
				eof = true;
			}
			else {
				outFile.write(buffer, bytes);
			}
		}

		outFile.close();
		unzCloseCurrentFile(zip);
	}
};

//////////////////////////////////////////////////////////////////////////////

ArchiveExtractor::ArchiveExtractor(const SGPath& rootPath) :
	_rootPath(rootPath)
{
}

ArchiveExtractor::~ArchiveExtractor() = default;

void ArchiveExtractor::extractBytes(const uint8_t* bytes, size_t count)
{
	if (!d) {
		_prebuffer.append((char*) bytes, count);
		auto r = determineType((uint8_t*) _prebuffer.data(), _prebuffer.size());
		if (r == InsufficientData) {
			return;
		}

		if (r == TarData) {
			d.reset(new TarExtractorPrivate(this));
        } else if (r == GZData) {
            d.reset(new GZTarExtractor(this));
        } else if (r == XZData) {
            d.reset(new XZTarExtractor(this));
        } else if (r == ZipData) {
            d.reset(new ZipExtractorPrivate(this));
        } else {
            SG_LOG(SG_IO, SG_WARN, "Invalid archive type");
			_invalidDataType = true;
			return;
        }

        // if hit here, we created the extractor. Feed the prefbuffer
		// bytes through it
		d->extractBytes((uint8_t*) _prebuffer.data(), _prebuffer.size());
		_prebuffer.clear();
		return;
	}

    if (d->state >= ArchiveExtractorPrivate::ERROR_STATE) {
        return;
    }

	d->extractBytes(bytes, count);
}

void ArchiveExtractor::flush()
{
	if (!d)
		return;

	d->flush();
}

bool ArchiveExtractor::isAtEndOfArchive() const
{
	if (!d)
		return false;

    return (d->state == ArchiveExtractorPrivate::END_OF_ARCHIVE);
}

bool ArchiveExtractor::hasError() const
{
	if (_invalidDataType)
		return true;

	if (!d)
		return false;

    return (d->state >= ArchiveExtractorPrivate::ERROR_STATE);
}

ArchiveExtractor::DetermineResult ArchiveExtractor::determineType(const uint8_t* bytes, size_t count)
{
	// check for ZIP
	if (count < 4) {
		return InsufficientData;
	}

	if (memcmp(bytes, "PK\x03\x04", 4) == 0) {
		return ZipData;
	}

    if (count < 6) {
        return InsufficientData;
    }

    const uint8_t XZ_HEADER[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
    if (memcmp(bytes, XZ_HEADER, 6) == 0) {
        return XZData;
    }

    auto r = isTarData(bytes, count);
    if ((r == TarData) || (r == InsufficientData) || (r == GZData)) {
        return r;
    }

    return Invalid;
}


ArchiveExtractor::DetermineResult ArchiveExtractor::isTarData(const uint8_t* bytes, size_t count)
{
    if (count < 2) {
        return InsufficientData;
    }

    UstarHeaderBlock* header = 0;
    DetermineResult result = InsufficientData;

    if ((bytes[0] == 0x1f) && (bytes[1] == 0x8b)) {
        // GZIP identification bytes
        z_stream z;
        uint8_t* zlibOutput = static_cast<uint8_t*>(alloca(4096));
        memset(&z, 0, sizeof(z_stream));
        z.zalloc = Z_NULL;
        z.zfree = Z_NULL;
        z.avail_out = 4096;
        z.next_out = zlibOutput;
        z.next_in = (uint8_t*) bytes;
        z.avail_in = count;

        if (inflateInit2(&z, ZLIB_INFLATE_WINDOW_BITS | ZLIB_DECODE_GZIP_HEADER) != Z_OK) {
            inflateEnd(&z);
            return Invalid;
        }

        int zResult = inflate(&z, Z_SYNC_FLUSH);
        if ((zResult == Z_OK) || (zResult == Z_STREAM_END)) {
            // all good
        } else {
            SG_LOG(SG_IO, SG_WARN, "isTarData: Zlib inflate failed:" << zResult);
            inflateEnd(&z);
            return Invalid; // not tar data
        }

        size_t written = 4096 - z.avail_out;
        if (written < TAR_HEADER_BLOCK_SIZE) {
            inflateEnd(&z);
            return InsufficientData;
        }

        header = reinterpret_cast<UstarHeaderBlock*>(zlibOutput);
        inflateEnd(&z);
        result = GZData;
    } else {
        // uncompressed tar
        if (count < TAR_HEADER_BLOCK_SIZE) {
            return InsufficientData;
        }

        header = (UstarHeaderBlock*) bytes;
        result = TarData;
    }

    if (strncmp(header->magic, TMAGIC, TMAGLEN) != 0) {
        return Invalid;
    }

    return result;
}

void ArchiveExtractor::extractLocalFile(const SGPath& archiveFile)
{

}

auto ArchiveExtractor::filterPath(std::string& pathToExtract)
  -> PathResult
{
    SG_UNUSED(pathToExtract);
    return Accepted;
}

} // of simgear
