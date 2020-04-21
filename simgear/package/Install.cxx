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

#include <simgear_config.h>
#include <simgear/package/Install.hxx>

#include <cstdlib>
#include <cstring>
#include <fstream>

#include <simgear/package/unzip.h>
#include <simgear/package/md5.h>

#include <simgear/io/untar.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

namespace simgear {

namespace pkg {

class Install::PackageArchiveDownloader : public HTTP::Request
{
public:
    PackageArchiveDownloader(InstallRef aOwner, const string_list& urls) :
        HTTP::Request("" /* dummy URL */),
        m_owner(aOwner)
    {
        m_urls = urls;
        if (urls.empty()) {
            m_urls = m_owner->package()->downloadUrls();
        }

        selectMirrorUrl();
        
        m_extractPath = aOwner->path().dir();
        m_extractPath.append("_extract_" + aOwner->package()->md5());

        // clean up any existing files (eg from previous failed download)
        Dir d(m_extractPath);
        if (d.exists()) {
            d.remove(true /* recursive */);
        }
    }

    ~PackageArchiveDownloader()
    {
        // always clean up our extraction dir: if we successfully downloaded
        // and installed it will be an empty dir, if we failed it might contain
        // (some) of the package files.
        Dir d(m_extractPath);
        if (d.exists()) {
            d.remove(true /* recursive */);
        }
    }

    size_t downloadedBytes() const
    {
        return m_downloaded;
    }

    int percentDownloaded() const
    {
        if (responseLength() <= 0) {
            return 0;
        }

        return (m_downloaded * 100) / responseLength();
    }
protected:
    void selectMirrorUrl()
    {
        const int randomizedIndex = rand() % m_urls.size();
        m_activeURL = m_urls.at(randomizedIndex);
        m_urls.erase(m_urls.begin() + randomizedIndex);
    }
    
    virtual std::string url() const override
    {
        return m_activeURL;
    }

    void responseHeadersComplete() override
    {
        Request::responseHeadersComplete();

        Dir d(m_extractPath);
        if (!d.create(0755)) {
            SG_LOG(SG_GENERAL, SG_WARN, "Failed to create extraction directory" << d.path());
        }

		m_extractor.reset(new ArchiveExtractor(m_extractPath));
        memset(&m_md5, 0, sizeof(SG_MD5_CTX));
        SG_MD5Init(&m_md5);
        
        m_owner->startDownload();
    }

    void gotBodyData(const char* s, int n) override
    {
        // if there's a pre-existing error, discard byte sinstead of pushing
        // more through the extactor
        if (m_extractor->hasError()) {
            return;
        }
        
		const uint8_t* ubytes = (uint8_t*) s;
        SG_MD5Update(&m_md5, ubytes, n);
        m_downloaded += n;
        m_owner->installProgress(m_downloaded, responseLength());
		m_extractor->extractBytes(ubytes, n);
        if (m_extractor->hasError()) {
            SG_LOG(SG_GENERAL, SG_WARN, "archive extraction failed (from " + m_activeURL + ")");
        }
    }

