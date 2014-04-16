#include "SocketTransport.hpp"

#include <base/Logging.hpp>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <stdexcept>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>

using boost::asio::ip::tcp;

namespace fipa {
namespace services {
namespace message_transport {

SocketTransport::SocketTransport(fipa::services::message_transport::MessageTransport* mts, fipa::services::DistributedServiceDirectory* dsd)
    : mAcceptor(mIo_service, tcp::endpoint(tcp::v4(), 0))
    , mpMts(mts)
    , mpDSD(dsd)
{
    boost::thread t(&SocketTransport::startAccept, this);
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

fipa::acl::AgentIDList SocketTransport::deliverForwardLetter(const fipa::acl::Letter& letter)
{
    fipa::acl::AgentIDList intendedReceivers = letter.flattened().getIntendedReceivers();
    fipa::acl::AgentIDList remainingReceivers;
    
    // TODO connection (caching) management: When sending messages instead of envelopes
    // and not saving where it was sent successfully, messages may be delivered duplicate.
    // TODO when sending letters:
    // fipa::acl::Letter updatedLetter = letter.createDedicatedEnvelope( fipa::acl::AgentID(intendedReceiverName) );
    
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
                    connectAndSend(letter, location.getServiceAddress());
                    // It worked, we do not need to try further locations
                    break;
                }
                catch(std::exception& e) 
                {
                    std::cout << e.what() << std::endl;
                    LOG_WARN("Could not connect socket: %s", e.what());
                    // export BASE_LOG_LEVEL=DEBUG/INFO/WARN/ERROR
                }
            }
        }
    }

    // List of agents which could not be delivered to
    return remainingReceivers;
}

void SocketTransport::connectAndSend(const acl::Letter& letter, std::string addressString)
{
    // TODO tcp address extraction and udt::Address::fromString nearly identical
    std::string address;
    uint16_t port;
    
    // address is in the format "tcp://Ip:Port
    boost::regex r("tcp://([^:]*):([0-9]{1,5})");
    boost::smatch what;
    if(boost::regex_match( addressString ,what,r))
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

    boost::asio::write(socket, boost::asio::buffer(msg.toString()),
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
