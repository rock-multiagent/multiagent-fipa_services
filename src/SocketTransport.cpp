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
#include <ifaddrs.h>

using boost::asio::ip::tcp;

namespace fipa {
namespace services {
namespace message_transport {

SocketTransport::SocketTransport(fipa::services::message_transport::MessageTransport* mts, fipa::services::DistributedServiceDirectory* dsd)
    : mAcceptor(mIo_service, tcp::endpoint(tcp::v4(), 0))
    , mpMts(mts)
    , mpDSD(dsd)
    , mSocket(mIo_service)
{
    mAcceptor.listen();
    boost::thread t(&SocketTransport::startAccept, this);
}

std::string SocketTransport::getAddress(const std::string& interfaceName)
{
    return "tcp://" + getIP(interfaceName) + ":" + boost::lexical_cast<std::string>(getPort());
}

// TODO UDTTransport Address Node::getAddress is identical. Merge!
std::string SocketTransport::getIP(const std::string& interfaceName)
{
    struct ifaddrs* interfaces;

    if(0 == getifaddrs(&interfaces))
    {
        struct ifaddrs* interface;
        for(interface = interfaces; interface != NULL; interface = interface->ifa_next)
        {
            if(interface->ifa_addr->sa_family == AF_INET)
            {
                // Match
                if(interfaceName == std::string(interface->ifa_name))
                {
                    struct sockaddr_in* sa = (struct sockaddr_in*) interface->ifa_addr;
                    char* addr = inet_ntoa(sa->sin_addr);
                    std::string ip = std::string(addr);

                    freeifaddrs(interfaces);

                    return ip;
                }
            }
        }
    }

    freeifaddrs(interfaces);
    throw std::runtime_error("fipa::services::udt::Node: could not get interface address of '" + interfaceName + "'");
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

            // Deserialize message
            fipa::acl::ACLMessage message;
            fipa::acl::Letter envelope;
            
            if(fipa::acl::EnvelopeParser::parseData(messageString, envelope, fipa::acl::representation::XML))
            {
                // success
                std::cout << "success!" << std::endl;
                std::cout << "The payload is:" << std::endl << envelope.getPayload() << std::endl;
                mpMts->handle(envelope);
            }
            else
            {
                if(fipa::acl::MessageParser::parseData(messageString, message, fipa::acl::representation::STRING_REP))
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
        }
        catch(std::exception & e)
        {
            LOG_WARN("Error forwarding message: %s", e.what());
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
    // TODO tcp address extraction and udt::Address::fromString nearly identical
    std::string address;
    uint16_t port;
    
    // address is in the format "tcp://Ip:Port
    boost::regex r("tcp://([^:]*):([0-9]{1,5})");
    boost::smatch what;
    if(boost::regex_match(addressString, what, r))
    {
        address = std::string(what[1].first, what[1].second);
        port = atoi( std::string(what[2].first, what[2].second).c_str() );
    } else {
        throw std::runtime_error("address '" + addressString + "' malformatted");
    }
    
    tcp::socket socket(mIo_service);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(address), port);
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
    sender.addAddress(getAddress());
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


} // end namespace message_transport
} // end namespace services
} // end namespace fipa