    void onDone() override
    {
        if (responseCode() != 200) {
            SG_LOG(SG_GENERAL, SG_WARN, "download failure:" << responseCode() <<
                   "\n\t" << url());
            Delegate::StatusCode code = Delegate::FAIL_DOWNLOAD;
            if (responseCode() == 404) {
                code = Delegate::FAIL_NOT_FOUND;
            }

            doFailure(code);
            return;
        }

        unsigned char digest[MD5_DIGEST_LENGTH];
        SG_MD5Final(digest, &m_md5);
        std::string const hex_md5 =
          strutils::encodeHex(digest, MD5_DIGEST_LENGTH);

        if (hex_md5 != m_owner->package()->md5()) {
            SG_LOG(SG_GENERAL, SG_WARN, "md5 verification failed:\n"
                << "\t" << hex_md5 << "\n\t"
                << m_owner->package()->md5() << "\n\t"
                << "downloading from:" << url());
            doFailure(Delegate::FAIL_CHECKSUM);
            return;
        }

		m_extractor->flush();
        if (m_extractor->hasError() || !m_extractor->isAtEndOfArchive()) {
            SG_LOG(SG_GENERAL, SG_WARN, "archive extraction failed");
            doFailure(Delegate::FAIL_EXTRACT);
            return;
        }

		// disable caching on the owner's path, otherwise the upcoming
		// delete & rename confuse everything
		m_owner->m_path.set_cached(false);
		m_extractPath.set_cached(false);

        if (m_owner->path().exists()) {
            Dir destDir(m_owner->path());
            destDir.remove(true /* recursive */);
        }

        // build a path like /path/to/packages/org.some.catalog/Aircraft/extract_xxxx/MyAircraftDir
        SGPath extractedPath = m_extractPath;
        if (m_owner->package()->properties()->hasChild("archive-path")) {
            extractedPath.append(m_owner->package()->properties()->getStringValue("archive-path"));
        } else {
            extractedPath.append(m_owner->package()->dirName());
        }


        // rename it to path/to/packages/org.some.catalog/Aircraft/MyAircraftDir
        bool ok = extractedPath.rename(m_owner->path());
        if (!ok) {
            doFailure(Delegate::FAIL_FILESYSTEM);
            return;
        }

        // extract_xxxx directory is now empty, so remove it
        // (note it might not be empty if the archive contained some other
        // files, but we delete those in such a case
        if (m_extractPath.exists()) {
            simgear::Dir(m_extractPath).remove();
        }

        m_owner->m_revision = m_owner->package()->revision();
        m_owner->writeRevisionFile();
        m_owner->m_download.reset(); // so isDownloading reports false

        m_owner->installResult(Delegate::STATUS_SUCCESS);
    }

