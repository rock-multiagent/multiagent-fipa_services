#ifndef FIPA_SERVICES_TRANSPORTS_CONFIGURATION_HPP
#define FIPA_SERVICES_TRANSPORTS_CONFIGURATION_HPP

#include <string>
#include <stdint.h>

namespace fipa {
namespace services {
namespace transports {
/**
 * Struct to configure different transports, i.e. to set a fix listening port.
 */
struct Configuration
{
    std::string transport_type;
    uint16_t listening_port;
    uint32_t maximum_clients;
    
    /**
     * Default values.
     */
    Configuration()
        : transport_type("UNKNOWN")
        , listening_port(0)
        , maximum_clients(50)
    {}
};

} // end namespace transports
} // end namespace services
} // end namespace fipa
#endif // FIPA_SERVICES_TRANSPORTS_CONFIGURATION_HPP

