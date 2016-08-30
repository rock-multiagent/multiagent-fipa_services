#include "UDTTransport.hpp"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <string.h>
#include <base-logging/Logging.hpp>
#include <boost/regex.hpp>

using namespace UDT;

namespace fipa {
namespace services {
namespace transports {
namespace udt {

extern const uint32_t MAX_MESSAGE_SIZE_BYTES = 20*1024*1024;

uint32_t UDTTransport::msRefCount = 0;


UDTTransport::UDTTransport()
    : Transport(UDT)
    , mBufferSize(10000000)
{
    ++msRefCount;
    if(UDT::ERROR == UDT::startup())
    {
        throw std::runtime_error("fipa_services::udt::UDTTransport: startup failed " +
                std::string(UDT::getlasterror().getErrorMessage()));
    }

    mServerSocket = UDT::socket(AF_INET, SOCK_DGRAM, 0);
    bool block = false;
    UDT::setsockopt(mServerSocket, 0 /*ignored*/, UDT_RCVSYN,&block,sizeof(bool));
    bool reuse = true;
    UDT::setsockopt(mServerSocket, 0 /*ignored*/, UDT_REUSEADDR,&reuse, sizeof(bool));

    mpBuffer = new char[mBufferSize];
    if(mpBuffer == NULL)
    {
        throw std::runtime_error("fipa_services::udt::UDTTransport: could not allocate memory for buffer");
    }
}

UDTTransport::~UDTTransport()
{
    if(--msRefCount == 0)
    {
        UDT::cleanup();
    }
    delete mpBuffer;
}

void UDTTransport::listen(uint16_t port, uint32_t maxClients)
{
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), '\0', 8);

    if( UDT::ERROR == UDT::bind(mServerSocket, (sockaddr*) &serv_addr, sizeof(serv_addr)))
    {
        char buffer[128];
        snprintf(buffer, 128,"fipa_services::udt::UDTTransport: could not bind to port %d", port);
        throw std::runtime_error( buffer );
    }

    if( UDT::ERROR == UDT::listen(mServerSocket, maxClients))
    {
        throw std::runtime_error("fipa_services::udt::UDTTransport: could not start listening. " +
                std::string(UDT::getlasterror().getErrorMessage()));
    } else {
        sockaddr_in sock_addr;
        int len = sizeof(sock_addr);
        if(UDT::ERROR == UDT::getsockname(mServerSocket, (sockaddr*) &sock_addr, &len) )
        {
            throw std::runtime_error("fipa_services::udt::UDTTransport: retrieving socket information failed. " +
                std::string(UDT::getlasterror().getErrorMessage()));
        }

        char hostname[HOST_NAME_MAX];
        if(0 == gethostname(hostname,HOST_NAME_MAX))
        {
            mAddress.ip = std::string(hostname);
        } else {
            throw std::runtime_error("fipa_services::udt::UDTTransport: could not get hostname");
        }
        mAddress.port = ntohs(sock_addr.sin_port);
    }
}

bool UDTTransport::accept()
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
                mClients.push_back( IncomingConnection::Ptr(new IncomingConnection(client, ip, port)) );
                success = true;
            }
        } else {
            return success;
        }
    }
    return success;
}

void UDTTransport::start(uint16_t port, uint32_t maxClients)
{
    listen(port, maxClients);
}

void UDTTransport::update(bool readAllMessages)
{
    using namespace fipa::acl;

    IncomingConnections cleanupList;

    while(true)
    {
        // Accept new connections
        accept();

        IncomingConnections::iterator it = mClients.begin();
        bool messageFound = false;
        for(;it != mClients.end(); ++it)
        {
            IncomingConnection::Ptr clientConnection  = *it;
            try {
                int size = 0;
                if( (size = clientConnection->receiveMessage(mpBuffer, mBufferSize)) > 0)
                {
                    std::string data(mpBuffer, size);
                    notify(data);
                }
            } catch(const std::runtime_error& e)
            {
                LOG_WARN("Receiving data failed: %s", e.what());
                cleanupList.push_back(clientConnection);
            }
        }

        if(!messageFound || !readAllMessages)
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
        }
    }
}

Address UDTTransport::getAddress(const std::string& interfaceName) const
{
    return Address(Transport::getLocalIPv4Address(interfaceName), mAddress.port);
}

/**
 * Create a udt based outgoing connection
 */
OutgoingConnection::Ptr UDTTransport::establishOutgoingConnection(const Address& address)
{
    return transports::OutgoingConnection::Ptr(new udt::OutgoingConnection(address));
}

} // end namespace udt
} // end namespace transports
} // end namespace services
} // end namespace fipa