    void onFail() override
    {
        if (responseCode() == -1) {
            doFailure(Delegate::USER_CANCELLED);
        } else {
            doFailure(Delegate::FAIL_DOWNLOAD);
        }
    }

private:
    void doFailure(Delegate::StatusCode aReason)
    {
        Dir dir(m_extractPath);
        if (dir.exists()) {
            dir.remove(true /* recursive */);
        }
        
        const auto canRetry = (aReason == Delegate::FAIL_NOT_FOUND) ||
            (aReason == Delegate::FAIL_DOWNLOAD) || (aReason == Delegate::FAIL_CHECKSUM);
        
        if (canRetry && !m_urls.empty()) {
            SG_LOG(SG_GENERAL, SG_WARN, "archive download failed from:" << m_activeURL
                   << "\n\twill retry with next mirror");
            // becuase selectMirrorUrl erased the active URL from m_urls,
            // this new request will select one of the other mirrors
            auto retryDownload = new PackageArchiveDownloader(m_owner, m_urls);
            m_owner->m_download.reset(retryDownload);
            m_owner->package()->catalog()->root()->makeHTTPRequest(retryDownload);
            return;
        }

        m_owner->m_download.reset(); // ensure we get cleaned up
        m_owner->installResult(aReason);
    }

    
    InstallRef m_owner;
    string m_activeURL;
    string_list m_urls;
    SG_MD5_CTX m_md5;
    SGPath m_extractPath;
    size_t m_downloaded = 0;
	std::unique_ptr<ArchiveExtractor> m_extractor;
};

////////////////////////////////////////////////////////////////////
Install::Install(PackageRef aPkg, const SGPath& aPath) :
    m_package(aPkg),
    m_path(aPath),
    m_status(Delegate::STATUS_IN_PROGRESS)
{
    parseRevision();
    m_package->catalog()->root()->registerInstall(this);
}

Install::~Install()
{
}

InstallRef Install::createFromPath(const SGPath& aPath, CatalogRef aCat)
{
    std::string path = aPath.file();
    PackageRef pkg = aCat->getPackageByPath(path);
    if (!pkg)
        throw sg_exception("no package with path:" + path);

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

    sg_ifstream f(revisionFile, std::ios::in);
    f >> m_revision;
}

void Install::writeRevisionFile()
{
    SGPath revisionFile = m_path;
    revisionFile.append(".revision");
    sg_ofstream f(revisionFile, std::ios::out | std::ios::trunc);
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

    m_download = new PackageArchiveDownloader(this, {});
    m_package->catalog()->root()->makeHTTPRequest(m_download);
    m_package->catalog()->root()->startInstall(this);
}

bool Install::uninstall()
{
    Dir d(m_path);
    if (!d.remove(true)) {
        SG_LOG(SG_GENERAL, SG_ALERT, "package uninstall failed: couldn't remove path " << m_path);
        return false;
    }

    m_package->catalog()->root()->unregisterInstall(this);
    return true;
}

bool Install::isDownloading() const
{
    return (m_download.valid());
}

bool Install::isQueued() const
{
    return m_package->catalog()->root()->isInstallQueued(const_cast<Install*>(this));
}

int Install::downloadedPercent() const
{
    if (!m_download.valid()) {
        return -1;
    }

    PackageArchiveDownloader* dl = static_cast<PackageArchiveDownloader*>(m_download.get());
    return dl->percentDownloaded();
}

size_t Install::downloadedBytes() const
{
    if (!m_download.valid()) {
        return -1;
    }

    PackageArchiveDownloader* dl = static_cast<PackageArchiveDownloader*>(m_download.get());
    return dl->downloadedBytes();

}

void Install::cancelDownload()
{
    if (m_download.valid()) {
        m_package->catalog()->root()->cancelHTTPRequest(m_download, "User cancelled download");
    }

    if (m_revision == 0) {
        SG_LOG(SG_GENERAL, SG_INFO, "cancel install of package, will unregister");
        m_package->catalog()->root()->unregisterInstall(this);
    }

    m_package->catalog()->root()->cancelDownload(this);
    m_download.clear();
}

SGPath Install::primarySetPath() const
{
    SGPath setPath(m_path);
    std::string ps(m_package->id());
    setPath.append(ps + "-set.xml");
    return setPath;
}

//------------------------------------------------------------------------------
Install* Install::done(const Callback& cb)
{
  if( m_status == Delegate::STATUS_SUCCESS )
    cb(this);
  else
    _cb_done.push_back(cb);

  return this;
}

//------------------------------------------------------------------------------
Install* Install::fail(const Callback& cb)
{
  if(    m_status != Delegate::STATUS_SUCCESS
      && m_status != Delegate::STATUS_IN_PROGRESS )
    cb(this);
  else
    _cb_fail.push_back(cb);

  return this;
}

//------------------------------------------------------------------------------
Install* Install::always(const Callback& cb)
{
  if( m_status != Delegate::STATUS_IN_PROGRESS )
    cb(this);
  else
    _cb_always.push_back(cb);

  return this;
}

//------------------------------------------------------------------------------
Install* Install::progress(const ProgressCallback& cb)
{
  _cb_progress.push_back(cb);
  return this;
}

//------------------------------------------------------------------------------
void Install::installResult(Delegate::StatusCode aReason)
{
    m_status = aReason;
    m_package->catalog()->root()->finishInstall(this, aReason);
    if (aReason == Delegate::STATUS_SUCCESS) {
        _cb_done(this);
    } else {
        _cb_fail(this);
    }

    _cb_always(this);
}

//------------------------------------------------------------------------------
void Install::installProgress(unsigned int aBytes, unsigned int aTotal)
{
  m_package->catalog()->root()->installProgress(this, aBytes, aTotal);
  _cb_progress(this, aBytes, aTotal);
}

void Install::startDownload()
{
    m_status = Delegate::STATUS_IN_PROGRESS;
}

Delegate::StatusCode Install::status() const
{
    return m_status;
}

} // of namespace pkg

} // of namespace simgear
