#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/transports/Transport.hpp>
#include <boost/bind.hpp>
#include <fipa_acl/fipa_acl.h>
#include <fipa_acl/message_parser/envelope_parser.h>
#include <fipa_acl/message_generator/envelope_generator.h>

using namespace fipa::acl;
using namespace fipa::services::transports;

BOOST_AUTO_TEST_SUITE(transports_udt)

void observer(const std::string& data)
{
    ACLEnvelope envelope;
    EnvelopeParser::parseData(data, envelope, representation::BITEFFICIENT);

    ACLMessage receivedMessage = envelope.getACLMessage();

    BOOST_TEST_MESSAGE("Observer received: " << receivedMessage.getContent());

    //BOOST_REQUIRE_MESSAGE(receivedMessage.getContent() == message.getContent(), "Content received '" << receivedMessage.getContent() << "' expected '" << envelope.getACLMessage().getContent() << "'");

}

BOOST_AUTO_TEST_CASE(address_test)
{
    std::string ip = "192.168.0.1";
    uint16_t port = 12391;

    Address address(ip, port);
    BOOST_REQUIRE_MESSAGE(address.toString() == "udt://192.168.0.1:12391", "Address toString '" << address.toString() << "'");
    Address addressFromString = Address::fromString( address.toString() );
    BOOST_REQUIRE_MESSAGE( addressFromString == address, "Address correctly generated and parsed again: '" << addressFromString.toString() << "'");
}

BOOST_AUTO_TEST_CASE(udt_transport_test)
{
    using namespace fipa::acl;
    using namespace fipa::services;

    Transport::Ptr udtTransport = Transport::create(Transport::UDT);
    udtTransport->registerObserver(observer);

    udtTransport->start();

    transports::Address address = udtTransport->getAddress("usb0");
    BOOST_TEST_MESSAGE("UDT Transport started on address: " << address.toString()); 

    OutgoingConnection::Ptr outConnection = udtTransport->establishOutgoingConnection(address);

    try {
        udtTransport->update();
    } catch(const std::runtime_error& e)
    {
        BOOST_REQUIRE_MESSAGE(false, "udtTransport->update failed: " << e.what());
    }

    ACLMessage message;
    message.setSender( AgentID("test-sender") );
    message.setContent("test-content");
    ACLEnvelope envelope(message, representation::BITEFFICIENT);
    std::string encodedEnvelope = EnvelopeGenerator::create(envelope, representation::BITEFFICIENT);

    outConnection->send(encodedEnvelope);

    sleep(1);
    try {
        udtTransport->update();
    } catch(const std::runtime_error& e)
    {
        BOOST_TEST_MESSAGE(e.what());
    } 

}

BOOST_AUTO_TEST_SUITE_END()
