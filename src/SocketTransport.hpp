#ifndef FIPA_SERVICE_SOCKET_TRANSPORT_HPP
#define FIPA_SERVICE_SOCKET_TRANSPORT_HPP

#include "MessageTransport.hpp"

#include <fipa_acl/fipa_acl.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace fipa {
namespace services {
namespace message_transport {
    
/**
 * Blah.
 */
class SocketTransport
{
public:
    /**
     * Blah.
     */
    SocketTransport(MessageTransport* mts);
    
    /**
     * Blah.
     */
    fipa::acl::AgentIDList deliverForwardLetter(const fipa::acl::Letter& letter);
    
private:
    MessageTransport* mpMts;
    
    boost::asio::io_service mIo_service;
    boost::asio::ip::tcp::acceptor mAcceptor;
    void startAccept();
};
    
} // namespace message_transport
} // namespace services
} // namespace fipa

#endif // FIPA_SERVICE_SOCKET_TRANSPORT_HPP