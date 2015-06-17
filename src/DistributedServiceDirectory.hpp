#ifndef FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP
#define FIPA_SERVICES_DISTRIBUTED_SERVICE_DIRECTORY_HPP

#include <fipa_services/ServiceDirectory.hpp>
#include <service_discovery/ServiceDiscovery.hpp>

#define DEFAULT_SERVICE_SCOPE "_fipa_service_directory._udp"

namespace fipa {
namespace services {
/**
 * \class DistributedServiceDirectory
 * \brief Implementation of a distributed service directory using the functionality of Avahi
 * \details The distributed service directory allows to register services which are then published
 * in a given avahi domain (default is _fipa_service_directory._udp). Each service can be associated
 * with information on how to access the service. This is done constructing a ServiceLocator object and specifying a service locator. 
 * \see http://www.fipa.org/specs/fipa00001/SC00001L.html#_Toc26668707
 * \verbatim
 #include <fipa_services/DistributedServiceDirectory.hpp>

 using namespace fipa::services;
 DistributedServiceDirectory directory;

 ServiceLocator locator;
 locator.addLocation("tcp://192.168.1.1:2000","agent-signature-type","agent-service-signature");
 ServiceDirectoryEntry client("local-agent","type-of-agent",locator, "Description of local-agent");
 directory.registerService(client);

 fipa::services::ServiceDirectory list = directory.search("other-agent", ServiceDirectoryEntry::NAME, false);

 \endverbatim
 */
class DistributedServiceDirectory : public ServiceDirectory
{
    typedef std::map<ServiceDirectoryEntry, servicediscovery::avahi::ServiceDiscovery*> ServiceDiscoveryMap;
    ServiceDiscoveryMap mServiceDiscoveries;

    static std::string canonizeName(const std::string& name);

    // Default listing object (to make sure service discovery is working when
    // no other service is registered)
    servicediscovery::avahi::ServiceDiscovery* mServiceDiscovery;

public:

    /**
     * Default constructor using the default listening scope for that service
     * discovery
     */
    DistributedServiceDirectory(const std::string& scope = DEFAULT_SERVICE_SCOPE);

    /**
     * Constructor to allow setting of multiple listening scopes
     */
    DistributedServiceDirectory(const std::vector<std::string>& scopes);

    /**
     * Virtual Destructor
     */
    virtual ~DistributedServiceDirectory();

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
     * RegisterService
     */
    void registerService(const ServiceDirectoryEntry& entry) { registerService(entry, DEFAULT_SERVICE_SCOPE); }

    /**
     * Register a service
     * \param entry The ServiceDirectoryEntry which describes the service that shall be registered
     * \param publishDomain Default domain under which services will be published
     * \param ttlInMS TimeToLive in Milliseconds
     */
    void registerService(const ServiceDirectoryEntry& entry, const std::string& publishDomain, uint32_t ttlInMS = 30000);

    /**
     * Looks up services that have been registered with this instance and deregisters them
     * \throws NotFound If the service has not been locally deregistered and thus cannot be deregistered
     */
    virtual void deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field);

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
