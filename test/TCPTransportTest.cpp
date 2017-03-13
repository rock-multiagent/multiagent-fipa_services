#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/transports/Transport.hpp>
#include <boost/bind.hpp>
#include <fipa_acl/fipa_acl.h>
#include <fipa_acl/message_parser/envelope_parser.h>
#include <fipa_acl/message_generator/envelope_generator.h>

using namespace fipa::acl;
using namespace fipa::services::transports;

BOOST_AUTO_TEST_SUITE(transports_tcp)

void observer(const std::string& data)
{
    ACLEnvelope envelope;
    EnvelopeParser::parseData(data, envelope, representation::BITEFFICIENT);

    ACLMessage receivedMessage = envelope.getACLMessage();

    BOOST_TEST_MESSAGE("Observer received: " << receivedMessage.getContent());

    //BOOST_REQUIRE_MESSAGE(receivedMessage.getContent() == message.getContent(), "Content received '" << receivedMessage.getContent() << "' expected '" << envelope.getACLMessage().getContent() << "'");

}

BOOST_AUTO_TEST_CASE(tcp_transport_test)
{
    using namespace fipa::acl;
    using namespace fipa::services;

    Transport::Ptr transport = Transport::create(Transport::TCP);
    transport->registerObserver(observer);

    transport->start();

    transports::Address address = transport->getAddress("eth0");
    BOOST_TEST_MESSAGE("TCP Transport started on address: " << address.toString());

    OutgoingConnection::Ptr outConnection = transport->establishOutgoingConnection(address);

    try {
        transport->update();
    } catch(const std::runtime_error& e)
    {
        BOOST_REQUIRE_MESSAGE(false, "transport->update failed: " << e.what());
    }

    ACLMessage message;
    message.setSender( AgentID("test-sender") );
    message.setContent("test-content");
    ACLEnvelope envelope(message, representation::BITEFFICIENT);
    std::string encodedEnvelope = EnvelopeGenerator::create(envelope, representation::BITEFFICIENT);

    outConnection->send(encodedEnvelope);

    sleep(1);
    try {
        transport->update();
    } catch(const std::runtime_error& e)
    {
        BOOST_TEST_MESSAGE(e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()
