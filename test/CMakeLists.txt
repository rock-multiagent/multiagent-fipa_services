find_package(Boost COMPONENTS unit_test_framework)
rock_executable(${PROJECT_NAME}_test 
    SOURCES Test.cpp
        DistributedServiceDirectoryTest.cpp
        MessageTransportTest.cpp
        ServiceDirectoryTest.cpp
        UDTTransportTest.cpp
        TCPTransportTest.cpp
    DEPS ${PROJECT_NAME}
    LIBS ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    NOINSTALL
)

