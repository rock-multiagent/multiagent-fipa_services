#include <vector>

#include "MessageTransport.hpp"
#include "ErrorHandling.hpp"
#include <base/Logging.hpp>
#include <base/Time.hpp>

#include <boost/algorithm/string.hpp>

namespace fipa {
namespace services {
namespace message_transport {

MessageTransport::MessageTransport(const fipa::acl::AgentID& id)
    : mAgentId(id)
    , mRepresentation(fipa::acl::representation::BITEFFICIENT)
{
}

void MessageTransport::handle(fipa::acl::Letter& letter)
{
    // This prevents looping (also in the case of communication errors)
    if(hasStamp(letter))
    {
        LOG_INFO("Agent '%s' received already stamped message. Conversation id: %s", mAgentId.getName().c_str(), letter.getACLMessage().getConversationID().c_str());
        return;
    } 

    // Note that the message needs to be updated using a stamp of this message transport service
    stamp(letter);

    // If this letter is internal communication, there is no need to proceed further
    if(handleInternalCommunication(letter))
        return;

    if(!forward(letter))
    {
        handleError(letter);
    }
}

bool MessageTransport::handleInternalCommunication(const fipa::acl::Letter& letter)
{
    // Check if message is directed to 'self'
    using namespace fipa::acl;

    AgentIDList receivers = letter.getBaseEnvelope().getTo();
    AgentIDList::const_iterator cit = std::find(receivers.begin(), receivers.end(), mAgentId);

    if(receivers.end() != cit)
    {
        ACLMessage message = letter.getACLMessage();
        if(message.getOntology() == fipa::agent_management::ONTOLOGY)
        {
            std::string content = message.getContent();
            // handle content
        }
    }
    return false;
}

void MessageTransport::handleError(const fipa::acl::Letter& letter) const
{
    fipa::acl::ACLMessage errorMessage = createInternalErrorMessage(letter.getACLMessage(), "Message delivery failed! Delivery path: " + letter.getDeliveryPathString());
    fipa::acl::Letter errorLetter(errorMessage, mRepresentation);
    stamp(errorLetter);
    if(!forward(errorLetter))
    {
        // we do not if forwarding of the error fails
        LOG_WARN("Forwarding of error failed. Conversation id: %s", errorMessage.getConversationID().c_str());
    }
}

void MessageTransport::registerTransport(const message_transport::Type& type, TransportHandler handle)
{
    if(mTransportHandlerMap.count(type))
    {
        throw DuplicateEntry();
    }

    // Update priority based on order of insertion
    TransportPriorityList::const_iterator cit = std::find(mTransportPriorityList.begin(), mTransportPriorityList.end(), type);
    if(cit == mTransportPriorityList.end())
    {
        mTransportPriorityList.push_back(type);
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

    // Remove from priority list
    TransportPriorityList::iterator cit = std::find(mTransportPriorityList.begin(), mTransportPriorityList.end(), type);
    assert(cit != mTransportPriorityList.end());

    mTransportPriorityList.erase(cit);
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

void MessageTransport::stamp(fipa::acl::Letter& letter) const
{
    letter.stamp(mAgentId);
}


bool MessageTransport::hasStamp(const fipa::acl::Letter& letter) const
{
    return letter.hasStamp(mAgentId);
}


fipa::acl::ACLMessage MessageTransport::createInternalErrorMessage(const fipa::acl::ACLMessage& msg, const std::string& description) const
{
    // Generate error message
    // using reply-to and existing sender to create a respons message
    using namespace fipa::acl;
    ACLMessage errorMsg;
    AgentID sender = msg.getSender();
    errorMsg.addReceiver(sender);
    errorMsg.setSender(mAgentId);
    errorMsg.setConversationID( msg.getConversationID());
    errorMsg.setInReplyTo( msg.getReplyWith());

    // Set field to allow identification of internal / fipa_agent_management error
    errorMsg.setPerformative(ACLMessage::FAILURE);
    errorMsg.setOntology(fipa::agent_management::ONTOLOGY);
    errorMsg.setContent(fipa::agent_management::INTERNAL_ERROR + " " + description);

    return errorMsg;
}

bool MessageTransport::forward(const fipa::acl::Letter& letter) const
{
    bool success = false;

    TransportPriorityList::const_iterator cit = mTransportPriorityList.begin();
    for(; cit != mTransportPriorityList.end(); ++cit)
    {
        TransportHandlerMap::const_iterator tit = mTransportHandlerMap.find(*cit);
        if(tit != mTransportHandlerMap.end())
        {
            TransportHandler transportHandler = tit->second;

            if( (success = transportHandler(letter)) )
            {
                LOG_DEBUG("Successfully forwarded letter");
                break;
            } else {
                LOG_DEBUG("Failed to forward letter");
            }
        }
    }
    return success;
}

} // end namespace message_transport
} // end namespace services
} // end namespace fipa
