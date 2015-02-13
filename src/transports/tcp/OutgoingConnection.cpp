#include "OutgoingConnection.hpp"
#include "TCPTransport.hpp"

namespace fipa {
namespace services {
namespace transports {
namespace tcp {

// Class OutgoingConnection
OutgoingConnection::OutgoingConnection()
    : fipa::services::transports::OutgoingConnection()
    , mClientSocket(TCPTransport::getIOService())
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, uint16_t port)
    : fipa::services::transports::OutgoingConnection(ipaddress, port)
    , mClientSocket(TCPTransport::getIOService())
{
    connect(ipaddress, port);
}

OutgoingConnection::OutgoingConnection(const Address& address)
    : fipa::services::transports::OutgoingConnection(address)
    , mClientSocket(TCPTransport::getIOService())
{
    connect(address.ip, address.port);
}

void OutgoingConnection::connect(const std::string& ipaddress, uint16_t port)
{
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(ipaddress), port);
    boost::system::error_code ec;
    
    mClientSocket.connect(endpoint, ec);
    if (ec)
    {
        LOG_DEBUG_S << "OutgoingConnection: Error: " << ec.message();
        // An error occurred.
        throw boost::system::system_error(ec);
    }
}

void OutgoingConnection::send(const std::string& data)
{
    if(!mClientSocket.is_open())
    {
        // try to reconnect to know endpoint
        connect(getIP(), getPort());
    }
   
    boost::system::error_code ec;

    // Send and close socket after writing to mark end of message
    boost::asio::write(mClientSocket, boost::asio::buffer(data),
                        boost::asio::transfer_all(), ec);
    mClientSocket.close();

    if (ec)
    {
        // An error occurred.
        throw boost::system::system_error(ec);
    }
}

} // end namespace tcp
} // end namespace transports
} // end namespace services
} // end namespace fipa

