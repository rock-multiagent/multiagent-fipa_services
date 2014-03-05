#ifndef FIPA_SERVICES_TRANSPORTS_UDT_HPP
#define FIPA_SERVICES_TRANSPORTS_UDT_HPP

#include <udt/udt.h>
#include <vector>
#include <queue>
#include <boost/shared_ptr.hpp>
#include <base/Time.hpp>
#include <base/Logging.hpp>
#include <fipa_acl/fipa_acl.h>
#include <fipa_acl/message_generator/envelope_generator.h>
#include <fipa_acl/message_parser/envelope_parser.h>
#include <fipa_services/ErrorHandling.hpp>

namespace fipa {
namespace services {
namespace udt {

extern const uint32_t MAX_MESSAGE_SIZE_BYTES;

/**
 * \class Address
 * \brief Communication address specified by ip and port
 */
struct Address
{
    std::string ip;
    uint16_t port;

    Address() {}

    Address(const std::string& ip, uint16_t port);

    /**
     * Convert address to string
     * \return Address as string
     */
    std::string toString() const;

    /**
     * Create address from string
     * \return ArgumentError if address is malformatted
     */
    static Address fromString(const std::string& address);

    /**
     * Equals operator
     * \return True if equal, false otherwise
     */
    bool operator==(const Address& other) const;

    bool operator!=(const Address& other) const { return !this->operator==(other); }
};

/**
 * \class Connection
 * \brief A connection is defined by a communication address
 * \see Address
 */
class Connection
{
protected:
    uint16_t mPort;
    std::string mIP;
public:
    Connection();
    Connection(const std::string& ip, uint16_t port);
    Connection(const Address& address);

    uint16_t getPort() const { return mPort; }
    std::string getIP() const { return mIP; }

    Address getAddress() const { return Address(mIP, mPort); }

    bool operator==(const Connection& other) const { return mPort == other.mPort && mIP == other.mIP; }
};

/**
 * \class OutgoingConnection
 * \brief A unidirectional, outgoing udt connection that allows to send fipa letter to a receiver
 */
class OutgoingConnection : public Connection
{
    UDTSOCKET mSocket;

public:
    OutgoingConnection();

    OutgoingConnection(const std::string& ipaddress, uint16_t port);

    OutgoingConnection(const Address& address);

    ~OutgoingConnection();

    /**
     * Connect to ipaddress and port
     * \param ipaddress IP as string
     * \param port Port number
     * \throws if connection cannot be established
     */
    void connect(const std::string& ipaddress, uint16_t port);

    /**
     * Connect to a given address
     * \param address Connection address
     */
    void connect(const Address& address) { connect(address.ip, address.port); }

    /**
     * Send message
     * \param data string as data container
     * \param ttl Time to live for this message, default is unlimited
     * \param inorder Set to true if message you be received only in the right order
     */
    void sendData(const std::string& data, int ttl = -1, bool inorder = true) const;

    /**
     * Send fipa::acl::Letter
     * \param letter FIPA letter
     * \param ttl Time to live for this message, default is unlimited
     * \param inorder Set to true if message you be received only in the right order
     */
    void sendLetter(const fipa::acl::Letter& letter, int ttl = -1, bool inorder = true) const;
};

/**
 * \class IncomingConnection
 * \brief A unidirectional, incoming connection to receive incoming messages
 */
class IncomingConnection : public Connection
{
    UDTSOCKET mSocket;

public:
    IncomingConnection();
    ~IncomingConnection();
    IncomingConnection(const UDTSOCKET& socket, const std::string& ip, uint16_t port);

    /**
     * Get underlying UDTSOCKET
     * \return socket
     */
    const UDTSOCKET& getSocket() const { return mSocket; }

    /**
     * Equals operator based on the connection address
     * \return true if the connection address is the same for both connections, false otherwise
     */
    bool operator==(const IncomingConnection& connection) const { return Connection::operator==(connection); }

    /**
     * Receive a message
     * \param buffer to store message
     * \param size size of the buffer
     * \return number of received bytes
     */
    int receiveMessage(char* buffer, size_t size) const;
};

typedef boost::shared_ptr<IncomingConnection> IncomingConnectionPtr;
typedef std::vector<IncomingConnectionPtr> IncomingConnections;

/**
 * \class Node
 * \brief A Node provides a server that allows to establish udt connection to transfer
 * fipa::acl::Letter
 * \details
 * The implementation is caching all incoming messages in an internal queue until they are
 * handle by calling Node::nextLetter
 * Very basic example for a receiver node:
 * \verbatim
 #include <fipa_services/transports/udt/UDTTransport.hpp>

 using namespace fipa::services;
 udt::Node node;

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
class Node : public Connection
{
    UDTSOCKET mServerSocket;
    IncomingConnections mClients;

    std::queue<fipa::acl::Letter> mLetterQueue;
    fipa::acl::EnvelopeParser mEnvelopeParser;

    size_t mBufferSize;
    char* mpBuffer;
public:

    Node();
    ~Node();

    /**
     * Start listening on the given port
     * \param port Port that this node should listen on, if 0 then binding to any open port
     * \param maxClients number of maximum clients, default is 50
     */
    void listen(uint16_t port = 0, uint32_t maxClients = 50);

    /**
     * Accept new connections
     * \returns true if a new client has been added, false otherwise
     */
    bool accept();

    /**
     * Update and read all sockets
     * \param readAllMessages If set to true, update will return only when no further message can be read from any of the IncomingConnections. If set to false all connections will be checked only once for new messages
     */
    void update(bool readAllMessages = false);

    /**
     * Check wether this node has received new letters
     * \return true if letter is available
     */
    bool hasLetter() const { return !mLetterQueue.empty(); }

    /**
     * Get the next available letter. This will remove the letter from the internal queue
     * \return The oldest/front letter of the internal message queue
     */
    fipa::acl::Letter nextLetter();


    /**
     * Get address of this node for a given interface
     * \param interfaceName name of the interface, default is eth0
     * \return address of the node
     */
    Address getAddress(const std::string& interfaceName = "eth0");

};


} // end namespace udt
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_UDT_HPP
