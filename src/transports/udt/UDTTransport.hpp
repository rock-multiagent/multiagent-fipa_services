#ifndef FIPA_SERVICES_TRANSPORTS_UDT_UDT_TRANSPORT_HPP
#define FIPA_SERVICES_TRANSPORTS_UDT_UDT_TRANSPORT_HPP

#include <vector>
#include <boost/shared_ptr.hpp>
#include <base/Logging.hpp>
#include <fipa_services/ErrorHandling.hpp>
#include <fipa_services/transports/Transport.hpp>
#include <fipa_services/transports/udt/IncomingConnection.hpp>

namespace fipa {
namespace services {
namespace transports {
namespace udt {

extern const uint32_t MAX_MESSAGE_SIZE_BYTES;

/**
 * \class UDTTransport
 * \brief A UDTTransport provides a server that allows to establish udt connection to transfer
 * fipa::acl::Letter
 * \details
 * The implementation is caching all incoming messages in an internal queue until they are
 * handle by calling UDTTransport::nextLetter
 * Very basic example for a receiver node:
 * \verbatim
 #include <fipa_services/transports/udt/UDTTransport.hpp>

 using namespace fipa::services;
 udt::UDTTransport node;

 // start node -- if no port is give it will be autoassigned and can be requested
 // by node.getAddress() afterwards.
 node.listen(2000);

 while(true)
 {
     // accept new connections
     node.accept();

     // Handle message transfers/reading new data
     node.update();
     while(node.hasLetter())
     {
         fipa::acl::Letter letter = node.nextLetter();
         // process letter
     }

     sleep(0.05);
 }
 \endverbatim
 * A very basic example for a sender:
 * \verbatim
 #include <fipa_services/transports/udt/UDTTransport.hpp>
 // the address has to be known
 udt::Adress address("127.0.0.1","2000");

 // --------- Sender ------------
 //
 udt::OutgoingConnection connection(address);
 fipa::acl::ACLMessage msg;
 msg.setContent("test-content");
 fipa::acl::Letter letter(msg, fipa::acl::representation::STRING_REP);
 connection.sendLetter(letter);
 \endverbatim
 */
class UDTTransport : public Transport
{
    static uint32_t msRefCount;

    UDTSOCKET mServerSocket;
    IncomingConnections mClients;

    Address mAddress;

    size_t mBufferSize;
    char* mpBuffer;

    /**
     * Accept new connections
     * \returns true if a new client has been added, false otherwise
     */
    bool accept();

    /**
     * Start listening on the given port
     * \param port Port that this node should listen on, if 0 then binding to any open port
     * \param maxClients number of maximum clients, default is 50
     */
    void listen(uint16_t port, uint32_t maxClients);
public:

    UDTTransport();
    ~UDTTransport();

    void start(uint16_t port = 0, uint32_t maxClients = 50);

    /**
     * Update and read all sockets
     * \param readAllMessages If set to true, update will return only when no further message can be read from any of the IncomingConnections. If set to false all connections will be checked only once for new messages
     */
    virtual void update(bool readAllMessages = true);

    /**
     * Get address of this node for a given interface
     * \param interfaceName name of the interface, default is eth0
     * \return address of the node
     */
    Address getAddress(const std::string& interfaceName = "eth0") const;

    /**
     * Create a udt based outgoing connection
     */
    virtual OutgoingConnection::Ptr establishOutgoingConnection(const Address& address);
};

} // end namespace udt
} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_UDT_UDT_TRANSPORT_HPP
