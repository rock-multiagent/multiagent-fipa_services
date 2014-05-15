#ifndef FIPA_SERVICES_SERVICE_LOCATOR_HPP
#define FIPA_SERVICES_SERVICE_LOCATOR_HPP

#include <vector>
#include <string>

namespace fipa {
namespace services {

/**
 * \class ServiceLocation
 * \brief Class to describe the location of a service
 */
class ServiceLocation
{
    std::string mServiceAddress;
    std::string mSignatureType;
    std::string mServiceSignature;

public:
    enum Field { SIGNATURE_TYPE = 0x01, SERVICE_SIGNATURE = 0x02, SERVICE_ADDRESS = 0x04 };
    
    ServiceLocation() {}

    ServiceLocation(const std::string& serviceAddress, const std::string& signatureType = "", const std::string& serviceSignature = "")
        : mServiceAddress(serviceAddress)
        , mSignatureType(signatureType)
        , mServiceSignature(serviceSignature)
    {}

    std::string getFieldContent(ServiceLocation::Field field) const;

    std::string getSignatureType() const { return mSignatureType; }
    std::string getServiceSignature() const { return mServiceSignature; }
    std::string getServiceAddress() const { return mServiceAddress; }

    std::string toString() const;
    static ServiceLocation fromString(const std::string& locationString);

    /**
     * Equals operator for service location
     * \return true if service locations are equal
     */
    bool operator==(const ServiceLocation& other) const;
};

typedef std::vector<ServiceLocation> ServiceLocations;

/**
 * \class ServiceLocator
 * \brief A ServiceLocator is a container for ServiceLocations
 * \details This class serves as a container and provides some convenience functions for updating information from string
 */
class ServiceLocator
{
    ServiceLocations mLocations;

public:
    ServiceLocator() {}

    ServiceLocator(const ServiceLocations& locations);

    ServiceLocations getLocations() const { return mLocations; }

    /**
     * Add a service location
     */
    void addLocation(const ServiceLocation& location) { mLocations.push_back(location); }

    /**
     * Check if this locator contains any locations
     * \return true if locations exists, false otherwise
     */
    bool hasLocations() const { return mLocations.empty(); }

    /**
     * Get the first location, i.e. the one with highest priority
     * \return ServiceLocation of highest priority
     */
    ServiceLocation getFirstLocation() const { return mLocations.front(); }

    /**
     * Search for a service location, e.g. given a certain signature type
     * \return List of matches
     * \throws NotFound if not locations could be found
     */
    ServiceLocations search(const std::string& regex, ServiceLocation::Field field = ServiceLocation::SIGNATURE_TYPE);

    /**
     * Parses a string using ';' as separators and
     * updates the ServiceLocator object
     */
    void updateFromString(const std::string& locations);

    /**
     * Create a ServiceLocator object from an existing
     * string
     * \see toString for valid separators in the string
     * \return the ServiceLocator object
     */
    static ServiceLocator fromString(const std::string& locations);

    /**
     * Create string representation of service locator
     * <address>;<signature-type>;<service-signature>
     */
    std::string toString() const;
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_SERVICE_LOCATOR_HPP
