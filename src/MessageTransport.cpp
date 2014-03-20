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

    fipa::acl::AgentIDList remainingReceivers = forward(letter);
    if(!remainingReceivers.empty())
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
            // handle content
            std::string content = message.getContent();
            LOG_DEBUG("Received internal message for agent management: '%s'", content.c_str());
        } else {
            std::string content = message.getContent();
            LOG_DEBUG("Received external message -- content will be ignored: '%s'", content.c_str());
        }
        return true;
    }
    return false;
}

void MessageTransport::handleError(const fipa::acl::Letter& letter) const
{
    // Adding all relevant information as inner message (acl message in string encoding)
    fipa::acl::ACLBaseEnvelope flattenedLetter = letter.flattened();
    fipa::acl::ACLMessage innerMessage;
    innerMessage.setSender(flattenedLetter.getFrom());
    innerMessage.setAllReceivers(flattenedLetter.getIntendedReceivers());
    innerMessage.setLanguage(fipa::agent_management::INTERNAL_ERROR);
    innerMessage.setContent("description: message delivery failed\ndelivery path: " + letter.getDeliveryPathString());
    std::string errorDescription = fipa::acl::MessageGenerator::create(innerMessage, fipa::acl::representation::STRING_REP);

    fipa::acl::ACLMessage errorMessage = createInternalErrorMessage(letter.getACLMessage(), errorDescription);
    fipa::acl::Letter errorLetter(errorMessage, mRepresentation);
    stamp(errorLetter);

    fipa::acl::AgentIDList remainingReceivers = forward(errorLetter);
    if(!remainingReceivers.empty())
    {
        fipa::acl::AgentIDList::const_iterator cit = remainingReceivers.begin();
        for(; cit != remainingReceivers.end(); ++cit)
        {
            // we do not if forwarding of the error fails
            LOG_WARN("Forwarding of error to '%s' failed. Conversation id: %s", cit->getName().c_str(), errorMessage.getConversationID().c_str());
        }
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
        throw NotFound("transport type: " + type );
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
        throw NotFound("transport type: " + type);
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
    errorMsg.setProtocol(msg.getProtocol());

    // Set field to allow identification of internal / fipa_agent_management error
    errorMsg.setPerformative(ACLMessage::FAILURE);
    errorMsg.setOntology(fipa::agent_management::ONTOLOGY);
    errorMsg.setContent(description);


    return errorMsg;
}

fipa::acl::AgentIDList MessageTransport::forward(fipa::acl::Letter& letter) const
{
    fipa::acl::AgentIDList remainingReceivers = letter.flattened().getIntendedReceivers();

    TransportPriorityList::const_iterator cit = mTransportPriorityList.begin();
    for(; cit != mTransportPriorityList.end(); ++cit)
    {
        TransportHandlerMap::const_iterator tit = mTransportHandlerMap.find(*cit);
        if(tit != mTransportHandlerMap.end())
        {
            TransportHandler transportHandler = tit->second;
            remainingReceivers = transportHandler(letter);

            if(!remainingReceivers.empty())
            {
                fipa::acl::ACLBaseEnvelope extraEnvelope;
                extraEnvelope.setIntendedReceivers(remainingReceivers);
                letter.addExtraEnvelope(extraEnvelope);
            } else {
                LOG_DEBUG("Delivered successfully to all agents");
                break;
            }
        }
    }

    fipa::acl::AgentIDList::const_iterator ait = remainingReceivers.begin();
    for(; ait != remainingReceivers.end(); ++ait)
    {
        LOG_DEBUG("Could not forward letter (with any available transport) to: '%s'", ait->getName().c_str());
    }

    return remainingReceivers;
}

} // end namespace message_transport
} // end namespace services
} // end namespace fipa
