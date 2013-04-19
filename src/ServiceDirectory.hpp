#ifndef FIPA_SERVICES_SERVICE_DIRECTORY_HPP
#define FIPA_SERVICES_SERVICE_DIRECTORY_HPP

#include <map>
#include <fipa_services/ServiceDirectoryEntry.hpp>
#include <fipa_services/ErrorHandling.hpp>

namespace fipa {
namespace service {

class ServiceDirectory
{
    ServiceDirectoryMap mServices;

public:

    /**
     * Register service
     * \param entry description object to add
     * \throws DuplicateEntry
     */
    void registerService(const ServiceDirectoryEntry& entry);


    /**
     * Deregister service
     * \param entry (only the name is relevant for deregistration)
     * \throws NotFound
     */
    void deregisterService(const ServiceDirectoryEntry& entry);

    /**
     * Remove a service by name
     * \param name Name of the service
     */
    void deregisterByName(const Name& name);

    /**
     * Search for a service matching the given 
     * name
     */
    ServiceDirectoryList search(const ServiceDirectoryEntry& entry) const;

    /**
     * Search for a service with a name matching the given 
     * regular expression
     */
    ServiceDirectoryList searchByName(const std::string& regex) const;

    /**
     * Modify an existing entry
     * \throws NotFound if entry does not exist
     */
    void modify(const ServiceDirectoryEntry& modify);

    /**
     * Retrieve all registered services
     */
    ServiceDirectoryList getAll() const;

};

} // end namespace service
} // end namespace fipa
#endif // FIPA_SERVICES_SERVICE_DIRECTORY_HPP
