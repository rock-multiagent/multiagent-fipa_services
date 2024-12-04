#ifndef FIPA_SERVICES_TRANSPORTS_UDT_INCOMING_CONNECTION_HPP
#define FIPA_SERVICES_TRANSPORTS_UDT_INCOMING_CONNECTION_HPP

#include <stdexcept>
#include <memory>
#include <udt/udt.h>
#include <base-logging/Logging.hpp>
#include <fipa_services/transports/Connection.hpp>

namespace fipa {
namespace services {
namespace transports {
namespace udt {

/**
 * \class IncomingConnection
 * \brief A unidirectional, incoming connection to receive incoming messages
 */
class IncomingConnection : public fipa::services::transports::Connection
{
    UDTSOCKET mSocket;

public:
    typedef std::shared_ptr<IncomingConnection> Ptr;

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

typedef std::vector<IncomingConnection::Ptr> IncomingConnections;

} // end namespace udt
} // end namespace transport
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_UDT_INCOMING_CONNECTION_HPP
