#include "ServiceDirectory.hpp"
#include "ServiceDirectoryEntry.hpp"
#include <boost/regex.hpp>
#include "ErrorHandling.hpp"

namespace fipa {
namespace services {

ServiceDirectory::ServiceDirectory()
    : mTimestamp( base::Time::now() )
{
}

void ServiceDirectory::registerService(const ServiceDirectoryEntry& entry)
{
    mServices[entry.getName()] = entry;
    updateTimestamp();
}

void ServiceDirectory::deregisterService(const ServiceDirectoryEntry& entry)
{
    ServiceDirectory::deregisterService(entry.getName(), ServiceDirectoryEntry::NAME);
}

void ServiceDirectory::deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field)
{
    ServiceDirectoryMap::iterator it = mServices.begin();

    boost::regex r(regex);
    boost::smatch what;
    for(; it != mServices.end(); ++it)
    {
        ServiceDirectoryEntry entry = it->second;

        if(boost::regex_match( entry.getFieldContent(field) ,what,r))
        {
            mServices.erase(it);
            updateTimestamp();
            return;
        }
    }
    throw NotFound();
}

ServiceDirectoryList ServiceDirectory::search(const ServiceDirectoryEntry& entry) const
{
    return search(entry.getName(), ServiceDirectoryEntry::NAME);
}

ServiceDirectoryList ServiceDirectory::search(const std::string& regex, ServiceDirectoryEntry::Field field) const
{
    ServiceDirectoryMap::const_iterator it = mServices.begin();
    ServiceDirectoryList resultList;

    boost::regex r(regex);
    boost::smatch what;
    for(; it != mServices.end(); ++it)
    {
        ServiceDirectoryEntry entry = it->second;

        if(boost::regex_match( entry.getFieldContent(field) ,what,r))
        {
            resultList.push_back(entry);
        }
    }
    if(resultList.empty())
    {
        throw NotFound();
    } else {
        return resultList;
    }
}

void ServiceDirectory::modify(const ServiceDirectoryEntry& entry)
{
    ServiceDirectoryList list = search(entry);

    if(list.empty())
    {
        throw NotFound();
    }

    mServices[entry.getName()] = entry;
    updateTimestamp();
}

ServiceDirectoryList ServiceDirectory::getAll() const
{
    ServiceDirectoryMap::const_iterator it = mServices.begin();
    ServiceDirectoryList resultList;

    for(; it != mServices.end(); ++it)
    {
        resultList.push_back(it->second);
    }

    return resultList;
}


void ServiceDirectory::mergeSelectively(const ServiceDirectoryList& updateList, ServiceDirectoryEntry::Field field)
{
    std::set<std::string> uniqueValues = getUniqueFieldValues(updateList, field);
    std::set<std::string>::const_iterator cit = uniqueValues.begin();

    for(; cit != uniqueValues.end(); ++cit)
    {
        clearSelectively(*cit, field);
    }

    ServiceDirectoryList::const_iterator uit = updateList.begin();
    for(; uit != updateList.end(); ++uit)
    {
        registerService(*uit);
    }
}

std::set<std::string> ServiceDirectory::getUniqueFieldValues(const ServiceDirectoryList& list, ServiceDirectoryEntry::Field field)
{
    ServiceDirectoryList::const_iterator cit = list.begin();
    std::set<std::string> uniqueFieldValues;
    for(; cit != list.end(); ++cit)
    {
        ServiceDirectoryEntry entry = *cit;
        uniqueFieldValues.insert( entry.getFieldContent( field ) );
    }

    return uniqueFieldValues;
}

void ServiceDirectory::clearSelectively(const std::string& regex, ServiceDirectoryEntry::Field field)
{
    ServiceDirectoryList list = ServiceDirectory::search(regex, field);
    ServiceDirectoryList::const_iterator cit = list.begin();
    for(; cit != list.end(); ++cit)
    {
        ServiceDirectoryEntry entry = *cit;
        deregisterService( entry.getName(), ServiceDirectoryEntry::NAME );
    }
}

} // end namespace services
} // end namespace fipa
