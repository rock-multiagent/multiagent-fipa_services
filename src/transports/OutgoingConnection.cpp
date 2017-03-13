#include "OutgoingConnection.hpp"

namespace fipa {
namespace services {
namespace transports {

// OutgoingConnection
OutgoingConnection::OutgoingConnection()
    : Connection()
    , mTTL(-1)
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, uint16_t port)
    : Connection(ipaddress, port)
    , mTTL(-1)
{
}

OutgoingConnection::OutgoingConnection(const Address& address)
    : Connection(address)
    , mTTL(-1)
{
}

} // end namespace transport
} // end namespace services
} // end namespace fipa
