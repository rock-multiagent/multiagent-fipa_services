#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/MessageTransport.hpp>

BOOST_AUTO_TEST_CASE(message_transport_internal_comm)
{
    using namespace fipa::acl;
    using namespace fipa::service::message_transport;

    MessageTransport messageTransport0(AgentID("mts-0"));
    MessageTransport messageTransport1(AgentID("mts-1"));

    std::vector<std::string> allAgents;
    std::vector<std::string> localAgents;

    allAgents.push_back("mts-1");
    allAgents.push_back("mts-2");
    allAgents.push_back("local-0");
    allAgents.push_back("local-1");

    localAgents.push_back("local-0");
    localAgents.push_back("local-1");

    ACLMessage internalMessage = messageTransport0.createConnectionStatusUpdateMessage(allAgents, localAgents);
    fipa::acl::Letter internalLetter(internalMessage, fipa::acl::representation::BITEFFICIENT);

    messageTransport1.handleInternalCommunication(internalLetter);
    
    BOOST_ASSERT(messageTransport1.getResponsibleMessageTransport("local-1") == "mts-0");

    messageTransport0.publishConnectionStatus(allAgents, localAgents);
}
