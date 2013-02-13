

#include <simgear/package/Install.hxx>

#include <boost/foreach.hpp>
#include <fstream>

// libarchive support
#include <archive.h>
#include <archive_entry.h>

#include <simgear/package/md5.h>

#include <simgear/structure/exception.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>

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
        std::cout << "got " << m_buffer.size() << " bytes" << std::endl;
    }
    
    virtual void responseComplete()
    {
        if (responseCode() != 200) {
            SG_LOG(SG_GENERAL, SG_ALERT, "download failure");
            doFailure();
            return;
        }
        std::cout << "content lenth:" << responseLength() << std::endl;
        std::cout << m_buffer.size() << " total received" << std::endl;
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
                
        struct archive* a = archive_read_new();
        archive_read_support_filter_all(a);
        archive_read_support_format_all(a);
        int result = archive_read_open_memory(a, (void*) m_buffer.data(), m_buffer.size());
        
        if (result != ARCHIVE_OK) {
            doFailure();
            return;
        }
        
        struct archive_entry* entry;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            SGPath finalPath(m_extractPath);
            finalPath.append(archive_entry_pathname(entry));
         //   std::cout << "writing:" << finalPath << std::endl;
            archive_entry_set_pathname(entry, finalPath.c_str());            
            archive_read_extract(a, entry, 0);
        }
        
        archive_read_free(a);
                  
        if (m_owner->path().exists()) {
            std::cout << "removing existing path" << std::endl;
            Dir destDir(m_owner->path());
            destDir.remove(true /* recursive */);
        }
        
        std::cout << "renaming to " << m_owner->path() << std::endl;
        m_extractPath.append(m_owner->package()->id());
        m_extractPath.rename(m_owner->path());
        
        m_owner->m_revision = m_owner->package()->revision();
        m_owner->writeRevisionFile();
    }
    
private:
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
