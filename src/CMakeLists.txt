find_package(Boost COMPONENTS regex unit_test_framework REQUIRED)

rock_library(fipa_services
    SOURCES 
        MessageTransport.cpp
        ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
    HEADERS 
        MessageTransport.hpp
        ServiceDirectoryEntry.hpp
        ServiceDirectory.hpp
    LIBS ${Boost_REGEX_LIBRARY}
    DEPS_PKGCONFIG base-lib fipa_acl
)

#rock_executable(fipa_services_bin Main.cpp
#    DEPS fipa_services)

