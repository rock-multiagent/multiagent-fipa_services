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

    namespace internal_communication {
        std::string CONNECTION_STATUS_UPDATE = "connection-status-update";
    }
}

namespace service {
namespace message_transport {

typedef std::string Type;
typedef boost::function1<bool, const fipa::acl::Letter&> TransportHandler;
typedef std::map<message_transport::Type, TransportHandler> TransportHandlerMap;
typedef std::vector<message_transport::Type> TransportPriorityList;
typedef std::map<std::string, std::vector<std::string> > TransportResponsabilities;

/**
 * \class MessageTransport
 * \brief Service responsible for handling FIPA message and managing the forwarding
 *  and relaying
 */
class MessageTransport
{
private:
    fipa::acl::AgentID mAgentId;

    TransportHandlerMap mTransportHandlerMap;
    TransportPriorityList mTransportPriorityList;

    // Map  clients to their associated transport services
    TransportResponsabilities mTransportResponsabilities;

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
     */
    bool forward(const fipa::acl::Letter& msg) const;

public:

    /**
     * \class MessageTransport
     * \param id Agent id for this message transport
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
     * Forward the connection status to allAgents, excepts the localAgents
     * since allAgents - localAgents == connected message transport services
     */
    void publishConnectionStatus(const std::vector<std::string>& allAgents, const std::vector<std::string>& localAgents);

    /**
     * Retrieve the message transport service that is responsible for the given client
     * throws if no transport is found
     */
    std::string getResponsibleMessageTransport(const std::string& agent) const;

    /**
     * Create a status update letter which can be used for internal communication
     */
    fipa::acl::ACLMessage createConnectionStatusUpdateMessage(const std::vector<std::string>& allAgents, const std::vector<std::string>& localAgents) const;

    /**
     * Handle an internal communication, i.e. for exchange of information between two message transport
     * services
     *
     * \returns true if the processed letter was for internal process only, false if this message
     * needs to be processed further
     */
    bool handleInternalCommunication(const fipa::acl::Letter& letter);



};

} // end namespace message_transport
} // end namespace service
} // end namespace fipa

#endif // FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
