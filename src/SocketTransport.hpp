#ifndef FIPA_SERVICE_SOCKET_TRANSPORT_HPP
#define FIPA_SERVICE_SOCKET_TRANSPORT_HPP

#include <fipa_acl/fipa_acl.h>

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
    fipa::acl::AgentIDList deliverForwardLetter(const fipa::acl::Letter& letter);
};
    
} // namespace message_transport
} // namespace services
} // namespace fipa

#endif // FIPA_SERVICE_SOCKET_TRANSPORT_HPP