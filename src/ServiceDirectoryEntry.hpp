#ifndef FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP
#define FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP

#include <vector>
#include <map>
#include <string>

namespace fipa {
namespace service
{

typedef std::string Name;
typedef std::string Type;
typedef std::string Locator;
typedef std::string Description;

class ServiceDirectoryEntry
{

private:
    Name mName;
    Type mType;
    Locator mLocator;
    Description mDescription;

public:

    ServiceDirectoryEntry();

    ServiceDirectoryEntry(const Name& name, const Type& type, const Locator& locator, const Description& description);

    // Setter and getter for properties
    Name getName() const { return mName; }

    void setName(const Name& name) { mName = name; }

    Type getType() const { return mType; }

    void setType(const Type& type) { mType = type; }

    Locator getLocator() const { return mLocator; }

    void setLocator(const Locator& locator) { mLocator = locator; }

    Description getDescription() const { return mDescription; }

    void setDescription(const Description& description) { mDescription = description; }

};

typedef std::vector<ServiceDirectoryEntry> ServiceDirectoryList;
typedef std::map<Name, ServiceDirectoryEntry> ServiceDirectoryMap;

} // end namespace service
} // end namespace fipa

#endif // FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP
