#include "Transport.hpp"

#include <base/Logging.hpp>

#include <stdexcept>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <fipa_services/transports/udt/UDTTransport.hpp>
#include <fipa_services/transports/tcp/SocketTransport.hpp>

namespace fipa {
namespace services {

// Address
Address::Address(const std::string& ip, uint16_t port, const std::string& protocol)
    : ip(ip)
    , port(port)
    , protocol(protocol)
{}

std::string Address::toString() const
{
    return protocol + "://" + ip + ":" + boost::lexical_cast<std::string>(port);
}

Address Address::fromString(const std::string& addressString)
{
    boost::regex r("([^:]*)://([^:]*):([0-9]{1,5})"); 
    boost::smatch what;
    if(boost::regex_match(addressString, what, r))
    {
        std::string protocol(what[1].first, what[1].second);
        std::string address(what[2].first, what[2].second);
        uint16_t port = atoi( std::string(what[3].first, what[3].second).c_str() );
        return Address(address, port, protocol);
    } else {
        throw std::runtime_error("address '" + addressString + "' malformatted");
    }
}

bool Address::operator==(const Address& other) const
{
    return this->ip == other.ip && this->port == other.port;
}

// Connection
Connection::Connection()
    : mAddress("0.0.0.0", 0)
{}

Connection::Connection(const std::string& ip, uint16_t port, const std::string& protocol)
    : mAddress(ip, port, protocol)
{}

Connection::Connection(const Address& address)
    : mAddress(address)
{}

// AbstractOutgoingConnection
AbstractOutgoingConnection::AbstractOutgoingConnection()
{}

AbstractOutgoingConnection::AbstractOutgoingConnection(const std::string& ipaddress, uint16_t port)
    : Connection(ipaddress, port)
{
}

AbstractOutgoingConnection::AbstractOutgoingConnection(const Address& address)
    : Connection(address)
{
}

// Transport
std::vector<std::string> Transport::additionalAcceptedSignatureTypes = boost::assign::list_of
    ("JadeProxyAgent");

std::string Transport::getLocalIPv4Address(const std::string& interfaceName)
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
    throw std::runtime_error("fipa::services::Transport: could not get interface address of '" + interfaceName + "'");
}

Transport::Transport(const std::string& name, fipa::services::DistributedServiceDirectory* dsd, const std::vector<fipa::services::ServiceLocation> mServiceLocations)
    : mpDSD(dsd)
    , mServiceLocations(mServiceLocations)
    , name(name)
{
    // Create a map of outgoing connections for each protocol that is to be used.
    // Also save the service signature types
    BOOST_FOREACH(fipa::services::ServiceLocation location, mServiceLocations)
    {
        std::string protocol = getProtocolOfServiceLocation(location);
        mOutgoingConnections[protocol] = std::map<std::string, fipa::services::AbstractOutgoingConnection*>();
        mServiceSignaturesTypes[protocol] = location.getSignatureType();
    }  
}

std::string Transport::getProtocolOfServiceLocation(const ServiceLocation& location)
{
    return Address::fromString(location.getServiceAddress()).protocol;
}

const std::vector<fipa::services::ServiceLocation> Transport::getServiceLocations() const
{
    return mServiceLocations;
}

fipa::acl::AgentIDList Transport::deliverOrForwardLetter(const fipa::acl::Letter& letter)
{
    using namespace fipa::acl;

    ACLBaseEnvelope envelope = letter.flattened();
    AgentIDList receivers = envelope.getIntendedReceivers();
    AgentIDList::const_iterator rit = receivers.begin();

    AgentIDList remainingReceivers = receivers;

    // For each intended receiver, try to deliver.
    // If it is a local client, deliver locally, otherwise forward to the known locator, which is an MTS in this context
    for(; rit != receivers.end(); ++rit)
    {
        LOG_DEBUG_S << "Transport '" << getName() << "' : deliverOrForwardLetter to: " << rit->getName();

        // Handle delivery
        // The name of the next destination, can be an intermediate receiver
        std::string receiverName = rit->getName();
        fipa::acl::Letter updatedLetter = letter.createDedicatedEnvelope( fipa::acl::AgentID(receiverName) );

        // Check for local receivers, or identify locator
        fipa::services::ServiceDirectoryList list = mpDSD->search(receiverName, fipa::services::ServiceDirectoryEntry::NAME, false);
        if(list.empty())
        {
            LOG_WARN_S << "Transport '" << getName() << "' : could neither deliver nor forward message to receiver: '" << receiverName << "' since it is globally and locally unknown";

            // Cleanup existing entries
            for(std::map<std::string, std::map<std::string, fipa::services::AbstractOutgoingConnection*> >::iterator it = mOutgoingConnections.begin();
                it != mOutgoingConnections.end(); it++)
            {
                std::map<std::string, fipa::services::AbstractOutgoingConnection*>::iterator it2 = it->second.find(receiverName);
                if(it2 != it->second.end())
                {
                    delete it2->second;
                    it->second.erase(it2++);
                }
            }
            continue;
        } else if(list.size() > 1) {
            LOG_WARN_S << "Transport '" << getName() << "' : receiver '" << receiverName << "' has multiple entries in the service directory -- cannot disambiguate";
        } else {
            using namespace fipa::services;
            // Extract the service location
            ServiceDirectoryEntry serviceEntry = list.front();
            ServiceLocator locator = serviceEntry.getLocator();
            ServiceLocations locations = locator.getLocations();
            
            // Try all service locations FIXME comtnue's
            for(ServiceLocations::const_iterator it = locations.begin(); it != locations.end(); it++)
            {
                ServiceLocation location = *it;
                Address address = Address::fromString(location.getServiceAddress());
            
                if (!mOutgoingConnections.count(address.protocol))
                {
                    // Protocol not implemented (or activated)
                    LOG_WARN_S << "Transport '" << getName() << "' : protocol '" << address.protocol << "' is not supported.";
                    continue;
                }

                if( location.getSignatureType() != mServiceSignaturesTypes[address.protocol]
                    && std::find(additionalAcceptedSignatureTypes.begin(), additionalAcceptedSignatureTypes.end(), location.getSignatureType()) == additionalAcceptedSignatureTypes.end() )
                {
                    LOG_INFO_S << "Transport '" << getName() << "' : service signature for '" << receiverName 
                            << "' is '" << location.getSignatureType() << "' but expected '" << mServiceSignaturesTypes[address.protocol] << "' -- will not connect: " << serviceEntry.toString();
                    continue;
                }

                LOG_DEBUG_S << "Transport: '" << getName() << "' forwarding to other MTS";
                    
                // Sending message to another MTS
                std::map<std::string, fipa::services::AbstractOutgoingConnection*>::const_iterator cit = mOutgoingConnections[address.protocol].find(receiverName);

                bool connectionExists = false;
                // Validate connection by comparing address in cache and current address in service directory
                
                fipa::services::AbstractOutgoingConnection* mtsConnection = 0;
                if(cit != mOutgoingConnections[address.protocol].end())
                {
                    LOG_DEBUG_S << "Transport: '" << getName() << "' checking on old connections.";
                    mtsConnection = cit->second;
                    if(mtsConnection->getAddress() != address)
                    {
                        LOG_DEBUG_S << "Transport '" << getName() << "' : cached connection requires an update " << mtsConnection->getAddress().toString() 
                                    << " vs. " << address.toString() << " -- deleting existing entry";

                        delete cit->second;
                        mOutgoingConnections[address.protocol].erase(receiverName);
                    } else {
                        connectionExists = true;
                    }
                }

                // Cache newly created connection
                if(!connectionExists)
                {
                    
                    LOG_DEBUG_S << "Transport: '" << getName() << "' establishing new connection.";
                    try {
                        // Switch protocols
                        if(address.protocol == "udt") {
                            LOG_DEBUG_S << "UDT is the protocol.";
                            mtsConnection = new udt::OutgoingConnection(address);
                        } else if(address.protocol == "tcp") {
                            LOG_DEBUG_S << "TCP is the protocol.";
                            mtsConnection = new tcp::OutgoingConnection(address);
                        } else {
                            LOG_WARN_S << "Don't know how to create a " << address.protocol << " connection. Protocol not implemented.";
                            continue;
                        }
                        mOutgoingConnections[address.protocol][receiverName] = mtsConnection;
                    } catch(const std::exception& e)
                    {
                        LOG_WARN_S << "Transport '" << getName() << "' : could not establish connection to '" << location.toString() << "' -- " << e.what();
                        continue;
                    }
                }

                LOG_DEBUG_S << "Transport: '" << getName() << "' : sending letter to '" << receiverName << "'";
                try {
                    mtsConnection->sendLetter(updatedLetter);

                    fipa::acl::AgentIDList::iterator it = std::find(remainingReceivers.begin(), remainingReceivers.end(), *rit);
                    if(it != remainingReceivers.end())
                    {
                        remainingReceivers.erase(it);
                    }
                    // Successfully sent. Break locations loop.
                    break;
                } catch(const std::runtime_error& e)
                {
                    LOG_WARN_S << "Transport '" << getName() << "' : could not send letter to '" << receiverName << "' -- " << e.what();
                    continue;
                } 
            }
        }
    }

    return remainingReceivers;
}

} // end namespace services
} // end namespace fipa