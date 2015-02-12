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
    /// The service address
    std::string mServiceAddress;
    /// The signature type is describes the 'type' of a
    /// service signature [optional element]
    std::string mSignatureType;
    /// According to FIPA a fully qualifified name that describes the binding
    /// signature of this service, e.g. org.omg.agent.idl-binding
    std::string mServiceSignature;

public:
    enum Field { SIGNATURE_TYPE = 0x01, SERVICE_SIGNATURE = 0x02, SERVICE_ADDRESS = 0x04 };
    
    /**
     * The default constructor for service location
     */
    ServiceLocation() {}

    ServiceLocation(const std::string& serviceAddress, const std::string& signatureType = "", const std::string& serviceSignature = "")
        : mServiceAddress(serviceAddress)
        , mSignatureType(signatureType)
        , mServiceSignature(serviceSignature)
    {}

    /**
     * Get the content of a specific field
     * \param field Field for which the value should be retrieved
     * \return content of field
     */
    std::string getFieldContent(ServiceLocation::Field field) const;

    /**
     * Get the signature type of this service location
     * \return signature type
     */
    std::string getSignatureType() const { return mSignatureType; }

    /**
     * Get the service signature
     * \return service signature
     */
    std::string getServiceSignature() const { return mServiceSignature; }

    /**
     * Get the service address
     * \return service address
     */
    std::string getServiceAddress() const { return mServiceAddress; }

    /**
     * Convert to ServiceLocation to string
     * \return stringified objects
     */
    std::string toString() const;

    /**
     * Create service location from string
     * \param stringified service location
     */
    static ServiceLocation fromString(const std::string& locationString);

    /**
     * Equals operator for service location
     * \return true if service locations are equal
     */
    bool operator==(const ServiceLocation& other) const;

    /**
     * Helper function to retrieve the underlying transport 
     * protocol that is part of the service address
     */
    std::string getProtocol() const;
};

typedef std::vector<ServiceLocation> ServiceLocations;

/**
 * \class ServiceLocator
 * \brief A ServiceLocator is a container for ServiceLocations
 * \details This class serves as a container and provides some convenience functions for updating information from string
 */
class ServiceLocator
{
    /// Service locations that are pare of this container
    ServiceLocations mLocations;

public:
    ServiceLocator() {}

    ServiceLocator(const ServiceLocations& locations);

    ServiceLocations getLocations() const { return mLocations; }

    /**
     * Add a service location
     * \param location A service location that shall be added to the container,
     * does not check on duplicates
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
     * \param regex Regex that is matched according to the parameter 'field'
     * \param field Field to which the first parameter ('regex') should be
     * applied
     * \return List of matches
     * \throws NotFound if not locations could be found
     */
    ServiceLocations search(const std::string& regex, ServiceLocation::Field field = ServiceLocation::SIGNATURE_TYPE);

    /**
     * Parses a string using ';' as separators and
     * updates the ServiceLocator object
     * \param locations List of ';' separated locations
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
     * <address-0> <signature-type-0> <service-signature-0>;<address-1> <...>
     * \return stringified object
     */
    std::string toString() const;
};

} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_SERVICE_LOCATOR_HPP
