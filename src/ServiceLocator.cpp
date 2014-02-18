#include "ServiceLocator.hpp"
#include "ErrorHandling.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace fipa {
namespace services {

std::string ServiceLocation::getFieldContent(ServiceLocation::Field field) const
{
    switch(field)
    {
        case SIGNATURE_TYPE: return getSignatureType();
        case SERVICE_SIGNATURE: return getServiceSignature();
        case SERVICE_ADDRESS: return getServiceAddress();
        default: assert(-1);
    }

    return "";
}

std::string ServiceLocation::toString() const
{
    std::string representation = mServiceAddress + " "  + mSignatureType + " " + mServiceSignature;
    boost::trim(representation);
    return representation;
}

ServiceLocation ServiceLocation::fromString(const std::string& locationString)
{
    ServiceLocation location;
    std::vector<std::string> fieldTokens;
    boost::split(fieldTokens, locationString, boost::is_any_of(" "));

    switch(fieldTokens.size())
    {
        case 3:
            location.mServiceSignature = fieldTokens[2];
        case 2:
            location.mSignatureType = fieldTokens[1];
        case 1:
            location.mServiceAddress = fieldTokens[0];
        default:
            break;
    }

    return location;
}

bool ServiceLocation::operator==(const ServiceLocation& other)
{
    return mServiceAddress == other.mServiceAddress && mServiceSignature == other.mServiceSignature && mSignatureType == other.mSignatureType;
}

void ServiceLocator::updateFromString(const std::string& locations)
{
    std::vector<std::string> locationTokens;
    boost::split(locationTokens, locations, boost::is_any_of(";"));

    std::vector<std::string>::const_iterator cit = locationTokens.begin();
    for(; cit != locationTokens.end(); ++cit)
    {
        ServiceLocation location = ServiceLocation::fromString(*cit);
        mLocations.push_back(location);
    }
}

ServiceLocator ServiceLocator::fromString(const std::string& locations)
{
    ServiceLocator locator;
    locator.updateFromString(locations);
    return locator;
}

std::string ServiceLocator::toString() const
{
    std::string description;
    ServiceLocations::const_iterator cit = mLocations.begin();
    for(; cit != mLocations.end(); ++cit)
    {
        description += cit->toString();
        description += ";";
    }
    boost::trim(description);

    return description;
}

ServiceLocations ServiceLocator::search(const std::string& regex, ServiceLocation::Field field)
{
    ServiceLocations serviceLocations;
    ServiceLocations::const_iterator it = mLocations.begin();

    boost::regex r(regex);
    boost::smatch what;
    for(; it != mLocations.end(); ++it)
    {
        ServiceLocation entry = *it;

        if(boost::regex_match( entry.getFieldContent(field), what,r))
        {
            serviceLocations.push_back(entry);
        }
    }
    if(serviceLocations.empty())
    {
        throw NotFound("ServiceLocation matching '" + regex + "'");
    } else {
        return serviceLocations;
    }
}

} // end namespace services
} // end namespace fipa
