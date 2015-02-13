#ifndef FIPA_SERVICES_TRANSPORTS_TCP_OUTGOING_CONNECTION_HPP
#define FIPA_SERVICES_TRANSPORTS_TCP_OUTGOING_CONNECTION_HPP

#include <boost/asio.hpp>
#include <fipa_services/transports/OutgoingConnection.hpp>

namespace fipa {
namespace services {
namespace transports {
namespace tcp {
    
/**
 * \class OutgoingConnection
 * \brief A unidirectional, outgoing tcp connection that allows to send fipa letter to a receiver
 */
class OutgoingConnection : public fipa::services::transports::OutgoingConnection
{
public:
    OutgoingConnection();
    OutgoingConnection(const std::string& ipaddress, uint16_t port);
    OutgoingConnection(const Address& address);

    /**
     * Connect to ipaddress and port
     * \param ipaddress IP as string
     * \param port Port number
     * \throws if connection cannot be established
     */
    void connect(const std::string& ipaddress, uint16_t port);

    /**
     * Send data
     * \param data
     */
    void send(const std::string& data);
    
private:
    boost::asio::ip::tcp::socket mClientSocket;
};    

} // end namespace tcp
} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_TCP_OUTGOING_CONNECTION_HPP
