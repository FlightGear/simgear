#include <iostream>
#include <unistd.h>

#include <simgear/debug/logstream.hxx>

#include "sg_socket.hxx"

using std::cout;

class TcpClient
{
public:
    TcpClient( const char* host, const char* port );
    ~TcpClient();

    bool open();
    bool process();
    bool close();

private:
    SGIOChannel* channel;
};

TcpClient::TcpClient( const char* host, const char* port )
{
    channel = new SGSocket( host, port, "tcp" );
}

TcpClient::~TcpClient()
{
    delete channel;
}

bool
TcpClient::open()
{
    return channel->open( SG_IO_OUT );
}

bool
TcpClient::process()
{
    char wbuf[1024];

    sprintf( wbuf, "hello world\n" );
    int length = channel->writestring( wbuf );
    cout << "writestring returned " << length << "\n";

    return true;
}

bool
TcpClient::close()
{
    return channel->close();
}

int
main()
{
    sglog().setLogLevels( SG_ALL, SG_INFO );
    TcpClient client( "localhost", "5500" );
    if (!client.open())
    {
	cout << "client open failed\n";
	return 0;
    }

    for (int i = 0; i < 3; ++i)
    {
	client.process();
	sleep(1);
    }

    //client.close();
    return 0;
}
