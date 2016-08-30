#include "DistributedServiceDirectory.hpp"
#include <boost/algorithm/string.hpp>
#include <base-logging/Logging.hpp>

namespace fipa {
namespace services {

DistributedServiceDirectory::DistributedServiceDirectory(const std::string& scope)
    : mServiceDiscovery(new servicediscovery::avahi::ServiceDiscovery())
{
    std::vector<std::string> scopes;
    scopes.push_back(scope);

    mServiceDiscovery->listenOn(scopes);
}

DistributedServiceDirectory::DistributedServiceDirectory(const std::vector<std::string>& scopes)
    : mServiceDiscovery(new servicediscovery::avahi::ServiceDiscovery())
{
    mServiceDiscovery->listenOn(scopes);
}

DistributedServiceDirectory::~DistributedServiceDirectory()
{
    mServiceDiscovery->stop();
    delete mServiceDiscovery;
}

std::string DistributedServiceDirectory::canonizeName(const std::string& name)
{
    std::string tmp = name;
    // Replace all dots in the name with a ?
    // that makes it useable as regex at the same time
    boost::replace_all(tmp, ".","?");
    return tmp;
}

servicediscovery::avahi::ServiceDescription DistributedServiceDirectory::convert(const ServiceDirectoryEntry& entry)
{
    servicediscovery::avahi::ServiceDescription description;

    // Set all fields for publishing
    for(size_t i = 0; i < ServiceDirectoryEntry::END_MARKER; ++i)
    {
        if(i == (int) ServiceDirectoryEntry::NAME)
        {
            description.setName( canonizeName(entry.getName()) );
        } else {
            description.setDescription( ServiceDirectoryEntry::FieldTxt[(ServiceDirectoryEntry::Field) i] ,entry.getFieldContent( (ServiceDirectoryEntry::Field) i));
        }

    }
    return description;
}


ServiceDirectoryEntry DistributedServiceDirectory::convert(const servicediscovery::avahi::ServiceDescription& description)
{
    ServiceDirectoryEntry entry;
    // Set all field for publishing
    for(size_t i = 0; i < ServiceDirectoryEntry::END_MARKER; ++i)
    {
        ServiceDirectoryEntry::Field field = (ServiceDirectoryEntry::Field) i;
        if(i == (int) ServiceDirectoryEntry::NAME)
        {
            entry.setFieldContent( field, canonizeName(description.getName()) );
        } else {
            entry.setFieldContent( field, description.getDescription(ServiceDirectoryEntry::FieldTxt[ field ]));
        }
    }
    return entry;
}

ServiceDirectoryList DistributedServiceDirectory::convert(const std::map<std::string, servicediscovery::avahi::ServiceDescription>& servicesMap)
{
    ServiceDirectoryList serviceDirectoryList;
    std::map<std::string, servicediscovery::avahi::ServiceDescription>::const_iterator cit = servicesMap.begin();
    for(; cit != servicesMap.end(); ++cit)
    {
        serviceDirectoryList.push_back( convert( cit->second) );
    }

    return serviceDirectoryList;
}

void DistributedServiceDirectory::registerService(const ServiceDirectoryEntry& entry, const std::string& publishDomain, uint32_t ttlInMS)
{
    using namespace servicediscovery::avahi;

    LOG_DEBUG_S << "DistributedServiceDirectory: register: " << entry.toString();

    boost::unique_lock<boost::mutex> lock(mMutex);
    ServiceDescription serviceDescription = convert(entry); 

    ServiceConfiguration serviceConfiguration(serviceDescription);
    serviceConfiguration.setType(publishDomain);

    ServiceDiscovery* serviceDiscovery = new ServiceDiscovery();
    serviceDiscovery->start(serviceConfiguration);

    mServiceDiscoveries[entry] = serviceDiscovery;
}

void DistributedServiceDirectory::deregisterService(const std::string& regex, ServiceDirectoryEntry::Field field)

{
    using namespace servicediscovery::avahi;

    boost::unique_lock<boost::mutex> lock(mMutex);
    ServiceDiscoveryMap::iterator it = mServiceDiscoveries.begin();

    boost::regex r(regex);
    boost::smatch what;
    for(; it != mServiceDiscoveries.end(); ++it)
    {
        ServiceDirectoryEntry entry = it->first;
        ServiceDiscovery* serviceDiscovery = it->second;

        if(boost::regex_match( entry.getFieldContent(field) ,what,r))
        {
            serviceDiscovery->stop();
            delete serviceDiscovery;
            mServiceDiscoveries.erase(it);
            return;
        }
    }
    throw NotFound("DistributedServiceDirectory: deregistration failed. No known ServiceDirectoryEntry matching '" + regex + "'");
}

ServiceDirectoryList DistributedServiceDirectory::search(const std::string& regex, ServiceDirectoryEntry::Field field, bool doThrow) const
{
    using namespace servicediscovery::avahi;

    SearchPattern pattern;
    if(field == ServiceDirectoryEntry::NAME)
    {
        pattern = SearchPattern(regex);
    } else {
        pattern = SearchPattern(".*", ServiceDirectoryEntry::FieldTxt[field], regex);
    }

    // Search for services
    LOG_DEBUG_S << "Searching for services: " << regex;
    std::map<std::string, ServiceDescription> results = ServiceDiscovery::getVisibleServices(pattern);
    LOG_DEBUG_S << "Search completed";
    if(results.empty() && doThrow)
    {
        throw NotFound("DistributedServiceDirectory::search failed: No ServiceDirectoryEntry matching '" + regex + "' on field type '" + ServiceDirectoryEntry::FieldTxt[field] + "'");
    }

    return convert(results);
}

} // end namespace services
} // end namespace fipa
