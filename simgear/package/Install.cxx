// Copyright (C) 2013  James Turner - zakalawe@mac.com
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

#include <simgear/package/Install.hxx>

#include <boost/foreach.hpp>
#include <fstream>

#include <simgear/package/unzip.h>
#include <simgear/package/md5.h>

#include <simgear/structure/exception.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>

extern "C" {
    void fill_memory_filefunc (zlib_filefunc_def*);
}

namespace simgear {
    
namespace pkg {

class Install::PackageArchiveDownloader : public HTTP::Request
{
public:
    PackageArchiveDownloader(Install* aOwner) :
        HTTP::Request("" /* dummy URL */),
        m_owner(aOwner)
    {
        m_urls = m_owner->package()->downloadUrls();
        if (m_urls.empty()) {
            throw sg_exception("no package download URLs");
        }
        
        // TODO randomise order of m_urls
        
        m_extractPath = aOwner->path().dir();
        m_extractPath.append("_DOWNLOAD"); // add some temporary value
        
    }
    
protected:
    virtual std::string url() const
    {
        return m_urls.front();
    }
    
    virtual void responseHeadersComplete()
    {
        std::cout << "starting download of " << m_owner->package()->id() << " from "
            << url() << std::endl;
        Dir d(m_extractPath);
        d.create(0755);        
        
        memset(&m_md5, 0, sizeof(MD5_CTX));
        MD5Init(&m_md5);
    }
    
    virtual void gotBodyData(const char* s, int n)
    {
        m_buffer += std::string(s, n);
        MD5Update(&m_md5, (unsigned char*) s, n);
    }
    
