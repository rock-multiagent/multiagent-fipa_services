#include "SocketTransport.hpp"

#include <fipa_acl/message_parser/envelope_parser.h>
#include <fipa_acl/message_generator/envelope_generator.h>
#include <fipa_acl/message_generator/message_generator.h>

#include <base/Logging.hpp>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <stdexcept>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


namespace fipa {
namespace services {
namespace tcp {
// Class OutgoingConnection
OutgoingConnection::OutgoingConnection()
    : mClientSocket(SocketTransport::getIOService())
{}

OutgoingConnection::OutgoingConnection(const std::string& ipaddress, uint16_t port)
    : fipa::services::AbstractOutgoingConnection(ipaddress, port)
    , mClientSocket(SocketTransport::getIOService())
{
    connect(ipaddress, port);
}

OutgoingConnection::OutgoingConnection(const Address& address)
    : fipa::services::AbstractOutgoingConnection(address)
    , mClientSocket(SocketTransport::getIOService())
{
    connect(address.ip, address.port);
}

OutgoingConnection::~OutgoingConnection()
{
    // FIXME needed? socket close?
}

void OutgoingConnection::connect(const std::string& ipaddress, uint16_t port)
{
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(ipaddress), port);
    boost::system::error_code ec;
    
    
    mClientSocket.connect(endpoint, ec);
    if (ec)
    {
        LOG_DEBUG_S << "Error: " << ec.message();
        // An error occurred.
        throw boost::system::system_error(ec);
    }
}

void OutgoingConnection::sendLetter(acl::Letter& letter)
{
    if(!mClientSocket.is_open())
    {
        throw std::runtime_error("TCP socket is not open!");
    }
    
    fipa::acl::ACLMessage msg = letter.getACLMessage();
    std::string msgStr = fipa::acl::MessageGenerator::create(msg, fipa::acl::representation::STRING_REP);
    std::cout << "Msg Str: " << msgStr << std::endl;
    
    // Extra envelope
    // Altering encoding to string
    fipa::acl::ACLBaseEnvelope extraEnvelope;
    extraEnvelope.setACLRepresentation(fipa::acl::representation::STRING_REP);
    extraEnvelope.setPayloadLength(msgStr.length());
    // Add sender tcp address
    fipa::acl::AgentID sender = msg.getSender();
    sender.addAddress(SocketTransport::getAddress().toString());
    extraEnvelope.setFrom(sender);
    
    letter.addExtraEnvelope(extraEnvelope);
    // Modify the payload
    letter.setPayload(msgStr);
    
    std::string envStr = fipa::acl::EnvelopeGenerator::create(letter, fipa::acl::representation::XML);
    std::cout << "Env XML: " << envStr << std::endl;
    
    boost::system::error_code ec;
    // Send including a line break at the end
    boost::asio::write(mClientSocket, boost::asio::buffer(envStr + "\n"),
                        boost::asio::transfer_all(), ec);
    if (ec)
    {
        // An error occurred.
        throw boost::system::system_error(ec);
    }
}

    
// Class SocketTransport
boost::asio::io_service SocketTransport::io_service;
boost::asio::ip::tcp::acceptor SocketTransport::mAcceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
boost::asio::ip::tcp::socket SocketTransport::mSocket(io_service);
fipa::services::message_transport::MessageTransport* SocketTransport::mpMts = NULL;

boost::asio::io_service& SocketTransport::getIOService()
{
    return io_service;
}

fipa::services::Address SocketTransport::getAddress(const std::string& interfaceName)
{
    return Address(Transport::getLocalIPv4Address(), getPort(), "tcp");
}

int SocketTransport::getPort()
{
    return mAcceptor.local_endpoint().port();
}

void SocketTransport::startListening(fipa::services::message_transport::MessageTransport* mts)
{
    mpMts = mts;
    mAcceptor.listen();
    // FIXME
    LOG_WARN_S << "SocketTransport now listening on " << getAddress().toString();
    boost::thread t(&SocketTransport::startAccept);
}

void SocketTransport::startAccept()
{
    try
    {
        while(true)
        {
            mAcceptor.accept(mSocket);
            
            try
            {
                

                // Read message (EOF delimited)
                boost::asio::streambuf messageBuf;
                boost::system::error_code  ec;
                boost::asio::read_until(mSocket, messageBuf, EOF, ec);
                if(ec && ec != boost::asio::error::eof)
                {
                    throw std::runtime_error("Some error reading from socket: " + ec.message());
                }

                boost::asio::streambuf::const_buffers_type bufs = messageBuf.data();
                std::string messageString(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + messageBuf.size());
                
                // cut leading and trailing whitespace, as this could be incompatible with payload_length
                boost::algorithm::trim(messageString);

                std::cout << "GOT: " << messageString << std::endl;

                // Deserialize envelope
                fipa::acl::Letter envelope;
                
                if(fipa::acl::EnvelopeParser::parseData(messageString, envelope, fipa::acl::representation::XML))
                {
                    // success
                    std::cout << "success!" << std::endl;
                    std::cout << "The payload is:" << std::endl << envelope.getPayload() << std::endl;
                    mpMts->handle(envelope);
                }
            }
            catch(std::exception & e)
            {
                LOG_WARN("Error forwarding envelope: %s", e.what());
            }
        }
    }
    catch(std::exception & e)
    {
        LOG_ERROR("Error accepting connection: %s", e.what());
    }
}

} // end namespace tcp
} // end namespace services
} // end namespace fipa
