// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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

#include "BVHPager.hxx"

#include <list>
#include <mutex>

#include <simgear/threads/SGThread.hxx>

#include "BVHPageNode.hxx"
#include "BVHPageRequest.hxx"

namespace simgear {

struct BVHPager::_PrivateData : protected SGThread {
    typedef SGSharedPtr<BVHPageRequest> _Request;
    typedef std::list<_Request> _RequestList;
    typedef std::list<SGSharedPtr<BVHPageNode> > _PageNodeList;
    
    struct _LockedQueue {
        void _push(const _Request& request)
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            _requestList.push_back(request);
        }
        _Request _pop()
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            if (_requestList.empty())
                return _Request();
            _Request request;
            request.swap(_requestList.front());
            _requestList.pop_front();
            return request;
        }
    private:
        std::mutex _mutex;
        _RequestList _requestList;
    };
    
    struct _WorkQueue {
        void _stop()
        {
            _push(_Request());
        }
        void _push(const _Request& request)
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            bool needSignal = _requestList.empty();
            _requestList.push_back(request);
            if (needSignal)
                _waitCondition.signal();
        }
        _Request _pop()
        {
            std::lock_guard<std::mutex> scopeLock(_mutex);
            while (_requestList.empty())
                _waitCondition.wait(_mutex);
            _Request request;
            request.swap(_requestList.front());
            _requestList.pop_front();
            return request;
        }
    private:
        std::mutex _mutex;
        SGWaitCondition _waitCondition;
        _RequestList _requestList;
    };

    _PrivateData() :
        _started(false),
        _useStamp(0)
    {
    }
    virtual ~_PrivateData()
    {
        _stop();
    }

    virtual void run()
    {
        for (;;) {
            _Request request = _pendingRequests._pop();
            // This means stop working
            if (!request.valid())
                return;
            request->load();
            _processedRequests._push(request);
        }
    }

    bool _start()
    {
        if (_started)
            return true;
        if (!start())
            return false;
        _started = true;
        return true;
    }
    
    void _stop()
    {
        if (!_started)
            return;
        // send a stop request ...
        _pendingRequests._stop();
        // ... and wait for the thread to finish
        join();
        _started = false;
    }

    void _use(BVHPageNode& pageNode)
    {
        if (pageNode._requested) {
            // move it forward in the lru list
            _pageNodeList.splice(_pageNodeList.end(), _pageNodeList,
                                 pageNode._iterator);
        } else {
            _Request request = pageNode.newRequest();
            if (!request.valid())
                return;

            pageNode._iterator = _pageNodeList.insert(_pageNodeList.end(),
                                                      &pageNode);
            pageNode._requested = true;

            if (_started) {
                _pendingRequests._push(request);
            } else {
                request->load();
                request->insert();
            }
        }
        pageNode._useStamp = _useStamp;
    }

    void _update(unsigned expiry)
    {
        // Insert all processed requests
        for (;;) {
            SGSharedPtr<BVHPageRequest> request;
            request = _processedRequests._pop();
            if (!request.valid())
                break;
            request->insert();
        }

        // ... and throw away stuff that is not used for a long time
        unsigned useStamp = _useStamp - expiry;
        _PageNodeList::iterator i = _pageNodeList.begin();
        while (i != _pageNodeList.end()) {
            // Ok, this means if the highest bit in the below difference
            // is set which is aequivalent to having a negative difference
            // but being wraparound save.
            unsigned diff = (*i)->_useStamp - useStamp;
            // test the sign bit of the difference
            if (!(diff & (~((~0u) >> 1))))
                break;
            (*i)->clear();
            (*i)->_requested = false;
            i = _pageNodeList.erase(i);
        }
    }

    bool _started;
    unsigned _useStamp;
    _WorkQueue _pendingRequests;
    _LockedQueue _processedRequests;
    // Store the rcu list of loaded nodes so that they can expire
    _PageNodeList _pageNodeList;
};

BVHPager::BVHPager() :
    _privateData(new _PrivateData)
{
}

BVHPager::~BVHPager()
{
    delete _privateData;
    _privateData = 0;
}

bool
BVHPager::start()
{
    return _privateData->_start();
}

void
BVHPager::stop()
{
    _privateData->_stop();
}

void
BVHPager::use(BVHPageNode& pageNode)
{
    _privateData->_use(pageNode);
}

void
BVHPager::update(unsigned expiry)
{
    _privateData->_update(expiry);
}

void
BVHPager::setUseStamp(unsigned stamp)
{
    _privateData->_useStamp = stamp;
}

unsigned
BVHPager::getUseStamp() const
{
    return _privateData->_useStamp;
}

}
