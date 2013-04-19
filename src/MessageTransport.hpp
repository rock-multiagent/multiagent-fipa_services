#ifndef FIPA_SERVICE_TRANSPORT_HPP
#define FIPA_SERVICE_TRANSPORT_HPP

#include <stdint.h>
#include <string>
#include <vector>

namespace fipa {
namespace service {
namespace message_transport {

typedef std::string Type;

// Status of the message delivery process
enum Status { UNKOWN, DELIVERY_IN_PROGRESS, DELIVERY_FAILED, DELIVERY_SUCCEEDED };


class Ticket
{
    static uint32_t currentId;
public:
    uint32_t id;
    Status status;

    Ticket();
};


class Context
{
    Ticket ticket;
    Type type;
};

typedef std::vector<Ticket> TicketList;

} // end namespace message_transport
} // end namespace service
} // end namespace fipa

#endif // FIPA_SERVICE_TRANSPORT_HPP
