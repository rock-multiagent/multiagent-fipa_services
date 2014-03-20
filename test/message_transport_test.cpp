#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/MessageTransport.hpp>
#include <boost/bind.hpp>

class TestDelivery
{
public:
    fipa::acl::AgentIDList deliverOrForwardLetterSuccess(const fipa::acl::Letter& letter)
    {
        return fipa::acl::AgentIDList();
    }

    fipa::acl::AgentIDList deliverOrForwardLetterFail(const fipa::acl::Letter& letter)
    {
        return letter.flattened().getTo();
    }
};

BOOST_AUTO_TEST_CASE(message_transport_internal_comm)
{
    using namespace fipa::acl;
    using namespace fipa::services::message_transport;

    MessageTransport messageTransport0(AgentID("mts-0"));
    MessageTransport messageTransport1(AgentID("mts-1"));

    TestDelivery delivery;

    messageTransport0.registerTransport("default-corba-transport", boost::bind(&TestDelivery::deliverOrForwardLetterSuccess,delivery,_1));
    messageTransport1.registerTransport("default-corba-transport", boost::bind(&TestDelivery::deliverOrForwardLetterFail,delivery,_1));


    ACLMessage msg;
    msg.setSender(AgentID("sender"));
    msg.addReceiver(AgentID("receiver"));
    msg.setContent("Test content");
    ACLEnvelope env(msg, representation::BITEFFICIENT);

    messageTransport0.handle(env);
    messageTransport1.handle(env);
}
