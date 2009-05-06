#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <string>
#include <iostream>

#include "sg_socket.hxx"

using std::string;
using std::cout;

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();
    bool open();
    bool process();
    bool close();

private:
    SGIOChannel* channel;
};

TcpServer::TcpServer()
{
    channel = new SGSocket( "", "5500", "tcp" );
}

TcpServer::~TcpServer()
{
    delete channel;
}

bool
TcpServer::open()
{
    channel->open( SG_IO_BI );
    return true;
}

bool
TcpServer::process()
{
    char buf[1024];

    int len;
    while ((len = channel->readline( buf, sizeof(buf) )) > 0)
    {
	cout << len << ": " << buf;
    }
    return true;
}

bool
TcpServer::close()
{
    return channel->close();
}


int
main()
{
    sglog().setLogLevels( SG_ALL, SG_INFO );
    TcpServer server;
    server.open();
    SG_LOG( SG_IO, SG_INFO, "Created TCP server" );

    while (1)
    {
	server.process();
    }

    server.close();
    return 0;
}
