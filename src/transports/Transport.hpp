#ifndef FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP
#define FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP

#include <string>
#include <stdint.h>
#include <fipa_acl/fipa_acl.h>
#include <fipa_services/DistributedServiceDirectory.hpp>
#include <fipa_services/ServiceLocator.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fipa {
namespace services {
// Forward declarations TODO remove
namespace udt {
    class OutgoingConnection;
}
namespace tcp {
    class OutgoingConnection;
}
    
/**
 * \class Address
 * \brief Communication address specified by ip,  port, and protocol
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

/**
 * \class AbstractOutgoingConnection
 * \brief A unidirectional, outgoing connection that allows to send fipa letters to a receiver
 */
class AbstractOutgoingConnection : public fipa::services::Connection
{
public:
    AbstractOutgoingConnection();
    AbstractOutgoingConnection(const std::string& ipaddress, uint16_t port);
    AbstractOutgoingConnection(const fipa::services::Address& address);
    virtual ~AbstractOutgoingConnection() {};
    
    /**
     * Connect to ipaddress and port
     * \param ipaddress IP as string
     * \param port Port number
     * \throws if connection cannot be established
     */
    virtual void connect(const std::string& ipaddress, uint16_t port) = 0;

    /**
     * Send fipa::acl::Letter
     * \param letter FIPA letter
     */
    virtual void sendLetter(fipa::acl::Letter& letter) = 0;
};

/**
 * \class Transport
 * \brief Connection management base class. Supports udt and tcp.
 * Also static method collection facilitating transport implementations.
 */
class Transport
{
public:
    Transport(const std::string& name, DistributedServiceDirectory* dsd, const std::map<std::string, fipa::services::ServiceLocation> mServiceLocations);
    
    /**
     * Get local IPv4 address for a given interface
     * \param interfaceName name of the interface, default is eth0
     * \return address as a string
     */
    static std::string getLocalIPv4Address(const std::string& interfaceName = "eth0");
    
    /**
     * Forwards a letter via UDT.
     * \param letter enenvelope
     * \return list of agents for which the delivery failed
     */
    fipa::acl::AgentIDList deliverOrForwardLetter(const fipa::acl::Letter& letter);
    
    // The name of the MessageTransportTask using this.
    const std::string& getName() const {return name; }
    
    const std::vector<fipa::services::ServiceLocation> getServiceLocations() const;
    
private:
    static std::vector<std::string> additionalAcceptedSignatureTypes;
    
    std::string name;
    DistributedServiceDirectory* mpDSD;
    // Outgoing connections for each protocol
    std::map<std::string, std::map<std::string, fipa::services::AbstractOutgoingConnection*> > mOutgoingConnections;
    // Service location for each protocol.
    // The protocolsfor which serviceLocations exist in this map, are activated
    std::map<std::string, fipa::services::ServiceLocation> mServiceLocations;
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP
