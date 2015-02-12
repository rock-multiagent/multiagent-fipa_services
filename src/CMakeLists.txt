find_package(Boost COMPONENTS thread regex system unit_test_framework REQUIRED)

rock_library(fipa_services
    SOURCES 
        DistributedServiceDirectory.cpp
        MessageTransport.cpp
        ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
        ServiceLocator.cpp
        transports/Address.cpp
        transports/Connection.cpp
        transports/OutgoingConnection.cpp
        transports/Transport.cpp
        transports/udt/UDTTransport.cpp
        transports/udt/OutgoingConnection.cpp
        transports/udt/IncomingConnection.cpp
    HEADERS 
        DistributedServiceDirectory.hpp
        ErrorHandling.hpp
        FipaServices.hpp
        MessageTransport.hpp
        ServiceDirectoryEntry.hpp
        ServiceDirectory.hpp
        ServiceLocator.hpp
        transports/Address.hpp
        transports/Configuration.hpp
        transports/Connection.hpp
        transports/OutgoingConnection.hpp
        transports/Transport.hpp
        transports/tcp/SocketTransport.hpp
        transports/udt/UDTTransport.hpp
        transports/udt/OutgoingConnection.hpp
        transports/udt/IncomingConnection.hpp
    LIBS ${Boost_REGEX_LIBRARIES} ${Boost_THREAD_LIBRARIES} ${Boost_SYSTEM_LIBRARIES} udt
    DEPS_PKGCONFIG base-lib fipa_acl service_discovery
)

