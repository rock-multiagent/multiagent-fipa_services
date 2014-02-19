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

struct Address
{
    std::string ip;
    uint16_t port;

    Address() {}

    Address(const std::string& ip, uint16_t port);

    /**
     * Convert address to string
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
     * \throws if connection cannot be established
     */
    void connect(const std::string& ipaddress, uint16_t port);

    /**
     * Connect to a given address
     */
    void connect(const Address& address) { connect(address.ip, address.port); }

    /**
     * Send message
     * \param data string as data container
     * \param ttl Time to live for this message
     * \param inorder Set to true if message you be received only in the right order
     */
    void sendData(const std::string& data, int ttl = -1, bool inorder = true) const;

    /**
     * Send fipa::acl::Letter
     * \param letter FIPA letter
     * \param ttl Time to live for this message
     * \param inorder Set to true if message you be received only in the right order
     */
    void sendLetter(const fipa::acl::Letter& letter, int ttl = -1, bool inorder = true) const;
};

class IncomingConnection : public Connection
{
    UDTSOCKET mSocket;

public:
    IncomingConnection();
    ~IncomingConnection();
    IncomingConnection(const UDTSOCKET& socket, const std::string& ip, uint16_t port);

    const UDTSOCKET& getSocket() const { return mSocket; }

    bool operator==(const IncomingConnection& connection) const { return Connection::operator==(connection); }

    int receiveMessage(char* buffer, size_t size) const;
};

typedef boost::shared_ptr<IncomingConnection> IncomingConnectionPtr;
typedef std::vector<IncomingConnectionPtr> IncomingConnections;

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
     */
    void listen(uint16_t port = 0, uint32_t maxClients = 50);

    /**
     * Accept new connections, returns true if a new client has been added
     */
    bool accept();

    // Update and read all sockets
    void update(bool readAll = false);

    /**
     * \return true if letter is available
     */
    bool hasLetter() const { return !mLetterQueue.empty(); }

    /**
     * Get the next letter
     * \return next letter
     */
    fipa::acl::Letter nextLetter();


    /**
     * Get address of this node
     */
    Address getAddress(const std::string& interfaceName);

};


} // end namespace udt
} // end namespace services
} // end namespace fipa


