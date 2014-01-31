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
    BOOST_TEST_MESSAGE("LISTENING on ip: " << node.getIP() << ":" << node.getPort());

    OutgoingConnection outConnection(node.getIP(), node.getPort());
    BOOST_REQUIRE_MESSAGE(node.accept(), "Node-Client connection established");

    sleep(10);
}
