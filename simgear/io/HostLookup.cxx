#include "HostLookup.hxx"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <map>

#include <simgear/threads/SGThread.hxx>

using std::string;

namespace simgear
{

class Resolver : public SGThread
{
public:
    Resolver()
    {
        // take lock when resolver starts,
        // won't release until it's actually running, and waiting on the
        // condition. Otherwise we get a race when starting.
        _lock.lock();
    }
    
	virtual void run()
	{        
		while (true) {
			_cond.wait(_lock); // wait until there's something to lookup
		
		// find the first un-resolved entry in the cache
			HostCache::iterator it;
			for (it = _cache.begin(); it != _cache.end(); ++it) {
				if (!it->second->resolved()) {
					break;
				}
			}
			
			if (it == _cache.end()) {
                continue;
			}
			
			string host = it->first;
        // don't hold any locks while looking up
			_lock.unlock();
		
		// start the actual lookup - may take a long period of time!
		// (eg, one, five or sixty seconds)
            struct addrinfo hints;
            bzero(&hints, sizeof(struct addrinfo));
            hints.ai_family = PF_UNSPEC;
            
            struct addrinfo *results;
            
			int result = getaddrinfo(host.c_str(), NULL, &hints, &results);
			_lock.lock(); // take the lock back before going any further
			            
			if (result == 0) {
			    _cache[host]->resolve(IPAddress(results->ai_addr, results->ai_addrlen));
                freeaddrinfo(results);                
			} else if (result == EAGAIN) {
			    
			} else {
                const char* errMsg = gai_strerror(result);
				// bad lookup, remove from cache? or set as failed?
                _cache[host]->fail();
			}
		} // of thread loop
		
		_lock.unlock();
	}
	
	void addLookup(HostLookup* l)
	{
		_lock.lock();
		_cache[l->host()] = l;
		_cond.signal();
		_lock.unlock();
	}
	
	HostLookup* lookup(const string& host)
	{
		_lock.lock();
		HostCache::iterator it = _cache.find(host);
		HostLookup* result = it->second;
		_lock.unlock();
		return result;
	}
private:
	SGMutex _lock;
	SGPthreadCond _cond;
	
	typedef std::map<string, HostLookup*> HostCache;
	HostCache _cache;
};

Resolver* static_resolve = NULL;

HostLookup* HostLookup::lookup(const string& host)
{
    if (!static_resolve) {
        static_resolve = new Resolver;
        static_resolve->start();
    }
    
	HostLookup* l = static_resolve->lookup(host);
	if (l) {
		return l;
	}
	
	l = new HostLookup(host);
    SGReferenced::get(l);
	static_resolve->addLookup(l);
	return l;
}

HostLookup::HostLookup(const std::string& h) :
	_host(h),
	_resolved(false)
{
}

void HostLookup::resolve(const IPAddress& addr)
{    
    _failed = false;
    _address = addr;
    _age.stamp();
    _resolved = true;
}

void HostLookup::fail()
{
    _failed = true;
    _resolved = false;
    _age.stamp();
}

} // of namespace
