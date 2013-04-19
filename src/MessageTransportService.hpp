#ifndef FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
#define FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP

#include <map>
#include <fipa_services/MessageTransport.hpp>
#include <boost/function.hpp>

namespace fipa {
namespace acl {
    class ACLMessage;
}

namespace service {
namespace message_transport {

class TransportContext;
class TransportTicket;

class ServiceDirectory;

typedef boost::function2<message_transport::Ticket, const fipa::acl::ACLMessage&, message_transport::Context*> TransportHandler;
typedef std::map<message_transport::Type, TransportHandler> TransportHandlerMap;

class MessageTransport
{
    TransportHandlerMap mTransportHandlerMap;
    TicketList mOpenTickets;

public:

    /**
     * \class MessageTransport
     */
    MessageTransport();

    /**
     * handle message
     */
    void handle(const fipa::acl::ACLMessage& msg);

    /**
     * Register a TransportHandler
     * \throw Duplicate
     */
    void registerTransport(const Type& type, TransportHandler handler);

    /**
     * Deregister a TransportHandler
     * \throw Duplicate
     */
    void deregisterTransport(const Type& type);

    /**
     * Modify existing TransportHandler
     * \throw Duplicate
     */
    void modifyTransport(const Type& type, TransportHandler handler);
};

} // end namespace message_transport
} // end namespace service
} // end namespace fipa

#endif // FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
