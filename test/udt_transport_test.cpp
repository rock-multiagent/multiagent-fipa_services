#include <boost/test/auto_unit_test.hpp>
#include <iostream>
#include <fipa_services/transports/udt/UDTTransport.hpp>
#include <boost/bind.hpp>

BOOST_AUTO_TEST_CASE(udt_transport_test)
{
    using namespace fipa::acl;
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

}
