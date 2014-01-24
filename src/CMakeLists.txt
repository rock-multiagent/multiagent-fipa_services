find_package(Boost COMPONENTS regex unit_test_framework REQUIRED)

rock_library(fipa_services
    SOURCES 
        MessageTransport.cpp
        ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
    HEADERS 
        ErrorHandling.hpp
        MessageTransport.hpp
        ServiceDirectoryEntry.hpp
        ServiceDirectory.hpp
    LIBS ${Boost_REGEX_LIBRARY}
    DEPS_PKGCONFIG base-lib fipa_acl
)

