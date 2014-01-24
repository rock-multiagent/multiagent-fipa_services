#ifndef FIPA_SERVICES_SERVICE_DIRECTORY_HPP
#define FIPA_SERVICES_SERVICE_DIRECTORY_HPP

#include <map>
#include <set>
#include <stdexcept>
#include <fipa_services/ServiceDirectoryEntry.hpp>
#include <fipa_services/ErrorHandling.hpp>

namespace fipa {
namespace services {

class ServiceDirectory
{
    ServiceDirectoryMap mServices;
    base::Time mTimestamp;

public:

    ServiceDirectory();

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
    void deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field = ServiceDirectoryEntry::NAME);

    /**
     * Search for a service matching the given
     * name
     */
    ServiceDirectoryList search(const ServiceDirectoryEntry& entry) const;

    /**
     * Search for a service with a name matching the given 
     * regular expression
     * \throw NotFound
     */
    ServiceDirectoryList search(const std::string& regex, ServiceDirectoryEntry::Field field = ServiceDirectoryEntry::NAME ) const;

    /**
     * Modify an existing entry
     * \throws NotFound if entry does not exist
     */
    void modify(const ServiceDirectoryEntry& modify);

    /**
     * Update modification time
     */
    void updateTimestamp() { mTimestamp = base::Time::now(); }

    /**
     * Get timestamp
     */
    base::Time getTimestamp() const { return mTimestamp; }

    /**
     * Retrieve all registered services
     */
    ServiceDirectoryList getAll() const;

    /**
     * Merge the existing service directory list with the existing.
     *
     * Uses each unique value to update corresponding field, e.g.,
     * when locator is set for seletive merge then all instances with a locator occuring in the update
     * are removed from the service directory to be updated
     */
    void mergeSelectively(const ServiceDirectoryList& updateList, ServiceDirectoryEntry::Field selectiveMerge);

private:
    /**
     * Extract the unique fields
     */
    static std::set<std::string> getUniqueFieldValues(const ServiceDirectoryList& list, ServiceDirectoryEntry::Field field);

    /**
     * Clear the service directory selectively, i.e. for field that match the given
     * regex
     */
    void clearSelectively(const std::string& regex, ServiceDirectoryEntry::Field field);
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_SERVICE_DIRECTORY_HPP
