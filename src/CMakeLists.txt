find_package(Boost COMPONENTS thread regex system unit_test_framework REQUIRED)

# Check whether there is a pkg-config information if not try direct linking
pkg_check_modules(UDT "udt")
if(UDT_FOUND)
    message(STATUS "-- found UDT using pkg-config: ${UDT_LIBRARIES}")
else()
    find_library(UDT NAMES "udt" HINTS "/usr/lib")
    if(UDT)
        set(UDT_FOUND true)
        set(UDT_LIBRARIES ${UDT})
    else()
        add_definitions(-DTRANSPORT_UDT_UNSUPPORTED)
        message(STATUS " -- could not find UDT library: ${UDT} -- compiling"
            " without out UDT support")
    endif()
endif()

rock_library(fipa_services
    SOURCES 
        DistributedServiceDirectory.cpp
        MessageTransport.cpp
        ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
        ServiceLocator.cpp
        transports/Address.cpp
        transports/Configuration.cpp
        transports/Connection.cpp
        transports/OutgoingConnection.cpp
        transports/Transport.cpp
        transports/tcp/OutgoingConnection.cpp
        transports/tcp/TCPTransport.cpp
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
        transports/tcp/OutgoingConnection.hpp
        transports/tcp/TCPTransport.hpp
        transports/udt/UDTTransport.hpp
        transports/udt/OutgoingConnection.hpp
        transports/udt/IncomingConnection.hpp
    LIBS ${Boost_REGEX_LIBRARIES} ${Boost_THREAD_LIBRARIES} ${Boost_SYSTEM_LIBRARIES}
    DEPS_PKGCONFIG base-lib fipa_acl service_discovery
)

if(UDT_FOUND)
    message(STATUS "-- found UDT library: ${UDT_LIBRARIES}")
    target_link_libraries(fipa_services ${UDT_LIBRARIES})
endif()
