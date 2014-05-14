#ifndef FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP
#define FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP

#include <string>
#include <stdint.h>

namespace fipa {
namespace services {
    
/**
 * \class Address
 * \brief Communication address specified by ip and port
 */
struct Address
{
    std::string ip;
    uint16_t port;
    std::string protocol;

    Address() {}

    // Default protocol is udt.
    Address(const std::string& ip, uint16_t port, const std::string& protocol = "udt");

    /**
     * Convert address to string
     * \return Address as string
     */
    std::string toString() const;

    /**
     * Create address from string
     * \return ArgumentError if address is malformatted
     */
    static Address fromString(const std::string& address);

    /**
     * Equals operator
     * \return True if equal, false otherwise
     */
    bool operator==(const Address& other) const;

    bool operator!=(const Address& other) const { return !this->operator==(other); }
};

/**
 * \class Connection
 * \brief A connection is defined by a communication address
 * \see Address
 */
class Connection
{
protected:
    uint16_t mPort;
    std::string mIP;
public:
    Connection();
    Connection(const std::string& ip, uint16_t port);
    Connection(const Address& address);

    uint16_t getPort() const { return mPort; }
    std::string getIP() const { return mIP; }

    Address getAddress() const { return Address(mIP, mPort); }

    bool operator==(const Connection& other) const { return mPort == other.mPort && mIP == other.mIP; }
};

/**
 * \class Transport
 * \brief Static method collection facilitating transport implementations.
 */
class Transport
{
public:
    /**
     * Get local IPv4 address for a given interface
     * \param interfaceName name of the interface, default is eth0
     * \return address as a string
     */
    static std::string getLocalIPv4Address(const std::string& interfaceName = "eth0");
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP
