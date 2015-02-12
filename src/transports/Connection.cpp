#include "Connection.hpp"

namespace fipa {
namespace services {
namespace transports {

// Connection
Connection::Connection()
    : mAddress("0.0.0.0", 0)
{}

Connection::Connection(const std::string& ip, uint16_t port, const std::string& protocol)
    : mAddress(ip, port, protocol)
{}

Connection::Connection(const Address& address)
    : mAddress(address)
{}

} // end namespace transport
} // end namespace services
} // end namespace fipa
