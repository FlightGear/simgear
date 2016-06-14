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

#include "untar.hxx"

#include <cstdlib>
#include <cassert>
#include <stdint.h>
#include <cstring>
#include <cstddef>
#include <algorithm>

#include <zlib.h>

#include <simgear/io/sg_file.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <simgear/debug/logstream.hxx>

namespace simgear
{

const int ZLIB_DECOMPRESS_BUFFER_SIZE = 32 * 1024;
const int ZLIB_INFLATE_WINDOW_BITS = MAX_WBITS;
const int ZLIB_DECODE_GZIP_HEADER = 16;

/* tar Header Block, from POSIX 1003.1-1990.  */

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
#define TMAGLEN  6
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

class TarExtractorPrivate
{
public:
    typedef enum {
        INVALID = 0,
        READING_HEADER,
        READING_FILE,
        READING_PADDING,
        PRE_END_OF_ARCHVE,
        END_OF_ARCHIVE,
        ERROR_STATE, ///< states above this are error conditions
        BAD_ARCHIVE,
        BAD_DATA
    } State;

    SGPath path;
    State state;
    union {
        UstarHeaderBlock header;
        uint8_t headerBytes[TAR_HEADER_BLOCK_SIZE];
    };

    size_t bytesRemaining;
    std::auto_ptr<SGFile> currentFile;
    size_t currentFileSize;
    z_stream zlibStream;
    uint8_t* zlibOutput;
    bool haveInitedZLib;
    uint8_t* headerPtr;

    TarExtractorPrivate() :
        haveInitedZLib(false)
    {
    }

    ~TarExtractorPrivate()
    {
        free(zlibOutput);
    }

    void checkEndOfState()
    {
        if (bytesRemaining > 0) {
            return;
        }

        if (state == READING_FILE) {
            currentFile->close();
            size_t pad = currentFileSize % TAR_HEADER_BLOCK_SIZE;
            if (pad) {
                bytesRemaining = TAR_HEADER_BLOCK_SIZE - pad;
                setState(READING_PADDING);
            } else {
                setState(READING_HEADER);
            }
        } else if (state == READING_HEADER) {
            processHeader();
        } else if (state == PRE_END_OF_ARCHVE) {
            if (headerIsAllZeros()) {
                setState(END_OF_ARCHIVE);
            } else {
                // what does the spec say here?
            }
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
            SG_LOG(SG_IO, SG_WARN, "magic is wrong");
            state = BAD_ARCHIVE;
            return;
        }

        std::string tarPath = std::string(header.prefix) + std::string(header.fileName);

        if (!isSafePath(tarPath)) {
            //state = BAD_ARCHIVE;
            SG_LOG(SG_IO, SG_WARN, "bad tar path:" << tarPath);
            //return;
        }

        SGPath p = path;
        p.append(tarPath);

        if (header.typeflag == DIRTYPE) {
            Dir dir(p);
            dir.create(0755);
            setState(READING_HEADER);
        } else if ((header.typeflag == REGTYPE) || (header.typeflag == AREGTYPE)) {
            currentFileSize = ::strtol(header.size, NULL, 8);
            bytesRemaining = currentFileSize;
            currentFile.reset(new SGBinaryFile(p.str()));
            currentFile->open(SG_IO_OUT);
            setState(READING_FILE);
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
            currentFile->write(bytes, curBytes);
            bytesRemaining -= curBytes;
        } else if ((state == READING_HEADER) || (state == PRE_END_OF_ARCHVE) || (state == END_OF_ARCHIVE)) {
            memcpy(headerPtr, bytes, curBytes);
            bytesRemaining -= curBytes;
            headerPtr += curBytes;
        } else if (state == READING_PADDING) {
            bytesRemaining -= curBytes;
        }

        checkEndOfState();
        if (count > curBytes) {
            // recurse with unprocessed bytes
            processBytes(bytes + curBytes, count - curBytes);
        }
    }

    bool headerIsAllZeros() const
    {
        char* headerAsChar = (char*) &header;
        for (size_t i=0; i < offsetof(UstarHeaderBlock, magic); ++i) {
            if (*headerAsChar++ != 0) {
                return false;
            }
        }

        return true;
    }

    bool isSafePath(const std::string& p) const
    {
        if (p.empty()) {
            return false;
        }

        // reject absolute paths
        if (p.at(0) == '/') {
            return false;
        }

        // reject paths containing '..'
        size_t doubleDot = p.find("..");
        if (doubleDot != std::string::npos) {
            return false;
        }

        // on POSIX could use realpath to sanity check
        return true;
    }
};

TarExtractor::TarExtractor(const SGPath& rootPath) :
    d(new TarExtractorPrivate)
{

    d->path = rootPath;
    d->state = TarExtractorPrivate::INVALID;

    memset(&d->zlibStream, 0, sizeof(z_stream));
    d->zlibOutput = (unsigned char*) malloc(ZLIB_DECOMPRESS_BUFFER_SIZE);
    d->zlibStream.zalloc = Z_NULL;
    d->zlibStream.zfree = Z_NULL;

    d->zlibStream.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
    d->zlibStream.next_out = d->zlibOutput;
}

TarExtractor::~TarExtractor()
{
}

void TarExtractor::extractBytes(const char* bytes, size_t count)
{
    if (d->state >= TarExtractorPrivate::ERROR_STATE) {
        return;
    }

    d->zlibStream.next_in = (uint8_t*) bytes;
    d->zlibStream.avail_in = count;

    if (!d->haveInitedZLib) {
        if (inflateInit2(&d->zlibStream, ZLIB_INFLATE_WINDOW_BITS | ZLIB_DECODE_GZIP_HEADER) != Z_OK) {
            SG_LOG(SG_IO, SG_WARN, "inflateInit2 failed");
            d->state = TarExtractorPrivate::BAD_DATA;
            return;
        } else {
            d->haveInitedZLib = true;
            d->setState(TarExtractorPrivate::READING_HEADER);
        }
    }
    
    size_t writtenSize;

    // loop, running zlib() inflate and sending output bytes to
    // our request body handler. Keep calling inflate until no bytes are
    // written, and ZLIB has consumed all available input
    do {
        d->zlibStream.next_out = d->zlibOutput;
        d->zlibStream.avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
        int result = inflate(&d->zlibStream, Z_NO_FLUSH);
        if (result == Z_OK || result == Z_STREAM_END) {
            // nothing to do

        } else if (result == Z_BUF_ERROR) {
            // transient error, fall through
        } else {
            //  _error = result;
            SG_LOG(SG_IO, SG_WARN, "Permanent ZLib error:" << d->zlibStream.msg);
            d->state = TarExtractorPrivate::BAD_DATA;
            return;
        }

        writtenSize = ZLIB_DECOMPRESS_BUFFER_SIZE - d->zlibStream.avail_out;
        if (writtenSize > 0) {
            d->processBytes((const char*) d->zlibOutput, writtenSize);
        }

        if (result == Z_STREAM_END) {
            break;
        }
    } while ((d->zlibStream.avail_in > 0) || (writtenSize > 0));
}

bool TarExtractor::isAtEndOfArchive() const
{
    return (d->state == TarExtractorPrivate::END_OF_ARCHIVE);
}

bool TarExtractor::hasError() const
{
    return (d->state >= TarExtractorPrivate::ERROR_STATE);
}

} // of simgear
