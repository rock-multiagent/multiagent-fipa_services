#ifndef FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
#define FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP

#include <map>
#include <fipa_acl/fipa_acl.h>
#include <boost/function.hpp>
#include <stdexcept>
#include <fipa_services/transports/Transport.hpp>
#include <fipa_services/transports/Configuration.hpp>
#include <fipa_services/ServiceDirectory.hpp>

namespace fipa {
namespace agent_management {
    const std::string ONTOLOGY = "fipa-agent-management";
    const std::string INTERNAL_ERROR = "internal-error";
    const std::string INTERNAL_COMMUNICATION = "internal-communication";
}

namespace services {
namespace message_transport {

/// MessageTransportHandler needs to return success of the delivery
typedef boost::function2<bool, const std::string&, const fipa::acl::Letter&> MessageTransportHandler;

typedef std::map<std::string, MessageTransportHandler> MessageTransportHandlerMap;
typedef std::vector<std::string> MessageTransportPriorityList;

/**
 * \class MessageTransport
 * \brief Service responsible for handling FIPA message and managing the forwarding
 *  and relaying
 * \details
 * Usage example of the MessageTransport class
 * \verbatim
 #include <iostream>

 class CustomMessageTransport
 {
 public:
     bool deliverForwardLetter(const std::string& receiverName, const fipa::acl::Letter& letter)
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
 mts.registerMessageTransport("custom-transport", boost::bind(&CustomTransport::deliverForwardLetter, this, _1,_2));

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
    ServiceDirectory::Ptr mpServiceDirectory;

    MessageTransportHandlerMap mMessageTransportHandlerMap;
    MessageTransportPriorityList mMessageTransportPriorityList;

    mutable std::map<transports::Transport::Type, transports::Transport::Ptr> mActiveTransports;
    std::vector<transports::Configuration> mTransportConfigurations;

    /// The local endpoints of the active transports after activation
    std::vector<fipa::services::ServiceLocation> mTransportEndpoints;

    // Representation which is used to exchange internal messages
    // default is bitefficient
    fipa::acl::representation::Type mRepresentation;

    /// Set of accepted service signatures
    /// This allow to filter out services that try to
    /// connect to this special MTS service
    std::string mServiceSignature;
    std::set<std::string> mAcceptedServiceSignatures;

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
     * Forward a letter locally using the (custom) registered MessageTransportHandlers
     * \return true if the delivery suceeded, false otherwise 
     */
    bool localForward(const std::string& receiverName, const fipa::acl::Letter& msg) const;

    /**
     * Forward a letter using the buildin transports
     */
    fipa::acl::AgentIDList forward(const fipa::acl::Letter& letter) const;

    void forward(const std::string& receiverName, const ServiceLocation& location, const fipa::acl::Letter& letter) const;

    /**
     * Check is the transport that corresponds to the given protocol name has been activated
     * \return true, if the transport has been activated, false otherwise
     */
    bool hasActiveTransport(const std::string& protocol) const;

    /**
     * Test is this is a local service location, i.e. corresponds to a local
     * endpoint
     * \return true if the service is local, false otherwise
     */
    bool isLocal(const ServiceLocation& local) const;

    /**
     * Handle incoming data from the transports
     */
    void handleData(const std::string& data);

    /**
     * Remove a receiver from the a list of receivers
     */
    static void removeFromList(const fipa::acl::AgentID& receiver, fipa::acl::AgentIDList& receiversList);

    /**
     * Set the endpoints of the inbuilt transports based on the IP of active
     * interfaces
     * \throws if an active interface cannot be found
     */
    void cacheTransportEndpoints(fipa::services::transports::Transport::Ptr transport);

public:

    /**
     * \brief Constructor for MessageTransport
     * \param id Agent id for this message transport
     * for registered agents)
     */
    MessageTransport(const fipa::acl::AgentID& id, ServiceDirectory::Ptr serviceDirectory);

    fipa::acl::AgentID getAgentID() const { return mAgentId; }

    void configure(const std::vector<transports::Configuration>& configurations) { mTransportConfigurations = configurations; }

    /**
     * Activate the given transports
     * \param list of transports that shall be activated -- names need to
     * corresponding to inbuilt transports
     * \see fipa::services::transports::Transport
     * \throw std::runtime_error when a transport has already been activated
     */
    void activateTransports(const std::vector<std::string>& transports);

    /**
     * Activate transports as indicated by flags,e.g.
     * \verbatim
         MessageTransport mt = MessageTransport( AgentID("agent"), ServiceDirectory::Ptr( new ServiceDirectory()));
         mt.activateTransports( transports::Transport::UDT | transports::Transport::TCP );
     \endverbatim
     * \throw std::runtime_error when a transport has already been activated
     */
    void activateTransports(transports::Transport::Type flags);

    /**
     * Activate a transport given by type
     * This start the transport
     * \throw std::runtime_error when a transport has already been activated
     */
    void activateTransport(transports::Transport::Type type);

    /**
     * Get the endpoints of the inbuilt transports
     * \return ServiceLocations of active transports
     */
    std::vector<fipa::services::ServiceLocation> getTransportEndpoints() const;

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
     * for local (!) delivery
     * \throw Duplicate
     */
    void registerMessageTransport(const std::string& id, MessageTransportHandler handler);

    /**
     * Deregister a MessageTransportHandler
     * \throw Duplicate
     */
    void deregisterMessageTransport(const std::string& id);

    /**
     * Modify existing MessageTransportHandler
     * \throw Duplicate
     */
    void modifyMessageTransport(const std::string& id, MessageTransportHandler handler);

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
     * \param message representation
     */
    void setInternalMessageRepresentationType(const fipa::acl::representation::Type& representation) { mRepresentation = representation; }

    /**
     * Get type of the internal message representation
     * \return the message representation to be used
     */
    fipa::acl::representation::Type getInternalMessageRepresentationType() const { return mRepresentation; }

    /**
     * Trigger the MessageTransport and all associated underlying transports to
     * process messages and establishing connections
     */
    void trigger();

    /**
     * Get the service signature of the MessageTransport
     */
    std::string getServiceSignature() const { return mServiceSignature; }

    /**
     * Service signature are part of the locator which is stored in the
     * ServiceDirectory.
     * The MessageTransport handles only services with service signature that
     * are 'accepted'.
     * This adds the given signature to the whitelist of accepted signatures.
     * \param signature Service signature
     */
    void addAcceptedServiceSignature(const std::string& signature) { mAcceptedServiceSignatures.insert(signature); }

    /**
     * Get the service directory that is associated with this MessageTransport
     * \return service directory
     */
    ServiceDirectory::Ptr getServiceDirectory() { return mpServiceDirectory; }

};

} // end namespace message_transport
} // end namespace services
} // end namespace fipa

#endif // FIPA_SERVICE_MESSAGE_TRANSPORT_SERVICE_HPP
