#include "OutgoingConnection.hpp"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

namespace fipa {
namespace services {
namespace transports {
namespace udt {

OutgoingConnection::OutgoingConnection()
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, uint16_t port)
    : fipa::services::transports::OutgoingConnection(ipaddress, port)
{
    connect(ipaddress, port);
}

OutgoingConnection::OutgoingConnection(const Address& address)
    : fipa::services::transports::OutgoingConnection(address)

{
    connect(address.ip, address.port);
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

void OutgoingConnection::send(const std::string& data)
{
    sendData(data, getTTL(), true);
}

} // end namespace udt
} // end namespace transport
} // end namespace services
} // end namespace fipa
