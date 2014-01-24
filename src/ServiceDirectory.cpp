#include "ServiceDirectory.hpp"
#include "ServiceDirectoryEntry.hpp"
#include <boost/regex.hpp>
#include "ErrorHandling.hpp"

namespace fipa {
namespace services {

void ServiceDirectory::registerService(const ServiceDirectoryEntry& entry)
{
    mServices[entry.getName()] = entry;
}

void ServiceDirectory::deregisterService(const ServiceDirectoryEntry& entry)
{
    ServiceDirectory::deregisterByName(entry.getName());
}

void ServiceDirectory::deregisterByName(const Name& name)
{
    ServiceDirectoryMap::iterator it = mServices.find(name); 
    if(it != mServices.end())
    {
        mServices.erase(it);
    } else {
        throw NotFound();
    }
}

ServiceDirectoryList ServiceDirectory::search(const ServiceDirectoryEntry& entry) const
{
    return searchByName(entry.getName());
}

ServiceDirectoryList ServiceDirectory::searchByName(const std::string& regex) const
{
    ServiceDirectoryMap::const_iterator it = mServices.begin();
    ServiceDirectoryList resultList;

    boost::regex r(regex);
    boost::smatch what;
    for(; it != mServices.end(); ++it)
    {
        if(boost::regex_match(it->first,what,r))
        {
            resultList.push_back(it->second);
        }
    }

    return resultList;
}

void ServiceDirectory::modify(const ServiceDirectoryEntry& entry)
{
    ServiceDirectoryList list = search(entry);

    if(list.empty())
    {
        throw NotFound();
    }

    mServices[entry.getName()] = entry;
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

} // end namespace services
} // end namespace fipa
