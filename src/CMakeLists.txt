rock_library(fipa_services
    SOURCES Dummy.cpp
    HEADERS Dummy.hpp)

rock_executable(fipa_services_bin Main.cpp
    DEPS fipa_services)

