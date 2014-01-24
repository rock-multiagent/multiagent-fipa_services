#ifndef FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP
#define FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP

#include <vector>
#include <map>
#include <string>
#include <base/Time.hpp>

namespace fipa {
namespace services {

typedef std::string Name;
typedef std::string Type;
typedef std::string Locator;
typedef std::string Description;

struct ServiceDirectoryEntry
{

    Name name;
    Type type;
    Locator locator;
    Description description;
    base::Time timestamp;

    enum Field { NAME = 0x01, TYPE = 0x02, LOCATOR = 0x04, DESCRIPTION = 0x08 };

    ServiceDirectoryEntry();

    ServiceDirectoryEntry(const Name& name, const Type& type, const Locator& locator, const Description& description);

    // Setter and getter for properties
    Name getName() const { return name; }

    void setName(const Name& name) { this->name = name; }

    Type getType() const { return type; }

    void setType(const Type& type) { this->type = type; }

    Locator getLocator() const { return locator; }

    void setLocator(const Locator& locator) { this->locator = locator; }

    Description getDescription() const { return description; }

    void setDescription(const Description& description) { this->description = description; }

    /**
     * Update the modification times of this
     * entry
     */
    void updateTimestamp() { timestamp = base::Time::now(); }

    std::string getFieldContent(ServiceDirectoryEntry::Field field) const;

};

typedef std::vector<ServiceDirectoryEntry> ServiceDirectoryList;
typedef std::map<Name, ServiceDirectoryEntry> ServiceDirectoryMap;

} // end namespace services
} // end namespace fipa

#endif // FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP
