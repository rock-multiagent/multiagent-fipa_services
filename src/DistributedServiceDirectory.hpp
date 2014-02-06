#ifndef FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP
#define FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP

#include <fipa_services/ServiceDirectory.hpp>
#include <service_discovery/ServiceDiscovery.hpp>

namespace fipa {
namespace services {

class DistributedServiceDirectory : public ServiceDirectory
{
    typedef std::map<ServiceDirectoryEntry, servicediscovery::avahi::ServiceDiscovery*> ServiceDiscoveryMap;
    ServiceDiscoveryMap mServiceDiscoveries;

public:
    static servicediscovery::avahi::ServiceDescription convert(const ServiceDirectoryEntry& entry);

    static ServiceDirectoryEntry convert(const servicediscovery::avahi::ServiceDescription& description);

    static ServiceDirectoryList convert(const std::map<std::string, servicediscovery::avahi::ServiceDescription>& servicesMap);

    /**
     * Register a service
     */
    void registerService(const ServiceDirectoryEntry& entry, const std::string& publishDomain = "_fipa_service_directory._udp", uint32_t ttlInMS = 30000);

    /**
     * Looks up services that have been registered with this instance and deregisters them
     * \throws NotFound If the service has not been locally deregistered and thus cannot be deregistered
     */
    void deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field);

    /**
     * Search for a given service based on the field information
     */
    ServiceDirectoryList search(const std::string& regex, ServiceDirectoryEntry::Field field, bool doThrow) const;
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP
