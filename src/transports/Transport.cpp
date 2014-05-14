#include "Transport.hpp"

#include <stdexcept>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace fipa {
namespace services {

// Address
Address::Address(const std::string& ip, uint16_t port, const std::string& protocol)
    : ip(ip)
    , port(port)
    , protocol(protocol)
{}

std::string Address::toString() const
{
    return protocol + "://" + ip + ":" + boost::lexical_cast<std::string>(port);
}

Address Address::fromString(const std::string& addressString)
{
    boost::regex r("([^:]*)://([^:]*):([0-9]{1,5})"); 
    boost::smatch what;
    if(boost::regex_match(addressString, what, r))
    {
        std::string protocol(what[1].first, what[1].second);
        std::string address(what[2].first, what[2].second);
        uint16_t port = atoi( std::string(what[3].first, what[3].second).c_str() );
        return Address(address, port, protocol);
    } else {
        throw std::runtime_error("address '" + addressString + "' malformatted");
    }
}

bool Address::operator==(const Address& other) const
{
    return this->ip == other.ip && this->port == other.port;
}

// Connection
Connection::Connection()
    : mPort(0)
    , mIP("0.0.0.0")
{}

Connection::Connection(const std::string& ip, uint16_t port)
    : mPort(port)
    , mIP(ip)
{}

Connection::Connection(const Address& address)
    : mPort(address.port)
    , mIP(address.ip)
{}

// Transport
std::string Transport::getLocalIPv4Address(const std::string& interfaceName)
{
    struct ifaddrs* interfaces;

    if(0 == getifaddrs(&interfaces))
    {
        struct ifaddrs* interface;
        for(interface = interfaces; interface != NULL; interface = interface->ifa_next)
        {
            if(interface->ifa_addr->sa_family == AF_INET)
            {
                // Match
                if(interfaceName == std::string(interface->ifa_name))
                {
                    struct sockaddr_in* sa = (struct sockaddr_in*) interface->ifa_addr;
                    char* addr = inet_ntoa(sa->sin_addr);
                    std::string ip = std::string(addr);

                    freeifaddrs(interfaces);

                    return ip;
                }
            }
        }
    }

    freeifaddrs(interfaces);
    throw std::runtime_error("fipa::services::Transport: could not get interface address of '" + interfaceName + "'");
}


} // end namespace services
} // end namespace fipa