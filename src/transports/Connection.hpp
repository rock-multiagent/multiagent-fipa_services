#ifndef FIPA_SERVICES_TRANSPORTS_CONNECTION_HPP
#define FIPA_SERVICES_TRANSPORTS_CONNECTION_HPP

#include <fipa_services/transports/Address.hpp>

namespace fipa {
namespace services {
namespace transports {

/**
 * \class Connection
 * \brief A connection is defined by a communication address
 * \see Address
 */
class Connection
{
protected:
    Address mAddress;
public:
    Connection();
    Connection(const std::string& ip, uint16_t port, const std::string& protocol = "udt");
    Connection(const Address& address);

    uint16_t getPort() const { return mAddress.port; }
    std::string getIP() const { return mAddress.ip; }

    Address getAddress() const { return mAddress; }

    bool operator==(const Connection& other) const { return mAddress == other.mAddress; }
};

} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_CONNECTION_HPP
