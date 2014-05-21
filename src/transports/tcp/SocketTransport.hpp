#ifndef FIPA_SERVICE_SOCKET_TRANSPORT_HPP
#define FIPA_SERVICE_SOCKET_TRANSPORT_HPP

#include <fipa_services/MessageTransport.hpp>
#include <fipa_services/DistributedServiceDirectory.hpp>
#include <fipa_acl/fipa_acl.h>
#include <fipa_services/transports/Transport.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fipa {
namespace services {
namespace tcp {
    
/**
 * \class OutgoingConnection
 * \brief A unidirectional, outgoing tcp connection that allows to send fipa letter to a receiver
 */
class OutgoingConnection : public fipa::services::AbstractOutgoingConnection
{
public:
    OutgoingConnection();
    OutgoingConnection(const std::string& ipaddress, uint16_t port);
    OutgoingConnection(const fipa::services::Address& address);

    /**
     * Connect to ipaddress and port
     * \param ipaddress IP as string
     * \param port Port number
     * \throws if connection cannot be established
     */
    void connect(const std::string& ipaddress, uint16_t port);

    /**
     * Send fipa::acl::Letter
     * \param letter FIPA letter
     */
    void sendLetter(fipa::acl::Letter& letter);
    
private:
    boost::asio::ip::tcp::socket mClientSocket;
};    

/**
 * This transport implementation receives messages on a socket and forwards them to ROCK agents. Everything is static.
 */
class SocketTransport
{
public:
    /**
     * Starts to listen for new tcp connections. This is done in a new thread.
     */
    static void startListening(fipa::services::message_transport::MessageTransport* mts);
    /**
     * Stops listening.
     */
    static void stopListening();
    /**
     * Gets the address it is being listened on.
     */
    static fipa::services::Address getAddress(const std::string& interfaceName = "eth0");
    /**
     * Gets the io_service object used for all operations.
     */
    static boost::asio::io_service& getIOService();
    
private:
    // Address and Port
    static int getPort();
    // start accepting new connections, each reading in an own thread
    static void startAccept();
    /* 
     * Reads from one socket, until the connection is closed by the other side.
     * All read envelopes are dispatched directly.
     * The read method deletes the socket after having finished.
     */
    static void read(boost::asio::ip::tcp::socket* socket);
    
    static boost::asio::io_service io_service;
    static boost::asio::ip::tcp::acceptor mAcceptor;
    static fipa::services::message_transport::MessageTransport* mpMts;
};
    
} // namespace tcp
} // namespace services
} // namespace fipa

#endif // FIPA_SERVICE_SOCKET_TRANSPORT_HPP