    virtual void responseComplete()
    {
        if (responseCode() != 200) {
            SG_LOG(SG_GENERAL, SG_ALERT, "download failure");
            doFailure();
            return;
        }

        MD5Final(&m_md5);
    // convert final sum to hex
        const char hexChar[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
        std::stringstream hexMd5;
        for (int i=0; i<16;++i) {
            hexMd5 << hexChar[m_md5.digest[i] >> 4];
            hexMd5 << hexChar[m_md5.digest[i] & 0x0f];
        }
        
        if (hexMd5.str() != m_owner->package()->md5()) {
            SG_LOG(SG_GENERAL, SG_ALERT, "md5 verification failed:\n"
                << "\t" << hexMd5.str() << "\n\t"
                << m_owner->package()->md5() << "\n\t"
                << "downloading from:" << url());
            doFailure();
            return;
        } else {
            std::cout << "MD5 checksum is ok" << std::endl;
        }
        
        if (!extractUnzip()) {
            SG_LOG(SG_GENERAL, SG_WARN, "zip extraction failed");
            doFailure();
            return;
        }
                  
        if (m_owner->path().exists()) {
            //std::cout << "removing existing path" << std::endl;
            Dir destDir(m_owner->path());
            destDir.remove(true /* recursive */);
        }
        
        m_extractPath.append(m_owner->package()->id());
        m_extractPath.rename(m_owner->path());        
        m_owner->m_revision = m_owner->package()->revision();
        m_owner->writeRevisionFile();
    }
    
private:

    void extractCurrentFile(unzFile zip, char* buffer, size_t bufferSize)
    {
        unz_file_info fileInfo;
        unzGetCurrentFileInfo(zip, &fileInfo, 
            buffer, bufferSize, 
            NULL, 0,  /* extra field */
            NULL, 0 /* comment field */);
            
        std::string name(buffer);
    // no absolute paths, no 'up' traversals
    // we could also look for suspicious file extensions here (forbid .dll, .exe, .so)
        if ((name[0] == '/') || (name.find("../") != std::string::npos) || (name.find("..\\") != std::string::npos)) {
            throw sg_format_exception("Bad zip path", name);
        }
        
        if (fileInfo.uncompressed_size == 0) {
            // assume it's a directory for now
            // since we create parent directories when extracting
            // a path, we're done here
            return;
        }
        
        int result = unzOpenCurrentFile(zip);
        if (result != UNZ_OK) {
            throw sg_io_exception("opening current zip file failed", sg_location(name));
        }
            
        std::ofstream outFile;
        bool eof = false;
        SGPath path(m_extractPath);
        path.append(name);
                        
    // create enclosing directory heirarchy as required
        Dir parentDir(path.dir());
        if (!parentDir.exists()) {
            bool ok = parentDir.create(0755);
            if (!ok) {
                throw sg_io_exception("failed to create directory heirarchy for extraction", path.c_str());
            }
        }
            
        outFile.open(path.c_str(), std::ios::binary | std::ios::trunc | std::ios::out);
        if (outFile.fail()) {
            throw sg_io_exception("failed to open output file for writing", path.c_str());
        }
            
        while (!eof) {
            int bytes = unzReadCurrentFile(zip, buffer, bufferSize);
            if (bytes < 0) {
                throw sg_io_exception("unzip failure reading curent archive", sg_location(name));
            } else if (bytes == 0) {
                eof = true;
            } else {
                outFile.write(buffer, bytes);
            }
        }
            
        outFile.close();
        unzCloseCurrentFile(zip);
    }
    
    bool extractUnzip()
    {
        bool result = true;
        zlib_filefunc_def memoryAccessFuncs;
        fill_memory_filefunc(&memoryAccessFuncs);
           
        char bufferName[128];
        snprintf(bufferName, 128, "%p+%lx", m_buffer.data(), m_buffer.size());
        unzFile zip = unzOpen2(bufferName, &memoryAccessFuncs);
        
        const size_t BUFFER_SIZE = 32 * 1024;
        void* buf = malloc(BUFFER_SIZE);
        
        try {
            int result = unzGoToFirstFile(zip);
            if (result != UNZ_OK) {
                throw sg_exception("failed to go to first file in archive");
            }
            
            while (true) {
                extractCurrentFile(zip, (char*) buf, BUFFER_SIZE);
                result = unzGoToNextFile(zip);
                if (result == UNZ_END_OF_LIST_OF_FILE) {
                    break;
                } else if (result != UNZ_OK) {
                    throw sg_io_exception("failed to go to next file in the archive");
                }
            }
        } catch (sg_exception& e) {
            result = false;
        }
        
        free(buf);
        unzClose(zip);
        return result;
    }
        
    void doFailure()
    {
        Dir dir(m_extractPath);
        dir.remove(true /* recursive */);
        if (m_urls.size() == 1) {
            
            return;
        }
        
        m_urls.erase(m_urls.begin()); // pop first URL
    }
    
    Install* m_owner;
    string_list m_urls;
    MD5_CTX m_md5;
    std::string m_buffer;
    SGPath m_extractPath;
};

////////////////////////////////////////////////////////////////////
    
Install::Install(Package* aPkg, const SGPath& aPath) :
    m_package(aPkg),
    m_path(aPath),
    m_download(NULL)
{
    parseRevision();
}

Install* Install::createFromPath(const SGPath& aPath, Catalog* aCat)
{
    std::string id = aPath.file();
    Package* pkg = aCat->getPackageById(id);
    if (!pkg)
        throw sg_exception("no package with id:" + id);
    
    return new Install(pkg, aPath);
}

void Install::parseRevision()
{
    SGPath revisionFile = m_path;
    revisionFile.append(".revision");
    if (!revisionFile.exists()) {
        m_revision = 0;
        return;
    }
    
    std::ifstream f(revisionFile.c_str(), std::ios::in);
    f >> m_revision;
}

void Install::writeRevisionFile()
{
    SGPath revisionFile = m_path;
    revisionFile.append(".revision");
    std::ofstream f(revisionFile.c_str(), std::ios::out | std::ios::trunc);
    f << m_revision << std::endl;
}

bool Install::hasUpdate() const
{
    return m_package->revision() > m_revision;
}

void Install::startUpdate()
{
    if (m_download) {
        return; // already active
    }
    
    m_download = new PackageArchiveDownloader(this);
    m_package->catalog()->root()->getHTTPClient()->makeRequest(m_download);
}

void Install::uninstall()
{
    Dir d(m_path);
    d.remove(true);
    delete this;
}

} // of namespace pkg

} // of namespace simgear
