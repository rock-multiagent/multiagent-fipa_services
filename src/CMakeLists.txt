find_package(Boost COMPONENTS regex unit_test_framework REQUIRED)

rock_library(fipa_services
    SOURCES ServiceDirectory.cpp
        ServiceDirectoryEntry.cpp
    HEADERS ServiceDirectoryEntry.hpp
        ServiceDirectory.hpp
    LIBS ${Boost_REGEX_LIBRARY}
)

#rock_executable(fipa_services_bin Main.cpp
#    DEPS fipa_services)

