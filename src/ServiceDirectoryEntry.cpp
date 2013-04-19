#include "ServiceDirectoryEntry.hpp"

namespace fipa {
namespace service {

ServiceDirectoryEntry::ServiceDirectoryEntry()
    : mName()
    , mType()
    , mLocator()
    , mDescription()
{}

ServiceDirectoryEntry::ServiceDirectoryEntry(const Name& name, const Type& type, const Locator& locator, const Description& description)
    : mName(name)
    , mType(type)
    , mLocator(locator)
    , mDescription(description)
{}

} // end namespace service
} // end namespace fipa
