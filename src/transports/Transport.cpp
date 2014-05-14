#include "Transport.hpp"

#include <base/Logging.hpp>

#include <stdexcept>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <fipa_services/transports/udt/UDTTransport.hpp>

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
    : mPort(0)
    , mIP("0.0.0.0")
{}

Connection::Connection(const std::string& ip, uint16_t port)
    : mPort(port)
    , mIP(ip)
{}

Connection::Connection(const Address& address)
    : mPort(address.port)
    , mIP(address.ip)
{}

// Transport
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

Transport::Transport(const std::string& name, DistributedServiceDirectory* dsd, fipa::services::ServiceLocation* serviceLocation)
    : mpDSD(dsd)
    , mServiceLocation(serviceLocation)
    , name(name)
{}


// TODO move LocalDelivery back to orogen
// TODO this should be UDT independent
fipa::acl::AgentIDList Transport::deliverOrForwardLetterViaUDT(const fipa::acl::Letter& letter)
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
        // The intended receiver, i.e. name of the final destination
        std::string intendedReceiverName = receiverName;
        fipa::acl::Letter updatedLetter = letter.createDedicatedEnvelope( fipa::acl::AgentID(intendedReceiverName) );

        // Check for local receivers, or identify locator
        fipa::services::ServiceDirectoryList list = mpDSD->search(receiverName, fipa::services::ServiceDirectoryEntry::NAME, false);
        if(list.empty())
        {
            LOG_WARN_S << "Transport '" << getName() << "' : could neither deliver nor forward message to receiver: '" << receiverName << "' since it is globally and locally unknown";

            // Cleanup existing entries
            std::map<std::string, fipa::services::udt::OutgoingConnection*>::const_iterator cit = mMTSConnections.find(receiverName);
            if(cit != mMTSConnections.end())
            {
                delete cit->second;
                mMTSConnections.erase(receiverName);
            }
            continue;
        } else if(list.size() > 1) {
            LOG_WARN_S << "Transport '" << getName() << "' : receiver '" << receiverName << "' has multiple entries in the service directory -- cannot disambiguate'";
        } else {
            using namespace fipa::services;
            // Extract the service location
            ServiceDirectoryEntry serviceEntry = list.front();
            ServiceLocator locator = serviceEntry.getLocator();
            ServiceLocation location = locator.getFirstLocation();

            if( location.getSignatureType() != mServiceLocation->getSignatureType())
            {
                LOG_ERROR_S << "Transport '" << getName() << "' : service signature for '" << receiverName << "' is '" << location.getSignatureType() << "' but expected '" << mServiceLocation->getSignatureType() << "' -- will not connect: " << serviceEntry.toString();
                continue;
            }

            // Local delivery
            if(location == *mServiceLocation)
            {
                LOG_DEBUG_S << "Transport: '" << getName() << "' delivery to local client";

                // FIXME receiversPorts is Orogen
                // Deliver the message to local client, i.e. the corresponding receiver has a dedicated output port available on this MTS
//                 ReceiverPorts::iterator portsIt = mReceivers.find(receiverName);
//                 if(portsIt == mReceivers.end())
//                 {
//                     LOG_WARN_S << "Transport '" << getName() << "' : could neither deliver nor forward message to receiver: '" << receiverName << "' due to an internal error. No port is available for this receiver.";
//                     continue;
//                 } else {
//                     RTT::OutputPort<fipa::SerializedLetter>* clientPort = dynamic_cast< RTT::OutputPort<fipa::SerializedLetter>* >(portsIt->second);
//                     if(clientPort)
//                     {
//                         fipa::SerializedLetter serializedLetter(updatedLetter, fipa::acl::representation::BITEFFICIENT);
//                         if(!clientPort->connected())
//                         {
//                             LOG_ERROR_S << "Transport '" << getName() << "' : client port to '" << receiverName << "' exists, but is not connected";
//                             continue;
//                         } else {
//                             clientPort->write(serializedLetter);
// 
//                             fipa::acl::AgentIDList::iterator it = std::find(remainingReceivers.begin(), remainingReceivers.end(), receiverName);
//                             if(it != remainingReceivers.end())
//                             {
//                                 remainingReceivers.erase(it);
//                             }
// 
//                             LOG_DEBUG_S << "Transport '" << getName() << "' : delivery to '" << receiverName << "' (indendedReceiver is '" << intendedReceiverName << "')";
//                             continue;
//                         }
//                     } else {
//                         LOG_ERROR_S << "Transport '" << getName() << "' : internal error since client port could not be casted to expected type";
//                         continue;
//                     }
//                 }
            } else {
                LOG_DEBUG_S << "Transport: '" << getName() << "' forwarding to other MTS";

                // Sending message to another MTS
                std::map<std::string, fipa::services::udt::OutgoingConnection*>::const_iterator cit = mMTSConnections.find(receiverName);
                Address address = Address::fromString(location.getServiceAddress());

                bool connectionExists = false;
                // Validate connection by comparing address in cache and current address in service directory
                //TODO use udt or tcp respectively
                fipa::services::udt::OutgoingConnection* mtsConnection = 0;
                if(cit != mMTSConnections.end())
                {
                    mtsConnection = cit->second;
                    if(mtsConnection->getAddress() != address)
                    {
                        LOG_DEBUG_S << "Transport '" << getName() << "' : cached connection requires an update " << mtsConnection->getAddress().toString() << " vs. " << address.toString() << " -- deleting existing entry";

                        delete cit->second;
                        mMTSConnections.erase(receiverName);
                    } else {
                        connectionExists = true;
                    }
                }

                // Cache newly created connection
                if(!connectionExists)
                {
                    try {
                        mtsConnection = new udt::OutgoingConnection(address);
                        mMTSConnections[receiverName] = mtsConnection;
                    } catch(const std::runtime_error& e)
                    {
                        LOG_WARN_S << "Transport '" << getName() << "' : could not establish connection to '" << location.toString() << "' -- " << e.what();
                        continue;
                    }
                }

                LOG_DEBUG_S << "Transport: '" << getName() << "' : sending letter to '" << receiverName << "'";
                try {
                    mtsConnection->sendLetter(updatedLetter);

                    fipa::acl::AgentIDList::iterator it = std::find(remainingReceivers.begin(), remainingReceivers.end(), receiverName);
                    if(it != remainingReceivers.end())
                    {
                        remainingReceivers.erase(it);
                    }
                } catch(const std::runtime_error& e)
                {
                    LOG_WARN_S << "Transport '" << getName() << "' : could not send letter to '" << receiverName << "' -- " << e.what();
                    continue;
                }
            } // end handling
        }
    }

    return remainingReceivers;
}

} // end namespace services
} // end namespace fipa