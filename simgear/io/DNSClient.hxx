// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2016 Torsten Dreyer <torsten@t3r.de>

/**
 * @file
 * @brief Simple DNS resolver client engine for SimGear
 */

#ifndef SG_DNS_CLIENT_HXX
#define SG_DNS_CLIENT_HXX

#include <memory> // for std::unique_ptr
#include <string>
#include <vector>
#include <ctime> // for time_t

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/event_mgr.hxx>

namespace simgear
{

namespace DNS
{

class Client;

using UDNSQueryPtr = void*;

class Request : public SGReferenced
{
public:
    Request( const std::string & dn );
    virtual ~Request();
    const std::string& getDn() const { return _dn; }
    int getType() const { return _type; }
    bool isComplete() const { return _complete; }
    bool isTimeout() const;
    void setComplete( bool b = true ) { _complete = b; }
    bool isCancelled() const { return _cancelled; }

    virtual void submit( Client * client) = 0;

    void cancel();

    std::string cname;
    std::string qname;
    unsigned ttl;
protected:
    friend class Client;

    UDNSQueryPtr _query = nullptr;
    std::string _dn;
    int _type;
    bool _complete;
    time_t _timeout_secs;
    time_t _start;
    bool _cancelled = false;
};
typedef SGSharedPtr<Request> Request_ptr;

class NAPTRRequest : public Request
{
public:
    NAPTRRequest( const std::string & dn );
    void submit(Client* client) override;

    struct NAPTR : SGReferenced {
        int order;
        int preference;
        std::string flags;
        std::string service;
        std::string regexp;
        std::string replacement;
    };
    typedef SGSharedPtr<NAPTR> NAPTR_ptr;
    typedef std::vector<NAPTR_ptr> NAPTR_list;
    NAPTR_list entries;

    std::string qflags;
    std::string qservice;
};

class SRVRequest : public Request
{
public:
    SRVRequest( const std::string & dn );
    SRVRequest(const std::string& dn, const std::string& service, const std::string& protocol);
    void submit(Client* client) override;

    struct SRV : SGReferenced {
      int priority;
      int weight;
      int port;
      std::string target;
    };
    typedef SGSharedPtr<SRV> SRV_ptr;
    typedef std::vector<SRV_ptr> SRV_list;
    SRV_list entries;
private:
    std::string _service;
    std::string _protocol;
};

class TXTRequest : public Request
{
public:
    TXTRequest( const std::string & dn );
    void submit(Client* client) override;

    typedef std::vector<std::string> TXT_list;
    typedef std::map<std::string,std::string> TXT_Attribute_map;
    TXT_list entries;
    TXT_Attribute_map attributes;
};

class Client
{
public:
    Client(const std::string& nameserver = {});
    virtual ~Client();

    void update(int waitTimeout = 0);

    void makeRequest(const Request_ptr& r);

//    void cancelRequest(const Request_ptr& r, std::string reason = std::string());

    class ClientPrivate;
    std::unique_ptr<ClientPrivate> d;
};

} // of namespace DNS

} // of namespace simgear

#endif // of SG_DNS_CLIENT_HXX
