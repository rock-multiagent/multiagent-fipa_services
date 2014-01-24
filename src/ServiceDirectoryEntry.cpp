#include "ServiceDirectoryEntry.hpp"

namespace fipa {
namespace services {

ServiceDirectoryEntry::ServiceDirectoryEntry()
    : timestamp( base::Time::now() )
{}

ServiceDirectoryEntry::ServiceDirectoryEntry(const Name& name, const Type& type, const Locator& locator, const Description& description)
    : name(name)
    , type(type)
    , locator(locator)
    , description(description)
    , timestamp( base::Time::now() )
{}

std::string ServiceDirectoryEntry::getFieldContent(ServiceDirectoryEntry::Field field) const
{
    switch(field)
    {
        case NAME: return getName();
        case TYPE: return getType();
        case LOCATOR: return getLocator();
        case DESCRIPTION: return getDescription();
        default: assert(-1);
    }

    return "";
}

} // end namespace services
} // end namespace fipa
