#ifndef FIPA_SERVICES_TRANSPORTS_ADDRESS_HPP
#define FIPA_SERVICES_TRANSPORTS_ADDRESS_HPP

#include <string>
#include <stdint.h>

namespace fipa {
namespace services {
namespace transports {

/**
 * \class Address
 * \brief Communication address specified by IP,  port, and protocol
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
     * \return std::invalid_argument if address is malformatted
     */
    static Address fromString(const std::string& address);

    /**
     * Equals operator
     * \return True if equal, false otherwise
     */
    bool operator==(const Address& other) const;

    bool operator!=(const Address& other) const { return !this->operator==(other); }

    /**
     * Less than operator, to allow usage of this structure with std::set
     * \return true if other address is greater, false otherwise
     */
    bool operator<(const Address& other) const;
};

} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_ADDRESS_HPP
