#include "SocketTransport.hpp"

#include <base/Logging.hpp>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <stdexcept>

using boost::asio::ip::tcp;

namespace fipa {
namespace services {
namespace message_transport {

SocketTransport::SocketTransport()
{
    return; // TODO cons must return!
    try
    {
        tcp::acceptor acceptor(mIo_service, tcp::endpoint(tcp::v4(), 7890)); // FIXME
        while(true)
        {
            tcp::socket socket(mIo_service);
            acceptor.accept(socket);

            // Read message
            boost::asio::streambuf responseBuf;
            boost::asio::read_until(socket, responseBuf, "\n"); // This does not work
            // TODO handle EOF

            boost::asio::streambuf::const_buffers_type bufs = responseBuf.data();
            std::string response(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + responseBuf.size());

            std::cout << "GOT: " << response << std::endl;

        }
    }
    catch(std::exception& e)
    {

    }
}


fipa::acl::AgentIDList SocketTransport::deliverForwardLetter(const fipa::acl::Letter& letter)
{
    // TODO only forward to JADE agents??

    fipa::acl::ACLMessage msg = letter.getACLMessage();

    std::cout << "Sending: " << msg.toString() << std::endl;

    tcp::socket socket(mIo_service);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 6789); // FIXME must the IP be configurable?
    boost::system::error_code ec;

    try
    {
        socket.connect(endpoint, ec);
        if (ec)
        {
            // An error occurred.
            throw boost::system::system_error(ec);
        }

        boost::asio::write(socket, boost::asio::buffer(msg.toString()),
                           boost::asio::transfer_all(), ec);
        if (ec)
        {
            // An error occurred.
            throw boost::system::system_error(ec);
        }
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
        LOG_WARN("Could not connect socket: %s",e.what()); // TODO where does this go? It's not printed...
    }

    // List of agents which could not be delivered to
    return fipa::acl::AgentIDList();
}

} // end namespace message_transport
} // end namespace services
} // end namespace fipa
