#include "TCPTransport.hpp"

#include <fipa_acl/message_parser/envelope_parser.h>
#include <fipa_acl/message_generator/envelope_generator.h>
#include <fipa_acl/message_generator/message_generator.h>

#include <base-logging/Logging.hpp>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <stdexcept>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/read.hpp>
#include <string>

#include <fipa_services/transports/tcp/OutgoingConnection.hpp>

namespace fipa {
namespace services {
namespace transports {
namespace tcp {

// Class TCPTransport
boost::asio::io_service TCPTransport::msIOService;

boost::asio::io_service& TCPTransport::getIOService()
{
    return msIOService;
}

TCPTransport::TCPTransport()
    : Transport(Transport::TCP)
    , mAcceptor(msIOService)
{
}

TCPTransport::~TCPTransport()
{}

void TCPTransport::start(uint16_t port, uint32_t maxClients)
{
    listen(port);
}

void TCPTransport::update(bool readAllMessages)
{
    LOG_DEBUG_S << "Update transport";
    std::vector<SocketPtr> cleanupList;

    while(true)
    {
        LOG_DEBUG_S << "Accepting connections";
        accept();

        LOG_DEBUG_S << "Processing connections";

        std::vector<SocketPtr>::iterator it = mClients.begin();
        bool messageFound = false;

        for(; it != mClients.end(); ++it)
        {
            SocketPtr clientConnection = *it;
            try {
                if( read(clientConnection))
                {
                    messageFound = true;
                }
            } catch(...)
            {
                LOG_WARN_S << "Receiving data failed via TCP connection";
                cleanupList.push_back(clientConnection);
            }
        }

        if(!messageFound || !readAllMessages)
        {
            break;
        }
    }

    // Cleanup invalid sockets
    std::vector<SocketPtr>::iterator cit = cleanupList.begin();
    for(; cit != cleanupList.end(); ++cit)
    {
        std::vector<SocketPtr>::iterator it = std::find(mClients.begin(), mClients.end(), *cit);
        if(it != mClients.end())
        {
            mClients.erase(it);
        }
    }
}

Address TCPTransport::getAddress(const std::string& interfaceName) const
{
    return Address(Transport::getLocalIPv4Address(interfaceName), getPort(), "tcp");
}

uint16_t TCPTransport::getPort() const
{
    if(!mAcceptor.is_open())
    {
        // We cannot return anything useful
        return 0;
    }
    return mAcceptor.local_endpoint().port();
}

void TCPTransport::listen(uint16_t port)
{
    try
    {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        mAcceptor.open(endpoint.protocol());
        mAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        mAcceptor.bind(endpoint);
        mAcceptor.listen();
        mAcceptor.non_blocking(true);
        //LOG_INFO_S << "TCPTransport: now listening on port: " << getPort();
        //boost::thread t(&TCPTransport::accept, this);
    }
    catch(std::exception & e)
    {
        LOG_ERROR_S << "TCPTransport: Error startListening: " << e.what();
    }
}

void TCPTransport::accept()
{
    try
    {
        while(true)
        {
            LOG_INFO_S << "TCPTransport: Waiting for a new connection.";
            SocketPtr socket(new boost::asio::ip::tcp::socket(msIOService));
            mAcceptor.accept(*socket.get());

            LOG_INFO_S << "TCPTransport: New connection accepted. Caching connection";
            mClients.push_back(socket);
        }
    } catch(std::exception& e)
    {
        LOG_DEBUG_S << "TCPTransport: Error accepting connection: " << e.what();
    }
}

bool TCPTransport::read(SocketPtr socket)
{
    bool successfulRead = false;
    try
    {
        std::stringstream ss;

        boost::system::error_code error;
        boost::asio::streambuf response;

        uint32_t bytes = socket->available(error);
        if(bytes == 0)
        {
            return false;
        }

        // Read until EOF -- relying on other end closing the socket
        // writing data to buffer
        while(boost::asio::read(*socket.get(), response, boost::asio::transfer_at_least(1), error))
        {
            ss << &response;
        }

        if( error != boost::asio::error::eof)
        {
            throw boost::system::system_error(error);
        }
        successfulRead = true;
        // Notify about a new package
        notify(ss.str());

    } catch(std::exception& e)
    {
        LOG_WARN_S << "TCPTransport: Error reading data: " << e.what();
    }

    // Always close the connection
    try
    {
        LOG_INFO_S << "TCPTransport: Closing connection";
        socket->close();
    } catch(boost::system::system_error& e)
    {
        LOG_ERROR_S << "TCPTransport: Could not close the socket: " << e.what();
    }
    return successfulRead;
}

OutgoingConnection::Ptr TCPTransport::establishOutgoingConnection(const Address& address)
{
    return OutgoingConnection::Ptr(new tcp::OutgoingConnection(address));
}

} // end namespace tcp
} // end namespace transports
} // end namespace services
} // end namespace fipa
