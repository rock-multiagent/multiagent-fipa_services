#include "SocketTransport.hpp"

#include <base/Logging.hpp>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <stdexcept>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

namespace fipa {
namespace services {
namespace message_transport {

SocketTransport::SocketTransport(MessageTransport* mts)
    : mAcceptor(mIo_service, tcp::endpoint(tcp::v4(), 0)) // FIXME
{
    mpMts = mts;
    boost::thread t(&SocketTransport::startAccept, this);
    
    // TODO ServiceLocator: publish proxy in avahi, with socket address and port
}


// TODO full getAddress
int SocketTransport::getPort()
{
    return mAcceptor.local_endpoint().port();
}

void SocketTransport::startAccept()
{
    while(true)
    {
        try
        {
            tcp::socket socket(mIo_service);
            mAcceptor.accept(socket);

            // Read message (EOF delimited)
            boost::asio::streambuf messageBuf;
            boost::system::error_code  ec;
            boost::asio::read_until(socket, messageBuf, EOF, ec);
            if(ec && ec != boost::asio::error::eof)
            {
                throw std::runtime_error("Some error reading from socket: " + ec.message());
            }

            boost::asio::streambuf::const_buffers_type bufs = messageBuf.data();
            std::string messageString(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + messageBuf.size());

            std::cout << "GOT: " << messageString << std::endl;

            // Deserialize message
            fipa::acl::ACLMessage message;
            //
            message.setSender(fipa::acl::AgentID("da0"));
            message.addReceiver(fipa::acl::AgentID("rock_agent"));
            message.setContent("This is the content.");
            //
            if(true)//fipa::acl::MessageParser::parseData(messageString, message, fipa::acl::representation::STRING_REP))
            {
                // success
                std::cout << "GOT again: " << message.toString() << std::endl;

                // Forward message with the MessageTransport
                fipa::acl::Letter letter (message, fipa::acl::representation::BITEFFICIENT);
                mpMts->handle(letter);
            }
            else
            {
                // FIXME ATM always parse errors with Jade messages
                std::cout << "parse error" << std::endl;
                throw std::runtime_error("Could not parse the sent message.");
            }
        }
        catch(std::exception & e)
        {
            LOG_WARN("Error forwarding message: %s", e.what());
        }
    }
}


fipa::acl::AgentIDList SocketTransport::deliverForwardLetter(const fipa::acl::Letter& letter) // TODO param ServiceLocator (oder mDistributedServiceDirectory im Konstruktor)
{
    fipa::acl::ACLMessage msg = letter.getACLMessage();
    
    //letter.flattened().getIntendedReceivers()

    tcp::socket socket(mIo_service);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 6789); // FIXME must the IP be configurable? avahi
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
        LOG_WARN("Could not connect socket: %s", e.what()); // TODO where does this go? It's not printed... export BASE_LOG_LEVEL=DEBUG/IMFO/WARN/ERROR
    }

    // List of agents which could not be delivered to
    return fipa::acl::AgentIDList(); // TODO not acurrate, but how do we determine that?
}

} // end namespace message_transport
} // end namespace services
} // end namespace fipa
