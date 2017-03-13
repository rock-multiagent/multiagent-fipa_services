#include "Configuration.hpp"
namespace fipa {
namespace services {
namespace transports {

Configuration::Configuration()
    : transport_type("UNKNOWN")
    , listening_port(0)
    , maximum_clients(50)
    , ttl(-1)
{}

Configuration::Configuration(const std::string& type,
        uint16_t listening_port,
        uint32_t maximum_clients,
        int ttl)
    : transport_type(type)
    , listening_port(listening_port)
    , maximum_clients(maximum_clients)
    , ttl(ttl)
{}

} // end namespace transports
} // end namespace services
} // end namespace fipa
