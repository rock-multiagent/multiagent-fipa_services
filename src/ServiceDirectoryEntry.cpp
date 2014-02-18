#include "ServiceDirectoryEntry.hpp"
#include <boost/assign/list_of.hpp>
#include <base/Logging.hpp>

namespace fipa {
namespace services {

std::map<ServiceDirectoryEntry::Field, std::string> ServiceDirectoryEntry::FieldTxt = boost::assign::map_list_of
    (ServiceDirectoryEntry::NAME, "NAME")
    (ServiceDirectoryEntry::TYPE, "TYPE")
    (ServiceDirectoryEntry::LOCATOR, "LOCATOR")
    (ServiceDirectoryEntry::DESCRIPTION, "DESCRIPTION")
    (ServiceDirectoryEntry::TIMESTAMP, "TIMESTAMP")
    ;

ServiceDirectoryEntry::ServiceDirectoryEntry()
{}

ServiceDirectoryEntry::ServiceDirectoryEntry(const Name& name, const Type& type, const ServiceLocator& locator, const Description& description)
    : mName(name)
    , mType(type)
    , mLocator(locator)
    , mDescription(description)
    , mTimestamp( base::Time::now() )
{}

std::string ServiceDirectoryEntry::getFieldContent(ServiceDirectoryEntry::Field field) const
{
    switch(field)
    {
        case NAME: return getName();
        case TYPE: return getType();
        case LOCATOR: return getLocator().toString();
        case DESCRIPTION: return getDescription();
        case TIMESTAMP: return getTimestamp().toString();
        default: assert(-1);
    }

    return "";
}

void ServiceDirectoryEntry::setFieldContent(ServiceDirectoryEntry::Field field, const std::string& content)
{
    try {
        switch(field)
        {
            case NAME:
                mName = content;
                break;
            case TYPE:
                mType = content;
                break;
            case LOCATOR:
                mLocator = ServiceLocator::fromString(content);
                break;
            case DESCRIPTION:
                mDescription = content;
                break;
            case TIMESTAMP:
                mTimestamp = base::Time::fromString(content);
                break;
            default:
                assert(-1);
        }
    } catch(const std::runtime_error& e)
    {
        LOG_WARN("Failed to convert field content: field '%s', content '%s'", ServiceDirectoryEntry::FieldTxt[field].c_str(), content.c_str());
        throw;
    }
}

std::string ServiceDirectoryEntry::toString() const
{
    std::string txt;
    txt += "ServiceDirectoryEntry: " + getName() + "\n";
    txt += "    type: " + getType() + "\n";
    txt += "    locator: " + getLocator().toString() + "\n";
    txt += "    description: " + getLocator().toString() + "\n";

    return txt;
}

} // end namespace services
} // end namespace fipa
