#ifndef SG_HOST_LOOKUP_HXX
#define SG_HOST_LOOKUP_HXX

#include <simgear/io/raw_socket.hxx>
#include <simgear/timing/timestamp.hxx>

#include <simgear/structure/SGReferenced.hxx>

namespace simgear
{

class HostLookup : public SGReferenced
{
public:
    static HostLookup* lookup(const std::string& h);
	
	bool resolved() const
	{ return _resolved; }
	
	bool failed() const
	{ return _failed; }
	
	const IPAddress& address() const
	{ return _address; }
	
	const std::string& host() const
        { return _host; }
private:
    HostLookup(const std::string& h);
    
	friend class Resolver;
	
	void resolve(const IPAddress& addr);
	void fail();
	
	std::string _host;
	bool _resolved;
	bool _failed;
	IPAddress _address;
	SGTimeStamp _age;
};

} // of namespace simgear

#endif // of SG_HOST_LOOKUP_HXX
