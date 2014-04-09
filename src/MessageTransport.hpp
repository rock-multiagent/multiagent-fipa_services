#ifndef FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
#define FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP

#include <map>
#include <fipa_acl/fipa_acl.h>
#include <boost/function.hpp>
#include <stdexcept>

namespace fipa {
namespace agent_management {
    const std::string ONTOLOGY = "fipa-agent-management";
    const std::string INTERNAL_ERROR = "internal-error";
    const std::string INTERNAL_COMMUNICATION = "internal-communication";
}

namespace services {
namespace message_transport {

typedef std::string Type;
// TransportHandler needs to return the remaining list of agents it could not handle
typedef boost::function1<fipa::acl::AgentIDList, const fipa::acl::Letter&> TransportHandler;
typedef std::map<message_transport::Type, TransportHandler> TransportHandlerMap;
typedef std::vector<message_transport::Type> TransportPriorityList;

/**
 * \class MessageTransport
 * \brief Service responsible for handling FIPA message and managing the forwarding
 *  and relaying
 * \details
 * Usage example of the MessageTransport class
 * \verbatim
 #include <iostream>

 class CustomTransport
 {
 public:
     fipa::acl::AgentIDList deliverForwardLetter(const fipa::acl::Letter& letter)
     {
         fipa::acl::ACLMessage msg = letter.getACLMessage();
         std::cout << msg.getContent() << std::endl;

         // List of agents which could not be delivery to
         return fipa::acl::AgentIDList();
     }
 };

 #include <fipa_services/FipaServices.hpp>
 #include <boost/bind.hpp>

 using fipa::acl::message_transport;

 MessageTransport mts("my-mts");
 mts.registerTransport("default-internal-transport", boost::bind(&CustomTransport::deliverForwardLetter, this, _1));

 ...
 // Some letter from somewhere, here just manually constructed
 using fipa::acl;
 ACLMessage msg;
 msg.setSender( AgentID("my-agent") );
 Letter letter( msg, representation::BITEFFICIENT );
 ...
 mts.handle(letter);
 ...
 
 \endverbatim
 */
class MessageTransport
{
private:
    fipa::acl::AgentID mAgentId;

    TransportHandlerMap mTransportHandlerMap;
    TransportPriorityList mTransportPriorityList;

    // Representation which is used to exchange internal messages
    // default is bitefficient
    fipa::acl::representation::Type mRepresentation;

    /**
     * Stamp message for further delivery,
     * i.e. mark as handled by this message transport
     */
    void stamp(fipa::acl::Letter& msg) const;

    /**
     * Test whether a message has already been stamped by
     * this message transport
     */
    bool hasStamp(const fipa::acl::Letter& msg) const;

    /** 
     * Create an internal error message
     */
    fipa::acl::ACLMessage createInternalErrorMessage(const fipa::acl::ACLMessage& msg, const std::string& description) const;

    /**
     * Forward a letter to the next relay or final recipient
     * \return list of agents that could not be delivered to
     */
    fipa::acl::AgentIDList forward(fipa::acl::Letter& msg) const;

public:

    /**
     * \brief Constructor for MessageTransport
     * \param id Agent id for this message transport
     * for registered agents)
     */
    MessageTransport(const fipa::acl::AgentID& id);

    /**
     * Handle message, i.e. 
     * check forward -- create and internal ticket (based on the conversation id and 
     * interprete error messages correctly)
     */
    void handle(fipa::acl::Letter& msg);

    /**
     * Handle error, i.e. 
     * generate an error message from the original message
     */
    void handleError(const fipa::acl::Letter& msg) const;

    /**
     * Register a TransportHandler (the order of registration determines the priority)
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

    /**
     * Handle an internal communication, i.e. for exchange of information between two message transport
     * services
     *
     * \returns true if the processed letter was for internal process only, false if this message
     * needs to be processed further
     */
    bool handleInternalCommunication(const fipa::acl::Letter& letter);

    /** 
     * Set the internal message representation type
     */
    void setInternalMessageRepresentationType(const fipa::acl::representation::Type& representation) { mRepresentation = representation; }

    /**
     * Get type of the internal message representation
     */
    fipa::acl::representation::Type getInternalMessageRepresentationType() const { return mRepresentation; }
};

} // end namespace message_transport
} // end namespace services
} // end namespace fipa

#endif // FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
