#ifndef FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP
#define FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP

#include <map>
#include <fipa_acl/fipa_acl.h>
#include <fipa_services/DistributedServiceDirectory.hpp>
#include <fipa_services/ServiceLocator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <fipa_services/transports/udt/OutgoingConnection.hpp>

namespace fipa {
namespace services {
namespace transports {

/// Allow to register callbacks with a transport once data arrives
typedef boost::function1<void, const std::string&> TransportObserver;

/**
 * \class Transport
 * \brief Connection management base class
 */
class Transport
{
public:
    /// Builtin transport types that can be activated
    enum Type { UNKNOWN = 0x00, UDT = 0x01, TCP = 0x02, ALL = 0xFF };

    static std::map<Type, std::string> TypeTxt;

private:
    Type mType; 
    std::vector<TransportObserver> mObservers;

    Transport() {}

protected:
    Transport(Type type);

public:
    typedef boost::shared_ptr<Transport> Ptr;

    /**
     * Create a transport of the given type
     * \return TransportType
     */
    static Transport::Ptr create(Type type);

    /**
     * Retrieve the type for a given string
     */
    static Type getTypeFromTxt(const std::string& typeTxt);

    /**
     * Register a callback function that is called via 
     * notify when new data arrives
     */
    void registerObserver(TransportObserver observer);

    /**
     * Get the type of this transport
     * \return type of this transport
     */
    Type getType() const { return mType; }

    /**
     * Get transport name
     */
    std::string getName() const { return Transport::TypeTxt[mType]; }

    /**
     * Get local IPv4 address for a given interface
     * \param interfaceName name of the interface, default is eth0
     * \return address as a string
     */
    static std::string getLocalIPv4Address(const std::string& interfaceName = "eth0");

    /**
     * Send the encoded data
     * \param receiverName name of the receiver to which the data should be sent
     * \param address Address to which the data should be sent
     * \param data Data that should be send to the receiver 
     * \throws std::runtime_error if sending failed
     */
    void send(const std::string& receiverName, const Address& address, const std::string& data);

    /**
     * Get addresses for this transport for all available interfaces
     * \return addresses of this transport for all available interfaces
     */
    std::set<Address> getAddresses() const;

    /**
     * Get address for this transport and for the given interface
     * \return address of this transport for a given interface
     */
    virtual Address getAddress(const std::string& interface = "eth0") const { throw std::runtime_error("fipa::services::Transport::getAddress not implemented by transport: " + getName()); }

    /**
     * Establish outgoing connection
     * This has to be implement by specific transport
     */
    virtual OutgoingConnection::Ptr establishOutgoingConnection(const Address& address) { throw std::runtime_error("fipa::services::Transport::establishOutgoingConnection not implemented by transport: " + getName()); }

    /**
     * Start transport functionality, e.g. listen socket etc.
     */
    virtual void start(uint16_t port = 0, uint32_t maxClients = 50) { throw std::runtime_error("fipa::services::Transport::start not implemented by transport: " + getName()); }

    /**
     * Update transport, e.g. accept new connections and read existing
     */
    virtual void update(bool readAllMessages = true) { throw std::runtime_error("fipa::services::transports::Transport::update not implemented by transport: " + getName()); }

    /**
     * Retrieve and outgoing connection from cache
     * \param receiverName Name of the receiver
     * \param address Address that should correspond to the receiver
     * \return NULL if connection does not exist or is invalid (e.g. when
     * addresses are different)
     */
    OutgoingConnection::Ptr getCachedOutgoingConnection(const std::string& receiverName, const Address& address);

    /**
     * Cleanup the receiver from the outgoing connection list
     */
    void cleanup(const std::string& receiver);

    /**
     * Trigger callbacks upon a newly arrived message
     */
    void notify(const std::string& message);

private:
    /// Outgoing connections for the given transport
    /// key: receiver name
    /// value: connection to this receiver
    std::map<std::string, OutgoingConnection::Ptr> mOutgoingConnections;
};

} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_TRANSPORT_HPP
