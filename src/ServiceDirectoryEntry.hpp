#ifndef FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP
#define FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP

#include <vector>
#include <map>
#include <string>
#include <base/Time.hpp>
#include <fipa_services/ServiceLocator.hpp>

namespace fipa {
namespace services {

typedef std::string Name;
typedef std::string Type;
typedef std::string Description;

// http://www.fipa.org/specs/fipa00001/SC00001L.html#_Toc26668641

class ServiceDirectoryEntry
{
    Name mName;
    Type mType;
    ServiceLocator mLocator;
    Description mDescription;
    base::Time mTimestamp;

public:

    enum Field { NAME = 0, TYPE,  LOCATOR, DESCRIPTION, TIMESTAMP, END_MARKER };
    static std::map<Field, std::string> FieldTxt;

    ServiceDirectoryEntry();

    ServiceDirectoryEntry(const Name& name, const Type& type, const ServiceLocator& locator, const Description& description);

    // Setter and getter for properties
    Name getName() const { return mName; }

    void setName(const Name& name) { mName = name; }

    /**
     * The signature type
     */
    Type getType() const { return mType; }

    void setType(const Type& type) { mType = type; }

    /**
     * Get locator
     */
    ServiceLocator getLocator() const { return mLocator; }

    void setLocator(const ServiceLocator& locator) { mLocator = locator; }

    Description getDescription() const { return mDescription; }

    void setDescription(const Description& description) { mDescription = description; }

    base::Time getTimestamp() const { return mTimestamp; }

    /**
     * Update the modification times of this
     * entry
     */
    void updateTimestamp() { mTimestamp = base::Time::now(); }

    /**
     * Get the string content of field based on the field identifier
     * \return String content of the selected field
     */
    std::string getFieldContent(ServiceDirectoryEntry::Field field) const;

    /**
     * Set the field content of field based on the field identifier
     * \throws std::runtime_error if parsing string content fails
     */
    void setFieldContent(ServiceDirectoryEntry::Field field, const std::string& content);

    /**
     * Comparison operator to allow usage as map key
     */
    bool operator<(const ServiceDirectoryEntry& other) const { return this->getName() < other.getName(); }

};

typedef std::vector<ServiceDirectoryEntry> ServiceDirectoryList;
typedef std::map<Name, ServiceDirectoryEntry> ServiceDirectoryMap;

} // end namespace services
} // end namespace fipa

#endif // FIPA_SERVICES_SERVICE_DIRECTORY_ENTRY_HPP
