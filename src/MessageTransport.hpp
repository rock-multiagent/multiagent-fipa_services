#ifndef FIPA_SERVICE_TRANSPORT_HPP
#define FIPA_SERVICE_TRANSPORT_HPP

#include <stdint.h>
#include <string>
#include <vector>
#include <base/time.h>

namespace fipa {
namespace service {
namespace message_transport {

typedef std::string Type;

// Status of the message delivery process
enum Status { UNKOWN, DELIVERY_IN_PROGRESS, DELIVERY_FAILED, DELIVERY_SUCCEEDED };

/**
 * \class Ticket
 * \brief A ticket should be created for each message so that the status can be tracked
 */
class Ticket
{
    static uint32_t currentId;
public:
    uint32_t id;
    base::Time time;
    Status status;

    Ticket();
};

/**
 * \class Context
 * \brief The Context allow to embed transport specific information if needed, but must at least
 * contain the created ticket for this message job
 */
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
