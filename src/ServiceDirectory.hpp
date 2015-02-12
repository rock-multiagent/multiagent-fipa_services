#ifndef FIPA_SERVICES_SERVICE_DIRECTORY_HPP
#define FIPA_SERVICES_SERVICE_DIRECTORY_HPP

#include <map>
#include <set>
#include <stdexcept>
#include <boost/thread.hpp>
#include <fipa_services/ServiceDirectoryEntry.hpp>
#include <fipa_services/ErrorHandling.hpp>

namespace fipa {
namespace services {

/**
 * \class ServiceDirectory
 * \brief Class to describe FIPA service directory
 */
class ServiceDirectory
{
    // Map of registered services
    ServiceDirectoryMap mServices;
    base::Time mTimestamp;

protected:
    // Mutex to guarantee thread-safe operation
    boost::mutex mMutex;

public:
    typedef boost::shared_ptr<ServiceDirectory> Ptr;

    ServiceDirectory();
    virtual ~ServiceDirectory() {}

    /**
     * Register service
     * \param entry description object to add
     * \throws DuplicateEntry
     */
    virtual void registerService(const ServiceDirectoryEntry& entry);

    /**
     * Deregister service
     * \param entry (only the name is relevant for deregistration)
     * \throws NotFound
     */
    void deregisterService(const ServiceDirectoryEntry& entry);

    /**
     * Remove a service by name
     * \param regex regular expression
     * \param field field to apply the regular expression on, default is name of the entry
     */
    virtual void deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field = ServiceDirectoryEntry::NAME);

    /**
     * Search for a service matching the given name
     * \param entry Entry to be searched for
     * \return Result list
     *
     */
    ServiceDirectoryList search(const ServiceDirectoryEntry& entry) const;

    /**
     * Search for a service with a name matching the given 
     * regular expression
     * \param regex Regular expression to match
     * \param field Field name to apply the regex to
     * \param doThrow Flag to control the throw behaviour, i.e. to throw an exception when no result has been found
     * \throw NotFound
     * \return Result list
     */
    virtual ServiceDirectoryList search(const std::string& regex, ServiceDirectoryEntry::Field field = ServiceDirectoryEntry::NAME, bool doThrow = true) const;

    /**
     * Modify an existing entry -- an entry will be identified by the same name
     * \param entry Entry that updates the existing one
     * \throws NotFound if entry does not exist
     */
    void modify(const ServiceDirectoryEntry& entry);

    /**
     * Update modification time
     */
    void updateTimestamp() { mTimestamp = base::Time::now(); }

    /**
     * Get timestamp
     * \return Timestamp when updateTimestamp has been called the last time
     */
    base::Time getTimestamp() const { return mTimestamp; }

    /**
     * Retrieve all registered services
     * \return List of all registered services
     */
    ServiceDirectoryList getAll() const;

    /**
     * Merge the existing service directory list with the existing.
     *
     * Uses each unique value to update corresponding field, e.g.,
     * when locator is set as field for the selective merge then all instances with the same locator
     * are removed from the service directory and are thus overriden by the updatelist
     * \param updateList list of services selected to update the existing one
     * \param selectiveMerge Field by which entries are identified for removal
     */
    virtual void mergeSelectively(const ServiceDirectoryList& updateList, ServiceDirectoryEntry::Field selectiveMerge);

private:
    /**
     * Extract the unique fields
     * \param list List of entries
     * \param field Field selection to identify unique values from
     * \return Set of field values
     */
    static std::set<std::string> getUniqueFieldValues(const ServiceDirectoryList& list, ServiceDirectoryEntry::Field field);

    /**
     * Clear the service directory selectively, i.e. for field that match the given
     * regex
     * \param regex Regular expression to match
     * \param field Field that the regex should be applied on
     */
    void clearSelectively(const std::string& regex, ServiceDirectoryEntry::Field field);
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_SERVICE_DIRECTORY_HPP
