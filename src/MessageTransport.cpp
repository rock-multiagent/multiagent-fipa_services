#include "MessageTransport.hpp"

namespace fipa {
namespace service {
namespace message_transport {

uint32_t Ticket::currentId = 0;

Ticket::Ticket()
    : id(++currentId)
    , time(base::Time::now())
    , status(UNKOWN) 
{}


} // end namespace message_transport
} // end namespace service
} // end namespace fipa
