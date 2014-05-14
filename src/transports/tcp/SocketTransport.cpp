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

SocketTransport::SocketTransport(fipa::services::message_transport::MessageTransport* mts, fipa::services::DistributedServiceDirectory* dsd)
    : mAcceptor(mIo_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0))
    , mpMts(mts)
    , mpDSD(dsd)
    , mSocket(mIo_service)
{
    mAcceptor.listen();
    boost::thread t(&SocketTransport::startAccept, this);
}

Address SocketTransport::getAddress(const std::string& interfaceName)
{
    return Address(Transport::getLocalIPv4Address(), getPort(), "tcp");
}

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
            //tcp::socket socket(mIo_service);
            mAcceptor.accept(mSocket);

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

fipa::acl::AgentIDList SocketTransport::deliverForwardLetter(const fipa::acl::Letter& letter)
{
    fipa::acl::AgentIDList intendedReceivers = letter.flattened().getIntendedReceivers();
    fipa::acl::AgentIDList remainingReceivers;
    
    // TODO connection (caching) management: When sending messages instead of envelopes
    // and not saving where it was sent successfully, messages may be delivered duplicate.
    
    // Loop through all intended receivers
    BOOST_FOREACH(fipa::acl::AgentID id, intendedReceivers)
    {
        fipa::services::ServiceDirectoryList list = mpDSD->search(id.getName(), fipa::services::ServiceDirectoryEntry::NAME, false);
        if(list.empty()) {
            LOG_WARN("Receiver unknown: %s", id.getName().c_str());
        } else if(list.size() > 1) {
            LOG_WARN("Ambiguous receiver : %s", id.getName().c_str());
        } else {
            using namespace fipa::services;
            // Extract the service location
            ServiceDirectoryEntry serviceEntry = list.front();
            ServiceLocator locator = serviceEntry.getLocator();
            // Check all locations
            ServiceLocations locations = locator.getLocations();
            
            BOOST_FOREACH(ServiceLocation location, locations)
            {
                // TODO Explicitly check for this?
                if(location.getSignatureType() != "JadeProxyAgent")
                {
                    continue;
                }
                // Now, we have a tcp locator to connect to                          
                try 
                {
                    fipa::acl::Letter updatedLetter = letter.createDedicatedEnvelope( fipa::acl::AgentID(id.getName()) );
                    connectAndSend(updatedLetter, location.getServiceAddress());
                    // It worked, we do not need to try further locations
                    break;
                }
                catch(std::exception& e) 
                {
                    std::cout << e.what() << std::endl;
                    LOG_WARN("Could not connect socket: %s", e.what());
                }
            }
        }
    }

    // List of agents which could not be delivered to
    return remainingReceivers;
}

void SocketTransport::connectAndSend(acl::Letter& letter, const std::string& addressString)
{
    Address address = Address::fromString(addressString);
    
    std::string ip = address.ip;
    uint16_t port = address.port;
    
    boost::asio::ip::tcp::socket socket(mIo_service);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(ip), port);
    boost::system::error_code ec;
    
    socket.connect(endpoint, ec);
    if (ec)
    {
        // An error occurred.
        throw boost::system::system_error(ec);
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
    sender.addAddress(getAddress().toString());
    extraEnvelope.setFrom(sender);
    
    letter.addExtraEnvelope(extraEnvelope);
    // Modify the payload
    letter.setPayload(msgStr);
    
    std::string envStr = fipa::acl::EnvelopeGenerator::create(letter, fipa::acl::representation::XML);
    std::cout << "Env XML: " << envStr << std::endl;
    
    boost::asio::write(socket, boost::asio::buffer(envStr),
                        boost::asio::transfer_all(), ec);
    if (ec)
    {
        // An error occurred.
        throw boost::system::system_error(ec);
    }
}


} // end namespace tcp
} // end namespace services
} // end namespace fipa
