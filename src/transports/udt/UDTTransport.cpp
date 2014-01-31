#include "UDTTransport.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <string.h>

using namespace UDT;

namespace fipa {
namespace services {
namespace udt {

extern const uint32_t MAX_MESSAGE_SIZE_BYTES = 20*1024*1024;

Connection::Connection()
    : mPort(-1)
    , mIP("0.0.0.0")
{}

Connection::Connection(const std::string& ip, int16_t port)
    : mPort(port)
    , mIP(ip)
{}

OutgoingConnection::OutgoingConnection()
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, int16_t port)
    : Connection(ipaddress, port)
{
    connect(ipaddress, port);
}

OutgoingConnection::~OutgoingConnection()
{
    UDT::close(mSocket);
}


void OutgoingConnection::connect(const std::string& ipaddress, int16_t port)
{
    mSocket = UDT::socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(ipaddress == "0.0.0.0")
    {
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    } else if(1 != inet_pton(AF_INET, ipaddress.c_str(), &serv_addr.sin_addr))
    {
        throw std::runtime_error("fipa_service::udt::OutgoingConnection: invalid ip address'" + ipaddress + "'");
    }
    memset(&(serv_addr.sin_zero), '\0', 8);

    if( UDT::ERROR == UDT::connect(mSocket, (sockaddr*) & serv_addr, sizeof(serv_addr)))
    {
        throw std::runtime_error("fipa_service::udt::OutgoingConnection: " + std::string( UDT::getlasterror().getErrorMessage()) );
    } else {
        LOG_WARN("Connection to %s:%d established", ipaddress.c_str(), port);
    }
}

void OutgoingConnection::sendMessage(const std::string& data, int ttl, bool inorder)
{
    int result = UDT::sendmsg(mSocket, data.data(), data.size(), ttl, inorder);
    if( UDT::ERROR == result)
    {
        throw std::runtime_error("fipa_service::udt::OutgoingConnection: " + std::string(UDT::getlasterror().getErrorMessage()));
    } else if( 0 == result)
    {
        throw std::runtime_error("fipa_service::udt::OutgoingConnection: sendMessage ran into timeout");
    }
}

IncomingConnection::IncomingConnection()
{}

IncomingConnection::~IncomingConnection()
{
    UDT::close(mSocket);
}

IncomingConnection::IncomingConnection(const UDTSOCKET& socket, const std::string& ip, int16_t port)
    : Connection(ip, port)
    , mSocket(socket)
{
}

int IncomingConnection::receiveMessage(char* buffer, size_t size)
{
    int result = UDT::recvmsg(mSocket, buffer, size);
    if(result == UDT::ERROR)
    {
        switch(UDT::getlasterror().getErrorCode())
        {
            case 2001: // connection broken before send is completed
            case 2002: // not connected
            case 5004: // invalid UDT socket
            case 5009: // wrong mode
                throw std::runtime_error("fipa::services::udt::IncomingConnection: " + std::string(UDT::getlasterror().getErrorMessage()));
            default:
                break;
        }
    }

    return result;
}

Node::Node()
    : mBufferSize(10000000)
{
    mServerSocket = UDT::socket(AF_INET, SOCK_DGRAM, 0);
    bool block = false;
    UDT::setsockopt(mServerSocket, 0 /*ignored*/, UDT_RCVSYN,&block,sizeof(bool));

    mpBuffer = new char[mBufferSize];
    if(mpBuffer == NULL)
    {
        throw std::runtime_error("fipa_service::udt::Node: could not allocate memory for buffer");
    }
}

Node::~Node()
{
    delete mpBuffer;
}

void Node::listen(int32_t port, uint32_t maxClients)
{
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), '\0', 8);

    if( UDT::ERROR == UDT::bind(mServerSocket, (sockaddr*) &serv_addr, sizeof(serv_addr)))
    {
        char buffer[128];
        snprintf(buffer, 128,"fipa_service::udt::Node: could not bind to port %d", port);
        throw std::runtime_error( buffer );
    }

    mPort = ntohs(serv_addr.sin_port);
    char buffer[20];
    if( NULL != inet_ntop(AF_INET, &serv_addr.sin_addr, buffer, 20))
    {
        mIP = std::string(buffer);
    }

    if( UDT::ERROR == UDT::listen(mServerSocket, maxClients))
    {
        throw std::runtime_error("fipa_service::udt::Node: could not start listening. " +
                std::string(UDT::getlasterror().getErrorMessage()));
    } else {
        LOG_DEBUG("fipa::udt::Node: started listening");

    }
}

bool Node::accept()
{
    int namelen;
    sockaddr_in other_addr;
    bool success = false;
    while(true)
    {
        UDTSOCKET client = UDT::accept(mServerSocket, (sockaddr*) &other_addr, &namelen);
        if(client != UDT::INVALID_SOCK)
        {

            int16_t port = ntohs(other_addr.sin_port);
            char buffer[20];
            if( NULL != inet_ntop(AF_INET, &other_addr.sin_addr, buffer, 20))
            {
                std::string ip = std::string(buffer);
                mClients.push_back( IncomingConnection(client, ip, port) );
                success = true;
            }
        } else {
            return success;
        }
    }
    return success;
}

void Node::update(bool readAll)
{
    IncomingConnections cleanupList;

    while(true)
    {
        IncomingConnections::iterator it = mClients.begin();
        bool messageFound = false;
        for(;it != mClients.end(); ++it)
        {
            IncomingConnection clientConnection  = *it;
            try {
                if( clientConnection.receiveMessage(mpBuffer, mBufferSize) > 0)
                {
                    messageFound = true;
                    // cycles through all clients once and update
                    mMessageQueue.push( std::string(mpBuffer) );
                }
            } catch(const std::runtime_error& e)
            {
                cleanupList.push_back(clientConnection);
            }
        }

        if(! (messageFound && readAll) )
        {
            break;
        }
    }

    // Cleanup invalid sockets
    IncomingConnections::const_iterator cit = cleanupList.begin();
    for(; cit != cleanupList.end(); ++cit)
    {
        IncomingConnections::iterator it = std::find(mClients.begin(), mClients.end(), *cit);
        if(it != mClients.end())
        {
            mClients.erase(it);
            break;
        }
    }
}

} // end namespace udt
} // end namespace services
} // end namespace fipa
