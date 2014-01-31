#include <udt/udt.h>
#include <fipa_acl/fipa_acl.h>
#include <vector>
#include <queue>
#include <base/Logging.hpp>

namespace fipa {
namespace services {
namespace udt {

extern const uint32_t MAX_MESSAGE_SIZE_BYTES;

class Connection
{
protected:
    int16_t mPort;
    std::string mIP;
public:
    Connection();
    Connection(const std::string& ip, int16_t);

    int16_t getPort() const { return mPort; }
    std::string getIP() const { return mIP; }

    bool operator==(const Connection& other) const { return mPort == other.mPort && mIP == other.mIP; }
};

class OutgoingConnection : public Connection
{
    UDTSOCKET mSocket;

public:
    OutgoingConnection();

    OutgoingConnection(const std::string& ipaddress, int16_t port);

    ~OutgoingConnection();

    /**
     * Connect to ipaddress and port
     * \throws if connection cannot be established
     */
    void connect(const std::string& ipaddress, int16_t port);

    /**
     * Send message
     */
    void sendMessage(const std::string& data, int ttl = -1, bool inorder = true);
};

class IncomingConnection : public Connection
{
    UDTSOCKET mSocket;

public:
    IncomingConnection();
    ~IncomingConnection();
    IncomingConnection(const UDTSOCKET& socket, const std::string& ip, int16_t port);

    const UDTSOCKET& getSocket() { return mSocket; }

    int receiveMessage(char* buffer, size_t size);

    bool operator==(const IncomingConnection& connection) const { return Connection::operator==(connection); }
};

typedef std::vector<IncomingConnection> IncomingConnections;

class Node : public Connection
{
    UDTSOCKET mServerSocket;
    IncomingConnections mClients;

    std::queue<std::string> mMessageQueue;

    size_t mBufferSize;
    char* mpBuffer;
public:

    Node();
    ~Node();

    /**
     * Start listening on the given port
     */
    void listen(int32_t port = 12101, uint32_t maxClients = 50);

    /**
     * Accept new connections, returns true if a new client has been added
     */
    bool accept();

    // Update and read all sockets
    void update(bool readAll = false);
};


} // end namespace udt
} // end namespace services
} // end namespace fipa


