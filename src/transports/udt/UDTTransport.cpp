#include "UDTTransport.hpp"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <string.h>
#include <base/Logging.hpp>
#include <boost/regex.hpp>

using namespace UDT;

namespace fipa {
namespace services {
namespace udt {

extern const uint32_t MAX_MESSAGE_SIZE_BYTES = 20*1024*1024;

Address::Address(const std::string& ip, uint16_t port)
    : ip(ip)
    , port(port)
{}

std::string Address::toString() const
{
    char buffer[128];
    snprintf(buffer, 128, "udt://%s:%d",ip.c_str(),port);
    return std::string(buffer);
}

Address Address::fromString(const std::string& addressString)
{
    boost::regex r("udt://([^:]*):([0-9]{1,5})");
    boost::smatch what;
    if(boost::regex_match( addressString ,what,r))
    {
        std::string address(what[1].first, what[1].second);
        uint16_t port = atoi( std::string(what[2].first, what[2].second).c_str() );
        return Address(address, port);
    } else {
        throw ArgumentError("address '" + addressString + "' malformatted");
    }
}

bool Address::operator==(const Address& other) const
{
    return this->ip == other.ip && this->port == other.port;
}

Connection::Connection()
    : mPort(0)
    , mIP("0.0.0.0")
{}

Connection::Connection(const std::string& ip, uint16_t port)
    : mPort(port)
    , mIP(ip)
{}

OutgoingConnection::OutgoingConnection()
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, uint16_t port)
    : Connection(ipaddress, port)
{
    connect(ipaddress, port);
}

OutgoingConnection::OutgoingConnection(const Address& address)
{
    connect(address);
}

OutgoingConnection::~OutgoingConnection()
{
    UDT::close(mSocket);
}


void OutgoingConnection::connect(const std::string& ipaddress, uint16_t port)
{
    mSocket = UDT::socket(AF_INET, SOCK_DGRAM, 0);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    char portTxt[6];
    snprintf(portTxt,6,"%hu",port);

    struct addrinfo* addresses;
    if(0 != getaddrinfo(ipaddress.c_str(), portTxt, &hints, &addresses) )
    {
        throw std::runtime_error("fipa_service::udt::OutgoingConnection: invalid ip address / unresolvable host '" + ipaddress + ":" + std::string(portTxt) + "'");
    }

    struct addrinfo* currentAddress;
    for(currentAddress = addresses; currentAddress != NULL; currentAddress = currentAddress->ai_next)
    {
        struct sockaddr_in *addr;
        addr = (struct sockaddr_in *) currentAddress->ai_addr;
        memset(&(addr->sin_zero), '\0', 8);

        if( UDT::ERROR == UDT::connect(mSocket, (sockaddr*) addr, sizeof(struct sockaddr)))
        {
            LOG_WARN("Connection to %s:%hu could not be established",inet_ntoa((struct in_addr)addr->sin_addr), ntohs(addr->sin_port));
        } else {
            LOG_INFO("Connection to %s:%hu established",inet_ntoa((struct in_addr)addr->sin_addr), ntohs(addr->sin_port));
            return;
        }
    }

    throw std::runtime_error("fipa_service::udt::OutgoingConnection: connection failed");
}

void OutgoingConnection::sendData(const std::string& data, int ttl, bool inorder) const
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


void OutgoingConnection::sendLetter(const fipa::acl::Letter& letter, int ttl, bool inorder) const
{
    using namespace fipa::acl;
    std::string encodedData = EnvelopeGenerator::create(letter, representation::BITEFFICIENT);
    sendData(encodedData, ttl, inorder);
}

IncomingConnection::IncomingConnection()
{}

IncomingConnection::~IncomingConnection()
{
    UDT::close(mSocket);
}

IncomingConnection::IncomingConnection(const UDTSOCKET& socket, const std::string& ip, uint16_t port)
    : Connection(ip, port)
    , mSocket(socket)
{
}

int IncomingConnection::receiveMessage(char* buffer, size_t size) const
{
    int result = UDT::recvmsg(mSocket, buffer, size);
    if(result == UDT::ERROR)
    {
        switch(UDT::getlasterror().getErrorCode())
        {
            case 6002: // no data is available to be received on a non-blocking socket
                break;
            case 2001: // connection broken before send is completed
            case 2002: // not connected
            case 5004: // invalid UDT socket
            case 5009: // wrong mode
            case 6003: // no buffer available for the non-blocing rcv call
            case 6004: // an overlapped recv is in progress
            default:
                throw std::runtime_error("fipa::services::udt::IncomingConnection: " + std::string(UDT::getlasterror().getErrorMessage()));
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

void Node::listen(uint16_t port, uint32_t maxClients)
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

    if( UDT::ERROR == UDT::listen(mServerSocket, maxClients))
    {
        throw std::runtime_error("fipa_service::udt::Node: could not start listening. " +
                std::string(UDT::getlasterror().getErrorMessage()));
    } else {
        sockaddr_in sock_addr;
        int len = sizeof(sock_addr);
        if(UDT::ERROR == UDT::getsockname(mServerSocket, (sockaddr*) &sock_addr, &len) )
        {
            throw std::runtime_error("fipa_service::udt::Node: retrieving socket information failed. " +
                std::string(UDT::getlasterror().getErrorMessage()));
        }

        char hostname[HOST_NAME_MAX];
        if(0 == gethostname(hostname,HOST_NAME_MAX))
        {
            mIP = std::string(hostname);
        } else {
            throw std::runtime_error("fipa_service::udt::Node: could not get hostname");
        }
        mPort = ntohs(sock_addr.sin_port);
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

            uint16_t port = ntohs(other_addr.sin_port);
            char buffer[20];
            if( NULL != inet_ntop(AF_INET, &other_addr.sin_addr, buffer, 20))
            {
                std::string ip = std::string(buffer);
                mClients.push_back( IncomingConnectionPtr(new IncomingConnection(client, ip, port)) );
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
    using namespace fipa::acl;

    IncomingConnections cleanupList;

    while(true)
    {
        IncomingConnections::iterator it = mClients.begin();
        bool messageFound = false;
        for(;it != mClients.end(); ++it)
        {
            IncomingConnectionPtr clientConnection  = *it;
            try {
                int size = 0;
                if( (size = clientConnection->receiveMessage(mpBuffer, mBufferSize)) > 0)
                {
                    Letter letter;
                    std::string data(mpBuffer, size);
                    if(!mEnvelopeParser.parseData(data, letter, representation::BITEFFICIENT))
                    {
                        LOG_WARN("Failed to parse letter");
                    } else {
                        mLetterQueue.push(letter);
                        messageFound = true;
                    }
                }
            } catch(const std::runtime_error& e)
            {
                LOG_WARN("Receiving data failed: %s", e.what());
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

fipa::acl::Letter Node::nextLetter()
{
    fipa::acl::Letter letter = mLetterQueue.front();
    mLetterQueue.pop();
    return letter;
}

} // end namespace udt
} // end namespace services
} // end namespace fipa
