#ifndef FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP
#define FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP

#include <fipa_services/ServiceDirectory.hpp>
#include <service_discovery/ServiceDiscovery.hpp>

namespace fipa {
namespace services {
/**
 * \class DistributedServiceDirectory
 * \brief Implementation of a distributed service directory using the functionality of Avahi
 */
class DistributedServiceDirectory : public ServiceDirectory
{
    typedef std::map<ServiceDirectoryEntry, servicediscovery::avahi::ServiceDiscovery*> ServiceDiscoveryMap;
    ServiceDiscoveryMap mServiceDiscoveries;

public:
    /**
     * Convert a ServiceDirectoryEntry to a avahi description
     * \param entry ServiceDirectoryEntry
     * \return ServiceDescription
     */
    static servicediscovery::avahi::ServiceDescription convert(const ServiceDirectoryEntry& entry);

    /**
     * Convert an avahi ServiceDescription to a ServiceDirectoryEntry
     * \param description avahi ServiceDescription
     * \return ServiceDirectoryEntry
     */
    static ServiceDirectoryEntry convert(const servicediscovery::avahi::ServiceDescription& description);

    /**
     * Convert map of avahi ServiceDescriptions to a ServiceDirectoryList
     * \param servicesMap Map of avahi ServiceDescriptions
     * \return ServiceDirectoryList
     */
    static ServiceDirectoryList convert(const std::map<std::string, servicediscovery::avahi::ServiceDescription>& servicesMap);

    /**
     * Register a service
     * \param entry The ServiceDirectoryEntry which describes the service that shall be registered
     * \param publishDomain Default domain under which services will be published
     * \param ttlInMS TimeToLive in Milliseconds
     */
    void registerService(const ServiceDirectoryEntry& entry, const std::string& publishDomain = "_fipa_service_directory._udp", uint32_t ttlInMS = 30000);

    /**
     * Looks up services that have been registered with this instance and deregisters them
     * \throws NotFound If the service has not been locally deregistered and thus cannot be deregistered
     */
    void deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field);

    /**
     * Search for a given service based on the field information
     * \param regex Regular expression which will be applied to the selected field
     * \param field Field selection which the regex shall be applied on
     * \param doThrow If set to true, the function will throw an exception if no entry could be found
     */
    ServiceDirectoryList search(const std::string& regex, ServiceDirectoryEntry::Field field, bool doThrow) const;
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP
