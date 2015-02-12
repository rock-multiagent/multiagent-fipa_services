#ifndef FIPA_SERVICES_TRANSPORTS_UDT_OUTGOING_CONNECTION_HPP
#define FIPA_SERVICES_TRANSPORTS_UDT_OUTGOING_CONNECTION_HPP

#include <udt/udt.h>
#include <stdexcept>
#include <base/Logging.hpp>
#include <fipa_services/transports/OutgoingConnection.hpp>

namespace fipa {
namespace services {
namespace transports {
namespace udt {

/**
 * \class OutgoingConnection
 * \brief A unidirectional, outgoing udt connection that allows to send fipa letter to a receiver
 */
class OutgoingConnection : public fipa::services::transports::OutgoingConnection
{
    UDTSOCKET mSocket;

public:
    OutgoingConnection();

    OutgoingConnection(const std::string& ipaddress, uint16_t port);

    OutgoingConnection(const Address& address);

    virtual ~OutgoingConnection();

    /**
     * Connect to ipaddress and port
     * \param ipaddress IP as string
     * \param port Port number
     * \throws if connection cannot be established
     */
    virtual void connect(const std::string& ipaddress, uint16_t port);
    
    /**
     * Send message
     * \param data string as data container
     * \param ttl Time to live for this message, default is unlimited
     * \param inorder Set to true if message you be received only in the right order
     */
    void sendData(const std::string& data, int ttl = -1, bool inorder = true) const;

    /**
     * Send data via this connection
     * \throw std::runtime_error if transport failed
     */
    virtual void send(const std::string& data); 
};
} // end namespace udt
} // end namespace transport
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_UDT_OUTGOING_CONNECTION_HPP
