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
#include <string>

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
    LOG_DEBUG_S << "OutgoingConnection: Writing on socket: " << envStr;
    
    boost::system::error_code ec;
    // Send. No line break at the end!
    boost::asio::write(mClientSocket, boost::asio::buffer(envStr),
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
    LOG_INFO_S << "SocketTransport: now listening on " << getAddress().toString();
    boost::thread t(&SocketTransport::startAccept);
}

void SocketTransport::startAccept()
{
    try
    {
        while(true)
        {
            LOG_INFO_S << "SocketTransport: Waiting for a new connection.";
            boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(io_service);
            mAcceptor.accept(*socket);
            LOG_INFO_S << "SocketTransport: New conenction accepted. Starting reading thread.";
            // Read in a new thread, and accept blocking again.
            boost::thread t(&SocketTransport::read, socket);
        }
    }
    catch(std::exception & e)
    {
        LOG_ERROR_S << "SocketTransport: Error accepting connection: " << e.what();
    }
}

void SocketTransport::read(boost::asio::ip::tcp::socket* socket)
{
    try
    {
        const std::string splitter = "</envelope>";
        
        // Read until "</envelope>" has been read.
        boost::system::error_code  ec;
        char segment[4096];
        int charsRead;
        std::string buffer;
        
        while((charsRead = socket->read_some(boost::asio::buffer(segment), ec)) != 0)
        {
            if(ec && ec != boost::asio::error::eof)
            {
                throw std::runtime_error("Some error reading from socket: " + ec.message());
            }
            buffer += std::string(segment, 0, charsRead);
            
            // As long as a (new) whole envelope is included...
            std::string::size_type index;
            while((index = buffer.find(splitter, 0)) != std::string::npos)
            {
                // Include the end tag
                index += splitter.length();
                
                // cut leading and trailing whitespace, as this could be incompatible with payload_length
                // boost::algorithm::trim(envelopeString);

                // Now the envelope has been read completely
                LOG_DEBUG_S << "SocketTransport: Read into buffer: " << buffer;

                // Deserialize envelope
                fipa::acl::Letter envelope;
                
                if(!fipa::acl::EnvelopeParser::parseData(buffer, envelope, fipa::acl::representation::XML))
                {
                    throw std::runtime_error("Parsing the envelope failed.");
                }
                
                // Get payload length
                int msgLen = envelope.flattened().getPayloadLength();
                
                // Now read until the whole payload is included
                while(buffer.size() < index + msgLen
                    && (charsRead = socket->read_some(boost::asio::buffer(segment), ec)) != 0)
                {
                    if(ec && ec != boost::asio::error::eof)
                    {
                        throw std::runtime_error("Some error reading from socket: " + ec.message());
                    }
                    buffer += std::string(segment, 0, charsRead);
                }
                
                if(buffer.size() < index + msgLen) {
                    throw std::runtime_error("Stream only contains " + boost::lexical_cast<std::string>(buffer.size()) +
                    " chars. Expected " + boost::lexical_cast<std::string>(index + msgLen));
                    
                } else {
                    // Now set the envelope payload
                    envelope.setPayload(buffer.substr(index, msgLen));
                    LOG_DEBUG_S << "SocketTransport: Handling message payload: " << envelope.getPayload();
                    mpMts->handle(envelope);
                }
                
                // Remove envelope and message from the buffer
                buffer.erase(0, index + msgLen);
            }
        }
    }
    catch(std::exception& e)
    {
        LOG_WARN_S << "SocketTransport: Error forwarding envelope: " << e.what();
    }

    // Always close the connection in the end!
    try
    {
        LOG_INFO_S << "SocketTransport: Closing connection";
        socket->close();
        delete socket;
    }
    catch(boost::system::system_error& e)
    {
        LOG_ERROR_S << "SocketTransport: Could not close the socket: " << e.what();
    }
}


} // end namespace tcp
} // end namespace services
} // end namespace fipa
