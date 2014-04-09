#include "SocketTransport.hpp"

#include <iostream>
#include <vector>

namespace fipa {
namespace services {
namespace message_transport {

fipa::acl::AgentIDList SocketTransport::deliverForwardLetter(const fipa::acl::Letter& letter)
{
    fipa::acl::ACLMessage msg = letter.getACLMessage();
    std::cout << msg.getContent() << std::endl;

    // List of agents which could not be delivery to
    return fipa::acl::AgentIDList();
}
    
} // end namespace message_transport
} // end namespace services
} // end namespace fipa
