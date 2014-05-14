#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/transports/udt/UDTTransport.hpp>
#include <boost/bind.hpp>

BOOST_AUTO_TEST_CASE(udt_transport_test)
{
    using namespace fipa::acl;
    using namespace fipa::services;
    using namespace fipa::services::udt;

    Node node;
    node.listen();
    BOOST_TEST_MESSAGE("Node is listening on ip: " << node.getIP() << ":" << node.getPort());

    OutgoingConnection outConnection(node.getIP(), node.getPort());
    BOOST_REQUIRE_MESSAGE(node.accept(), "Node-Client connection established");

    ACLMessage message;
    message.setSender( AgentID("test-sender") );
    message.setContent("test-content");

    ACLEnvelope envelope(message, representation::BITEFFICIENT);
    outConnection.sendLetter(envelope);
    sleep(1);

    node.update();
    BOOST_REQUIRE_MESSAGE(node.hasLetter(), "Node received data");

    Letter letter = node.nextLetter();
    ACLMessage receivedMessage = letter.getACLMessage();

    BOOST_REQUIRE_MESSAGE(receivedMessage.getContent() == message.getContent(), "Content received '" << receivedMessage.getContent() << "' expected '" << envelope.getACLMessage().getContent() << "'");

    std::string ip = "192.168.0.1";
    uint16_t port = 12391;

    Address address(ip, port);
    BOOST_REQUIRE_MESSAGE(address.toString() == "udt://192.168.0.1:12391", "Address toString '" << address.toString() << "'");
    Address addressFromString = Address::fromString( address.toString() );
    BOOST_REQUIRE_MESSAGE( addressFromString == address, "Address correctly generated and parsed again: '" << addressFromString.toString() << "'");
 
    Address nodeAddress = node.getAddress("eth0");
    BOOST_TEST_MESSAGE("Node on eth0 is: '" << nodeAddress.toString() << "'");
}
