#include "OutgoingConnection.hpp"

namespace fipa {
namespace services {
namespace transports {

// OutgoingConnection
OutgoingConnection::OutgoingConnection()
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, uint16_t port)
    : Connection(ipaddress, port)
{
}

OutgoingConnection::OutgoingConnection(const Address& address)
    : Connection(address)
{
}

} // end namespace transport
} // end namespace services
} // end namespace fipa
