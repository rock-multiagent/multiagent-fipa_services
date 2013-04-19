#include "MessageTransportService.hpp"
#include "ErrorHandling.hpp"

namespace fipa {
namespace service {
namespace message_transport {

MessageTransport::MessageTransport()
{
}

void MessageTransport::handle(const fipa::acl::ACLMessage& msg)
{
}

void MessageTransport::registerTransport(const message_transport::Type& type, TransportHandler handle)
{
    if(mTransportHandlerMap.count(type))
    {
        throw DuplicateEntry();
    }

    mTransportHandlerMap[type] = handle;
}

void MessageTransport::deregisterTransport(const message_transport::Type& type)
{
    TransportHandlerMap::iterator it = mTransportHandlerMap.find(type);
    if(it == mTransportHandlerMap.end())
    {
        throw NotFound();
    }

    mTransportHandlerMap.erase(it);
}

void MessageTransport::modifyTransport(const message_transport::Type& type, TransportHandler handler)
{
    TransportHandlerMap::iterator it = mTransportHandlerMap.find(type);
    if(it == mTransportHandlerMap.end())
    {
        throw NotFound();
    }

    mTransportHandlerMap[type] = handler;
}


} // end namespace message_transport
} // end namespace service
} // end namespace fipa
