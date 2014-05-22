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
    OutgoingConnection(const std::string& ipaddress, uint16_t port, const std::vector<fipa::services::ServiceLocation>& mServiceLocations);
    OutgoingConnection(const fipa::services::Address& address, const std::vector<fipa::services::ServiceLocation>& mServiceLocations);

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
    // The service locations, from which the addresses will be put into 
    const std::vector<fipa::services::ServiceLocation> mServiceLocations;
};    

/**
 * This transport implementation receives messages on a socket and forwards them to ROCK agents. Everything is static.
 */
class SocketTransport
{
public:
    /**
     * Gets the io_service object used for all operations.
     */
    static boost::asio::io_service& getIOService();
    
    /**
     * Constructor
     */
    SocketTransport(fipa::services::message_transport::MessageTransport* mts);
    /**
     * Starts to listen for new tcp connections. This is done in a new thread.
     */
    void startListening();
    /**
     * Gets the address it is being listened on.
     */
    fipa::services::Address getAddress(const std::string& interfaceName = "eth0");
    
    
private:
    static boost::asio::io_service io_service;
    
    // Address and Port
    int getPort();
    // start accepting new connections, each reading in an own thread
    void startAccept();
    /* 
     * Reads from one socket, until the connection is closed by the other side.
     * All read envelopes are dispatched directly.
     * The read method deletes the socket after having finished.
     */
    void read(boost::asio::ip::tcp::socket* socket);
    
    boost::asio::ip::tcp::acceptor mAcceptor;
    fipa::services::message_transport::MessageTransport* mpMts;
};
    
} // namespace tcp
} // namespace services
} // namespace fipa

#endif // FIPA_SERVICE_SOCKET_TRANSPORT_HPP