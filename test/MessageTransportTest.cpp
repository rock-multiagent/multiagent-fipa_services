#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/MessageTransport.hpp>

using namespace std::placeholders;

class TestDelivery
{
public:
    bool deliverOrForwardLetterSuccess(const std::string& receiverName, const fipa::acl::Letter& letter)
    {
        BOOST_TEST_MESSAGE("Deliver message to " << receiverName << " with success: " << letter.getACLMessage().getContent());
        return true;
    }

    bool deliverOrForwardLetterFail(const std::string& receiverName, const fipa::acl::Letter& letter)
    {
        BOOST_TEST_MESSAGE("Deliver message to " << receiverName << " failed: " << letter.getACLMessage().getContent());
        return false;
    }
};

BOOST_AUTO_TEST_SUITE(message_transport)

BOOST_AUTO_TEST_CASE(internal_communication)
{
    using namespace fipa::acl;
    using namespace fipa::services::message_transport;
    using namespace fipa::services;

    ServiceDirectory::Ptr serviceDirectory(new ServiceDirectory());

    MessageTransport messageTransport0(AgentID("mts-0"), serviceDirectory);
    MessageTransport messageTransport1(AgentID("mts-1"), serviceDirectory);

    TestDelivery delivery;
    messageTransport0.registerMessageTransport("default-corba-transport", std::bind(&TestDelivery::deliverOrForwardLetterSuccess,delivery,_1,_2));
    messageTransport1.registerMessageTransport("default-corba-transport", std::bind(&TestDelivery::deliverOrForwardLetterFail,delivery,_1,_2));


    ACLMessage msg;
    msg.setSender(AgentID("sender"));
    msg.addReceiver(AgentID("receiver"));
    msg.setContent("Test content");
    ACLEnvelope env(msg, representation::BITEFFICIENT);

    messageTransport0.handle(env);
    messageTransport1.handle(env);
}

BOOST_AUTO_TEST_CASE(inter_service_communication)
{
    using namespace fipa::acl;
    using namespace fipa::services::message_transport;
    using namespace fipa::services;

    ServiceDirectory::Ptr serviceDirectory(new ServiceDirectory());

    MessageTransport messageTransport0(AgentID("mts-0"), serviceDirectory);
    MessageTransport messageTransport1(AgentID("mts-1"), serviceDirectory);

    messageTransport0.activateTransport(transports::Transport::UDT);
    messageTransport1.activateTransport(transports::Transport::UDT);

    AgentID mt0Client("mt0-client");
    AgentID mt1Client("mt1-client");

    // Setting up the proper entries in the service directory
    {
        std::string description = "Message client of " + messageTransport0.getAgentID().getName();
        messageTransport0.registerClient(mt0Client.getName(), description);
    }

    {
        std::string description = "Message client of " + messageTransport1.getAgentID().getName();
        messageTransport1.registerClient(mt1Client.getName(), description);
    }

    TestDelivery delivery;
    messageTransport0.registerMessageTransport("default-corba-transport", std::bind(&TestDelivery::deliverOrForwardLetterSuccess,delivery,_1,_2));
    messageTransport1.registerMessageTransport("default-corba-transport", std::bind(&TestDelivery::deliverOrForwardLetterSuccess,delivery,_1,_2));

    ACLMessage msg;
    msg.setSender(mt0Client);
    msg.addReceiver(mt1Client);
    msg.setContent("Test Content");
    ACLEnvelope env(msg, representation::BITEFFICIENT);
    messageTransport0.handle(env);
    //messageTransport1.handle(env);


    for(int i = 0; i < 10; ++i)
    {
        BOOST_TEST_MESSAGE("Trigger");
        messageTransport0.trigger();
        messageTransport1.trigger();
        sleep(0.5);
    }
}

BOOST_AUTO_TEST_CASE(inter_service_communication_fail)
{
    using namespace fipa::acl;
    using namespace fipa::services::message_transport;
    using namespace fipa::services;

    ServiceDirectory::Ptr serviceDirectory(new ServiceDirectory());

    MessageTransport messageTransport0(AgentID("mts-0"), serviceDirectory);
    MessageTransport messageTransport1(AgentID("mts-1"), serviceDirectory);

    messageTransport0.activateTransport(transports::Transport::UDT);
    messageTransport1.activateTransport(transports::Transport::UDT);

    AgentID mt0Client("mt0-client");
    AgentID mt1Client("mt1-client");

    // Setting up the proper entries in the service directory
    {
        std::string description = "Message client of " + messageTransport0.getAgentID().getName();
        messageTransport0.registerClient(mt0Client.getName(), description);
    }

    {
        std::string description = "Message client of " + messageTransport1.getAgentID().getName();
        messageTransport1.registerClient(mt1Client.getName(), description);
    }

    TestDelivery delivery;
    messageTransport0.registerMessageTransport("default-corba-transport", std::bind(&TestDelivery::deliverOrForwardLetterSuccess,delivery,_1,_2));
    messageTransport1.registerMessageTransport("default-corba-transport", std::bind(&TestDelivery::deliverOrForwardLetterFail,delivery,_1,_2));

    ACLMessage msg;
    msg.setSender(mt0Client);
    msg.addReceiver(mt1Client);
    msg.setContent("Test Content");
    ACLEnvelope env(msg, representation::BITEFFICIENT);
    messageTransport0.handle(env);
    //messageTransport1.handle(env);


    for(int i = 0; i < 10; ++i)
    {
        BOOST_TEST_MESSAGE("Trigger");
        messageTransport0.trigger();
        messageTransport1.trigger();
        sleep(0.5);
    }
}
BOOST_AUTO_TEST_SUITE_END()
