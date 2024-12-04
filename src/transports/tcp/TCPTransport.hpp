#ifndef FIPA_SERVICES_TRANSPORTS_TCP_TCP_TRANSPORT_HPP
#define FIPA_SERVICES_TRANSPORTS_TCP_TCP_TRANSPORT_HPP

#include <fipa_services/MessageTransport.hpp>
#include <fipa_services/DistributedServiceDirectory.hpp>
#include <fipa_acl/fipa_acl.h>
#include "../Transport.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fipa {
namespace services {
namespace transports {
namespace tcp {

/**
 * This transport implementation receives messages on a socket and forwards them to ROCK agents. Everything is static.
 */
class TCPTransport : public Transport
{
    friend class OutgoingConnection;

    typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;

    static boost::asio::io_service msIOService;
    boost::asio::ip::tcp::acceptor mAcceptor;

    // List of connected clients
    std::vector<SocketPtr> mClients;

    /**
     * Get port of the listening connection
     * \return 0 if connection has not been established yet
     */
    uint16_t getPort() const;

    /**
     * Accept all new clients and store the connections in an internal
     * client list
     */
    void accept();

    /*
     * Reads from one socket, until the connection is closed by the other side.
     * All read envelopes are dispatched directly.
     * The read method deletes the socket after having finished.
     */
    bool read(SocketPtr socket);

protected:
    /**
     * Gets the io_service object used for all operations.
     */
    static boost::asio::io_service& getIOService();

public:
    /**
     * Default constructor for TCPTransport
     */
    TCPTransport();
    ~TCPTransport();

    /**
     * Start the transport using on a given port
     * Note that setting maxClients has no effect for this transport.
     */
    virtual void start();

    /**
     * Starts to listen for new tcp connections on a given port.
     * This is done in a new thread.
     */
    void listen(uint16_t port = 0);

    /**
     * Update client connection by reading sockets
     * \param readAllMessages if set to true, try reading all connections otherwise only one (might lead to starvation)
     */
    void update(bool readAllMessages);

    /**
     * Gets the address it is being listened on.
     * \return Address for TCPTransport
     */
    Address getAddress(const std::string& interfaceName = "eth0") const;

    /**
     * Establish outgoing connection using TCPTransport
     * \return OutgoingConnection to the given address
     */
    virtual OutgoingConnection::Ptr establishOutgoingConnection(const Address& address);

};

} // end namespace tcp
} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_TCP_TCP_TRANSPORT_HPP
