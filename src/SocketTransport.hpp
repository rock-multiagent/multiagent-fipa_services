#ifndef FIPA_SERVICE_SOCKET_TRANSPORT_HPP
#define FIPA_SERVICE_SOCKET_TRANSPORT_HPP

#include "MessageTransport.hpp"
#include <fipa_services/DistributedServiceDirectory.hpp>
#include <fipa_acl/fipa_acl.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fipa {
namespace services {
namespace message_transport {
    
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
    SocketTransport(MessageTransport* mts, DistributedServiceDirectory* dsd);
    
    /**
     * This method is called once a letter arrived and needs to be forwarded.
     */
    fipa::acl::AgentIDList deliverForwardLetter(const fipa::acl::Letter& letter);
    
    int getPort();
    
private:
    MessageTransport* mpMts;
    DistributedServiceDirectory* mpDSD;
    
    boost::asio::io_service mIo_service;
    boost::asio::ip::tcp::acceptor mAcceptor;
    void startAccept();
    
    /**
     * Tries to connect to the socket with the given address in format
     * "tcp://IP:Port" and send the given letter. Throws an exception on
     * failure.
     */
    void connectAndSend(const fipa::acl::Letter& letter, std::string addressString);
};
    
} // namespace message_transport
} // namespace services
} // namespace fipa

#endif // FIPA_SERVICE_SOCKET_TRANSPORT_HPP