#include "Transport.hpp"

#include <base-logging/Logging.hpp>
#include <boost/algorithm/string.hpp>

#include <stdexcept>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#ifndef TRANSPORT_UDT_UNSUPPORTED
#include <fipa_services/transports/udt/UDTTransport.hpp>
#endif
#include <fipa_services/transports/tcp/TCPTransport.hpp>

namespace fipa {
namespace services {
namespace transports {

std::map<Transport::Type, std::string> Transport::TypeTxt = {
    {Transport::UNKNOWN, "UNKNOWN"},
    {Transport::UDT, "UDT"},
    {Transport::TCP, "TCP"},
    {Transport::ALL, "ALL"}};

std::string Transport::getLocalIPv4Address(const std::string& interfaceName)
{
    struct ifaddrs* interfaces;

    if(0 == getifaddrs(&interfaces))
    {
        struct ifaddrs* interface;
        for(interface = interfaces; interface != NULL; interface = interface->ifa_next)
        {
            // is interface running?
            if(interface->ifa_flags & IFF_UP)
            {
                // is interface IPv4?
                if(interface->ifa_addr->sa_family == AF_INET)
                {
                    // is the interface name matching?
                    if(interfaceName == std::string(interface->ifa_name) || interfaceName.empty())
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
    }

    freeifaddrs(interfaces);
    throw std::runtime_error("fipa::services::Transport: could not get interface address of '" + interfaceName + "'");
}

Transport::Ptr Transport::create(Type type)
{
    switch(type)
    {
        case UDT:
#ifndef TRANSPORT_UDT_UNSUPPORTED
            return Transport::Ptr(new udt::UDTTransport());
#else
            throw std::runtime_error("fipa::services::Transport: support for selected transport is not available: " + TypeTxt[type]);
#endif
        case TCP:
            return Transport::Ptr(new tcp::TCPTransport());
        default:
            throw std::invalid_argument("fipa::services::Transport cannot create transport of type" + TypeTxt[type]);
    }
}

Transport::Transport(Type type)
    : mType(type)
{
}

Transport::Transport(const Configuration& config)
    : mType( getTypeFromTxt(config.transport_type) )
    , mConfiguration(config)
{}

Transport::Type Transport::getTypeFromTxt(const std::string& type)
{
    std::string tmp = type;
    boost::to_upper(tmp);
    std::map<Type, std::string>::const_iterator cit = TypeTxt.begin();
    for(; cit != TypeTxt.end(); ++cit)
    {
        if(cit->second == tmp)
        {
            return cit->first;
        }
    }
    throw std::invalid_argument("Transport::getTypeFromString: unknown type '" + type + "'");

}

void Transport::registerObserver(TransportObserver observer)
{
    mObservers.push_back(observer);
}

void Transport::notify(const std::string& data)
{
    std::vector<TransportObserver>::iterator it = mObservers.begin();
    for(; it != mObservers.end(); ++it)
    {
        TransportObserver notifyHandle = *it;
        // trigger notification
        notifyHandle(data);
    }
}

OutgoingConnection::Ptr Transport::getCachedOutgoingConnection(const std::string& receiverName, const Address& address)
{
    std::map<std::string, OutgoingConnection::Ptr>::const_iterator cit = mOutgoingConnections.find(receiverName);

    OutgoingConnection::Ptr connection;

    if(cit != mOutgoingConnections.end())
    {
        LOG_DEBUG_S << "Transport: '" << getName() << "': checking on cached connection.";
        connection = cit->second;
        if(connection->getAddress() != address)
        {
            LOG_DEBUG_S << "Transport '" << getName() << "': cached connection requires an update " << connection->getAddress().toString()
                        << " vs. " << address.toString() << " -- deleting existing entry";
            mOutgoingConnections.erase(receiverName);
        }  else {
            return connection;
        }
    }
    // Return nullptr
    return OutgoingConnection::Ptr();
}

void Transport::send(const std::string& receiverName, const Address& address, const std::string& data)
{
    /// Send letter according to this transport
    // allow for one retry in order to handle old dangling connection
    LOG_DEBUG_S << "Transport: '" << getName() << "': sending letter to '" << receiverName << "'";
    for(int r = 0; r < 2; ++r)
    {
        // Validate connection by comparing address in cache and current address in service directory
        OutgoingConnection::Ptr connection = getCachedOutgoingConnection(receiverName, address);

        // Connection does not exist, create and
        // cache new connection
        if(!connection)
        {
            LOG_DEBUG_S << "Transport: '" << getName() << "': establishing new connection.";
            try {
                connection = establishOutgoingConnection(address);

                // cache connection
                mOutgoingConnections[receiverName] = connection;
            } catch(const std::exception& e)
            {
               mOutgoingConnections.erase(receiverName);
               throw std::runtime_error("Transport '" + getName() + "': could not establish connection to '" + address.toString() + "' -- " + e.what());
            }
        }

        try {
            connection->send(data);
            // Successfully sent. Break locations loop.
            break;
        } catch(const std::exception& e)
        {
            mOutgoingConnections.erase(receiverName);
            if(r >= 1) // after one retry throw
            {
                throw std::runtime_error("Transport '" + getName() + "': could not send data to '" + receiverName + "' -- " + e.what());
            } else {
                LOG_DEBUG_S << "Transport: '" << getName() << "': first try sending letter to '" << receiverName << "' failed -- cleaning cache to handle dangling connection";
            }
        }
    }
}

std::set<Address> Transport::getAddresses() const
{
    std::set<Address> addresses;

    struct ifaddrs* interfaces;
    if(0 == getifaddrs(&interfaces))
    {
        struct ifaddrs* interface;
        for(interface = interfaces; interface != NULL; interface = interface->ifa_next)
        {
            if(interface->ifa_addr == NULL)
            {
                continue;
            }

            // filter out loopback device
            if(!(interface->ifa_flags & IFF_LOOPBACK))
            {
                std::string interfaceName(interface->ifa_name);
                try {
                    Address address = getAddress(interfaceName);
                    addresses.insert(address);
                } catch(const std::exception& e)
                {
                    LOG_DEBUG_S << "Could not retrieve address for nic: " << interfaceName;
                }
            }
        }
    }
    freeifaddrs(interfaces);

    if(addresses.empty())
    {
        throw std::runtime_error("fipa::services::Transport: could not get any address for this transport");
    } else {
        LOG_DEBUG_S << "Return " << addresses.size() << " addresses";
        return addresses;
    }
}

void Transport::cleanup(const std::string& receiverName)
{
    // Cleanup existing entries for the given receiver name, since they
    // are not valid any more
    std::map<std::string, OutgoingConnection::Ptr>::iterator it = mOutgoingConnections.find(receiverName);
    if(it != mOutgoingConnections.end())
    {
        // Erase entry
        mOutgoingConnections.erase(it++);
    }
}

} // end namespace transport
} // end namespace services
} // end namespace fipa
