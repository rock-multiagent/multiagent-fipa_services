#include <vector>

#include "MessageTransport.hpp"
#include "ErrorHandling.hpp"
#include <base-logging/Logging.hpp>
#include <base/Time.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>

#include <fipa_acl/message_generator/envelope_generator.h>
#include <fipa_acl/message_parser/envelope_parser.h>

namespace fipa {
namespace services {
namespace message_transport {

MessageTransport::MessageTransport(const fipa::acl::AgentID& id, ServiceDirectory::Ptr serviceDirectory)
    : mAgentId(id)
    , mpServiceDirectory(serviceDirectory)
    , mRepresentation(fipa::acl::representation::BITEFFICIENT)
    , mServiceSignature("fipa::services::transports::MessageTransport")
{
    mAcceptedServiceSignatures.insert(mServiceSignature);
    mAcceptedServiceSignatures.insert("JadeProxyAgent");

    if(!mpServiceDirectory)
    {
        throw std::invalid_argument("MessageTransport: a service directory is required for instanciation");
    }
}

void MessageTransport::activateTransports(transports::Transport::Type flags)
{
    using namespace fipa::services::transports;
    for(int i = static_cast<int>(Transport::UDT); i < static_cast<int>(Transport::ALL); ++i)
    {
        Transport::Type type = static_cast<Transport::Type>(i);
        if(flags & type)
        {
            activateTransport(type);
        }
    }
}

void MessageTransport::activateTransport(transports::Transport::Type type)
{
    if( mActiveTransports.end() != mActiveTransports.find(type))
    {
        throw std::runtime_error("MessageTransport::activateTransport: transport '" + transports::Transport::TypeTxt[type] + "' has already been activated");
    }

    transports::Transport::Ptr transport = transports::Transport::create( type );
    std::string typeTxt = transport->getName();

    // Check if there is a configuration for this transport
    std::vector<transports::Configuration>::const_iterator configIt = std::find_if(mTransportConfigurations.begin(), mTransportConfigurations.end(), [typeTxt](const transports::Configuration& config) { return config.transport_type == typeTxt; } );

    if(configIt != mTransportConfigurations.end())
    {
        transport->setConfiguration( *configIt );
    }
    transport->start();

    transport->registerObserver(boost::bind(&MessageTransport::handleData, this, boost::placeholders::_1));
    cacheTransportEndpoints(transport);

    mActiveTransports[type] = transport;
}

void MessageTransport::activateTransports(const std::vector<std::string>& transportNames)
{
    // Add transports to activation list
    std::vector<std::string>::const_iterator cit = transportNames.begin();
    for(; cit != transportNames.end(); ++cit)
    {
        transports::Transport::Type type = transports::Transport::getTypeFromTxt(*cit);
        activateTransport(type);
    }
}

std::vector<fipa::services::ServiceLocation> MessageTransport::getTransportEndpoints() const
{
    return mTransportEndpoints;
}

void MessageTransport::cacheTransportEndpoints(transports::Transport::Ptr transport)
{
    using namespace fipa::services::transports;

    std::vector<fipa::services::ServiceLocation> serviceLocations;
    std::set<Address> addresses = transport->getAddresses();

    std::set<Address>::const_iterator ait = addresses.begin();
    for(; ait != addresses.end(); ++ait)
    {
        serviceLocations.push_back(fipa::services::ServiceLocation(ait->toString(), mServiceSignature));
    }

    mTransportEndpoints.insert(mTransportEndpoints.end(), serviceLocations.begin(), serviceLocations.end());
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

void MessageTransport::registerMessageTransport(const std::string& type, MessageTransportHandler handle)
{
    if(mMessageTransportHandlerMap.count(type))
    {
        throw DuplicateEntry();
    }

    // Update priority based on order of insertion
    MessageTransportPriorityList::const_iterator cit = std::find(mMessageTransportPriorityList.begin(), mMessageTransportPriorityList.end(), type);
    if(cit == mMessageTransportPriorityList.end())
    {
        mMessageTransportPriorityList.push_back(type);
    }

    mMessageTransportHandlerMap[type] = handle;
}

void MessageTransport::deregisterMessageTransport(const std::string& type)
{
    MessageTransportHandlerMap::iterator it = mMessageTransportHandlerMap.find(type);
    if(it == mMessageTransportHandlerMap.end())
    {
        throw NotFound("transport type: " + type );
    }

    // Remove from priority list
    MessageTransportPriorityList::iterator cit = std::find(mMessageTransportPriorityList.begin(), mMessageTransportPriorityList.end(), type);
    assert(cit != mMessageTransportPriorityList.end());

    mMessageTransportPriorityList.erase(cit);
    mMessageTransportHandlerMap.erase(it);
}

void MessageTransport::modifyMessageTransport(const std::string& type, MessageTransportHandler handler)
{
    MessageTransportHandlerMap::iterator it = mMessageTransportHandlerMap.find(type);
    if(it == mMessageTransportHandlerMap.end())
    {
        throw NotFound("transport type: " + type);
    }

    mMessageTransportHandlerMap[type] = handler;
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

bool MessageTransport::localForward(const std::string& receiverName, const fipa::acl::Letter& letter) const
{
    MessageTransportPriorityList::const_iterator cit = mMessageTransportPriorityList.begin();
    for(; cit != mMessageTransportPriorityList.end(); ++cit)
    {
        MessageTransportHandlerMap::const_iterator tit = mMessageTransportHandlerMap.find(*cit);
        if(tit != mMessageTransportHandlerMap.end())
        {
            MessageTransportHandler transportHandler = tit->second;
            if( transportHandler(receiverName, letter) )
            {
                LOG_DEBUG_S << "Delivered successfully to '" << receiverName << "'";
                return true;
            }
        }
    }
    return false;
}

fipa::acl::AgentIDList MessageTransport::forward(const fipa::acl::Letter& letter) const
{
    using namespace fipa::acl;

    // Get access to the base envelope
    ACLBaseEnvelope envelope = letter.flattened();
    // Get the list of intended receivers
    AgentIDList receivers = envelope.getIntendedReceivers();
    LOG_DEBUG_S << "Intended receivers: " << receivers;
    AgentIDList::const_iterator rit = receivers.begin();

    // The list of remaining receivers -- e.g. if a transport failed
    AgentIDList remainingReceivers = receivers;

    // For each intended receiver try to deliver
    // Try to forward to a receiver using the information given in the service
    // directory, i.e. using the locators of this service, which is an MTS in this context
    for(; rit != receivers.end(); ++rit)
    {
        LOG_DEBUG_S << "MessageTransport '" << mAgentId.getName() << "': deliverOrForwardLetter to: " << rit->getName();

        // Handle delivery
        // The name of the next destination -- this next destination can also be an intermediate receiver
        std::string receiverName = rit->getName();

        // Check for local receivers, or identify locator
        bool doThrow = false;

        // Add "$" to make sure names are not interpreted as prefix
        fipa::services::ServiceDirectoryList list = mpServiceDirectory->search(receiverName + "$", fipa::services::ServiceDirectoryEntry::NAME, doThrow);
        // The list can be either empty or contain or or multiple entries, e.g.
        // regex pattern matching simplifies broadcasting/multicasting
        if(list.empty())
        {
            // Try local delivery
            LOG_DEBUG_S << "Could not find receiver " << receiverName << " in service directory: trying local delivery";
            if(localForward(receiverName, letter))
            {
                removeFromList(*rit, remainingReceivers);
                break;
            } else {
                LOG_WARN_S << "MessageTransport '" << mAgentId.getName() << "': could neither deliver nor forward message to receiver: '" << receiverName << "' since it is globally and locally unknown";
            }

            // Iterate over the list of builtin transports and cleanup the cache
            std::map<transports::Transport::Type, transports::Transport::Ptr>::iterator it = mActiveTransports.begin();
            for(; it != mActiveTransports.end(); ++it)
            {
                it->second->cleanup(receiverName);
            }
            continue;
        } else {
            using namespace fipa::services;
            ServiceDirectoryList::const_iterator cit = list.begin();
            for(; cit != list.end(); ++cit)
            {
                ServiceDirectoryEntry serviceEntry = *cit;

                // Filter out sender from broadcast/multicast
                if(serviceEntry.getName() == envelope.getFrom().getName())
                {
                    LOG_DEBUG_S << "Skipping sending broadcast to self " << envelope.getFrom().getName();
                    continue;
                }

                ServiceLocator locator = serviceEntry.getLocator();
                ServiceLocations locations = locator.getLocations();
                for(ServiceLocations::const_iterator it = locations.begin(); it != locations.end(); it++)
                {
                    // Retrieve address
                    ServiceLocation location = *it;

                    try{
                        // Handle delivery
                        // The name of the next destination -- this next destination can also be an intermediate receiver
                        receiverName = serviceEntry.getName();
                        fipa::acl::Letter updatedLetter = letter.createDedicatedEnvelope( fipa::acl::AgentID(receiverName) );
                        // serviceEntry.getName() is our receiver -- after
                        // resolution of regex
                        forward(serviceEntry.getName(), location, updatedLetter);
                        removeFromList(*rit, remainingReceivers);

                        // Successfully sent. Break locations loop.
                        break;

                    } catch(const std::runtime_error& e)
                    {
                        LOG_WARN_S << "MessageTransport: '" << mAgentId.getName() << ": could not send letter to '" << receiverName << "' -- via location: " << location.toString() << " " << e.what();
                        continue;
                    }

                } // end for
            }
        } // end else
    } // end for receivers

    return remainingReceivers;
}

bool MessageTransport::hasActiveTransport(const std::string& protocol) const
{
    transports::Transport::Type type = transports::Transport::getTypeFromTxt(protocol);
    return mActiveTransports.end() != mActiveTransports.find(type);
}

bool MessageTransport::isLocal(const ServiceLocation& local) const
{
    return mTransportEndpoints.end() != std::find(mTransportEndpoints.begin(), mTransportEndpoints.end(), local);
}

void MessageTransport::handleData(const std::string& data)
{
    using namespace fipa::acl;

    LOG_DEBUG_S << mAgentId.getName() << " received data ";

    Letter letter;
    for(int i = static_cast<int>(representation::BITEFFICIENT); i < static_cast<int>(representation::END_MARKER); ++i)
    {
        try {
            representation::Type rep = static_cast<representation::Type>(i);
            if( fipa::acl::EnvelopeParser::parseData(data, letter, rep) )
            {
                LOG_DEBUG_S << mAgentId.getName() << " decoding of envelope succeeded: " << representation::TypeTxt[rep];
                LOG_DEBUG_S << mAgentId.getName() << " forward envelope to handler";
                handle(letter);
                return;
            }
        } catch(const std::runtime_error& e)
        {
            LOG_INFO_S << e.what();
        }
    }
    LOG_WARN_S << "Failed to handle data: check correct use of representation";
}


void MessageTransport::forward(const std::string& receiverName, const ServiceLocation& location, const fipa::acl::Letter& letter) const
{
    transports::Address address;
    try {
        address = transports::Address::fromString(location.getServiceAddress());
    } catch(const std::invalid_argument& e)
    {
        throw std::runtime_error("MessageTransport '" + mAgentId.getName() + "' : address '" + location.getServiceAddress() + "' for receiver '" + receiverName + "'");
    }

    // Check if the destination is a local address
    if(isLocal(location))
    {
        LOG_DEBUG_S << "Receiver: " << receiverName << " at " << location.toString() << " is local receiver";
        if(!localForward(receiverName, letter))
        {
            throw std::runtime_error("MessageTransport '" + mAgentId.getName() + "': could not forward to receiver: '" + receiverName + "' -- local delivery failed");
        }
    } else {
        // Check if the transport that corresponds to the protocol is allowed
        if(!hasActiveTransport(address.protocol))
        {
            // Protocol not implemented (or not activated)
            throw std::runtime_error("MessageTransport '" + mAgentId.getName() + "' : transport protocol '" + address.protocol + "' is not active or supported.");
        }

        // Check if the service signature matches
        if( mAcceptedServiceSignatures.end() == std::find( mAcceptedServiceSignatures.begin(), mAcceptedServiceSignatures.end(), location.getSignatureType()))
        {
            throw std::runtime_error("MessageTransport '" + mAgentId.getName() + "': service signature for '" + receiverName + "' is '" + location.getSignatureType() + "' and is not on the list of accepted signatures -- will not connect to: " + location.toString());
        }

        transports::Transport::Type type = transports::Transport::getTypeFromTxt(address.protocol);
        std::map<transports::Transport::Type, transports::Transport::Ptr>::iterator tit = mActiveTransports.find(type);
        if( tit != mActiveTransports.end() )
        {
            transports::Transport::Ptr transport = tit->second;
            LOG_DEBUG_S << "MessageTransport: '" << transport->getName() << "': forwarding to other MTS";

            std::string data = serializeLetter(letter, location.getSignatureType());

            // Try sending via given transport
            // will throw on failure
            transport->send(receiverName, address, data);
        }
    } // end else
}


void MessageTransport::removeFromList(const fipa::acl::AgentID& agent, fipa::acl::AgentIDList& agents)
{
    // Sucessfully send, remove from list of remaining
    // receivers
    fipa::acl::AgentIDList::iterator it = std::find(agents.begin(), agents.end(), agent);
    if(it != agents.end())
    {
        agents.erase(it);
    }
}

void MessageTransport::trigger()
{
    using namespace fipa::services::transports;
    std::map<Transport::Type, Transport::Ptr>::iterator it = mActiveTransports.begin();
    for(; it != mActiveTransports.end(); ++it)
    {
        Transport::Ptr transport = it->second;
        transport->update();
    }
}

void MessageTransport::registerClient(const std::string& clientName, const std::string& clientDescription)
{
    ServiceLocator locator;
    if(mTransportEndpoints.empty())
    {
        throw std::runtime_error("MessageTransport '" + mAgentId.getName() + "': registerClient '" + clientName + "' failed since a locator (transport endpoint) does not exist. "
                "Please call 'activateTransport' first");
    } else {
        std::vector<fipa::services::ServiceLocation>::const_iterator cit = mTransportEndpoints.begin();
        for(; cit != mTransportEndpoints.end(); ++cit)
        {
            locator.addLocation(*cit);
        }
    }

    fipa::services::ServiceDirectoryEntry client(clientName, mServiceSignature, locator, clientDescription);
    LOG_INFO_S << "Register client: '" << client.toString() << "'";
    mpServiceDirectory->registerService(client);
}

void MessageTransport::deregisterClient(const std::string& clientName)
{
    mpServiceDirectory->deregisterService(clientName, fipa::services::ServiceDirectoryEntry::NAME);
}

std::string MessageTransport::serializeLetter(const fipa::acl::Letter& letter, const std::string& signature) const
{
    // Requires special adaptation
    // TODOS: allow special adapter integration
    if(signature == "JadeProxyAgent")
    {
        fipa::acl::Letter adaptedLetter = letter;
        fipa::acl::ACLMessage msg = letter.getACLMessage();
        std::string msgStr = fipa::acl::MessageGenerator::create(msg, fipa::acl::representation::STRING_REP);

        // Extra envelope
        // Altering encoding to string
        fipa::acl::ACLBaseEnvelope extraEnvelope;
        extraEnvelope.setACLRepresentation(fipa::acl::representation::STRING_REP);
        extraEnvelope.setPayloadLength(msgStr.length());

        // Add sender addresses from the service locations
        fipa::acl::AgentID sender = msg.getSender();
        std::vector<fipa::services::ServiceLocation>::const_iterator cit = mTransportEndpoints.begin();
        for(; cit != mTransportEndpoints.end(); ++cit)
        {
            sender.addAddress(cit->getServiceAddress());
        }
        extraEnvelope.setFrom(sender);
        adaptedLetter.addExtraEnvelope(extraEnvelope);

        // Modify the payload
        adaptedLetter.setPayload(msgStr);
        return fipa::acl::EnvelopeGenerator::create(adaptedLetter, fipa::acl::representation::XML);
    } else
    {
        return fipa::acl::EnvelopeGenerator::create(letter, fipa::acl::representation::BITEFFICIENT);
    }
}

} // end namespace message_transport
} // end namespace services
} // end namespace fipa
