find_package(Boost COMPONENTS thread regex system unit_test_framework REQUIRED)

rock_library(fipa_services
    SOURCES 
        DistributedServiceDirectory.cpp
        MessageTransport.cpp
        ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
        ServiceLocator.cpp
        transports/tcp/SocketTransport.cpp
        transports/udt/UDTTransport.cpp
    HEADERS 
        DistributedServiceDirectory.hpp
        ErrorHandling.hpp
        FipaServices.hpp
        MessageTransport.hpp
        ServiceDirectoryEntry.hpp
        ServiceDirectory.hpp
        ServiceLocator.hpp
        transports/tcp/SocketTransport.hpp
        transports/udt/UDTTransport.hpp
    LIBS ${Boost_REGEX_LIBRARIES} ${Boost_THREAD_LIBRARIES} ${Boost_SYSTEM_LIBRARIES} udt
    DEPS_PKGCONFIG base-lib fipa_acl service_discovery
)

