#include "Address.hpp"

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace fipa {
namespace services {
namespace transports {

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
        throw std::invalid_argument("address '" + addressString + "' malformatted");
    }
}

bool Address::operator==(const Address& other) const
{
    return this->ip == other.ip && this->port == other.port;
}

bool Address::operator<(const Address& other) const
{
    if(*this == other)
        return false;

    if(ip < other.ip)
    {
        return true;
    } else if(port < other.port)
    {
        return true;
    } else if(protocol < other.protocol)
    {
        return true;
    }

    return false;
}

} // end namespace transport
} // end namespace services
} // end namespace fipa
