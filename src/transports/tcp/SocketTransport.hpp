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
    ~OutgoingConnection();

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
    boost::asio::io_service mIo_service;
};    

    
/**
 * This transport implementation forwards messages via sockets to JADE (or something else). It also receives messages on a socket and forwards them
 * in the other direction, to ROCK agents.
 */
class SocketTransport
{
public:
    /**
     * The constructor needs pointers the the main message transport and to the DistributedServiceDirectory,
     * to locate JADE agents.
     */
    SocketTransport(fipa::services::message_transport::MessageTransport* mts, DistributedServiceDirectory* dsd);
    
    /**
     * This method is called once a letter arrived and needs to be forwarded.
     */
    fipa::acl::AgentIDList deliverForwardLetter(const fipa::acl::Letter& letter);
    
    fipa::services::Address getAddress(const std::string& interfaceName = "eth0");
    
private:
    fipa::services::message_transport::MessageTransport* mpMts;
    DistributedServiceDirectory* mpDSD;
    boost::asio::io_service mIo_service;
    boost::asio::ip::tcp::acceptor mAcceptor;
    boost::asio::ip::tcp::socket mSocket;
    // FIXME Debug
    boost::asio::ip::tcp::socket clientSocket;
    
    // Address and Port
    int getPort();
    
    void startAccept();
    
    /**
     * Tries to connect to the socket with the given address in format
     * "tcp://IP:Port" and send the given letter. Throws an exception on
     * failure.
     */
    void connectAndSend(fipa::acl::Letter& letter, const std::string& addressString);
};
    
} // namespace tcp
} // namespace services
} // namespace fipa

#endif // FIPA_SERVICE_SOCKET_TRANSPORT_HPP