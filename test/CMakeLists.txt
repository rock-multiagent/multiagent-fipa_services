find_package(Boost COMPONENTS unit_test_framework)
rock_executable(${PROJECT_NAME}_test 
    SOURCES test.cpp 
        service_directory_test.cpp
        message_transport_test.cpp
        udt_transport_test.cpp
    DEPS ${PROJECT_NAME}
    LIBS ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    NOINSTALL
)

