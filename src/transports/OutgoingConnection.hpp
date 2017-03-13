#ifndef FIPA_SERVICES_TRANSPORTS_OUTGOING_CONNECTION_HPP
#define FIPA_SERVICES_TRANSPORTS_OUTGOING_CONNECTION_HPP

#include <boost/shared_ptr.hpp>
#include <fipa_services/transports/Connection.hpp>

namespace fipa {
namespace services {
namespace transports {

/**
 * \class OutgoingConnection
 * \brief A unidirectional, outgoing connection that allows to send fipa letters to a receiver
 */
class OutgoingConnection : public Connection
{
public:
    typedef boost::shared_ptr<OutgoingConnection> Ptr;

    OutgoingConnection();
    OutgoingConnection(const std::string& ipaddress, uint16_t port);
    OutgoingConnection(const Address& address);
    virtual ~OutgoingConnection() {};

    /**
     * Connect to ipaddress and port
     * \param ipaddress IP as string
     * \param port Port number
     * \throws if connection cannot be established
     */
    virtual void connect(const std::string& ipaddress, uint16_t port) = 0;

    /**
     * Send data, i.e. encoded letters
     * \param data
     */
    virtual void send(const std::string& data) = 0;

    /**
     * Set the Time-to-Live for message in millisec
     */
    void setTTL(int ttl) { mTTL = ttl; }

    /**
     * Get the Time-to-Live
     */
    int getTTL() const { return mTTL; }

protected:
    int mTTL;
};

} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_OUTGOING_CONNECTION_HPP
