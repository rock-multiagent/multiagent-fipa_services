find_package(Boost COMPONENTS thread regex unit_test_framework REQUIRED)

rock_library(fipa_services
    SOURCES 
        DistributedServiceDirectory.cpp
        MessageTransport.cpp
        ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
        ServiceLocator.cpp
        transports/udt/UDTTransport.cpp
    HEADERS 
        DistributedServiceDirectory.hpp
        ErrorHandling.hpp
        MessageTransport.hpp
        ServiceDirectoryEntry.hpp
        ServiceDirectory.hpp
        ServiceLocator.hpp
        transports/udt/UDTTransport.hpp
    LIBS ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY} udt
    DEPS_PKGCONFIG base-lib fipa_acl service_discovery
)

