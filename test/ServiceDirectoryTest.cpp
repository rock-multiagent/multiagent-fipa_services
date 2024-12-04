#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fipa_services/ServiceDirectory.hpp>
BOOST_AUTO_TEST_SUITE(service_directory)

std::string getProtocolPath()
{
    char buffer[1024];
    BOOST_REQUIRE_MESSAGE( readlink("/proc/self/exe", buffer, 1024) != -1, "Retrieving current execution path");
    std::string str(buffer);
    std::string executionDir = str.substr(0, str.rfind('/'));
    // Assuming we have do a build into build/ parallel to src/ 
    std::string configurationPath = executionDir + "/../../../../configuration/protocols";
    return configurationPath;
}

BOOST_AUTO_TEST_CASE(simple_test)
{
    using namespace fipa::services;

    Name name("test-name");
    Type type;
    ServiceLocator locator;
    Description description;


    ServiceDirectoryEntry entry(name, type, locator, description);

    // Modify only the type
    Type otherType("other-type");
    ServiceDirectoryEntry otherEntry(name, otherType, locator, description);

    ServiceDirectory sd;
    BOOST_REQUIRE_THROW(sd.deregisterService(entry), std::runtime_error);
    BOOST_REQUIRE_NO_THROW(sd.registerService(entry));
    BOOST_REQUIRE_NO_THROW(sd.modify(otherEntry));

    ServiceDirectoryList list = sd.search(otherEntry);
    BOOST_REQUIRE(list[0].getName() == name);
    
}

BOOST_AUTO_TEST_CASE(regex_matching)
{
    using namespace fipa::services;

    ServiceDirectory sd;
    {
        Name name("test-A");
        Type type;
        ServiceLocator locator;
        Description description;
        ServiceDirectoryEntry entry(name, type, locator, description);
        // Modify only the type
        Type otherType("other-type");
        ServiceDirectoryEntry otherEntry(name, otherType, locator, description);
        BOOST_REQUIRE_NO_THROW(sd.registerService(entry));
    }
    {
        Name name("test-B");
        Type type;
        ServiceLocator locator;
        Description description;
        ServiceDirectoryEntry entry(name, type, locator, description);
        // Modify only the type
        Type otherType("other-type");
        ServiceDirectoryEntry otherEntry(name, otherType, locator, description);
        BOOST_REQUIRE_NO_THROW(sd.registerService(entry));
    }

    ServiceDirectoryList list = sd.search(".*$", ServiceDirectoryEntry::NAME);
    BOOST_REQUIRE(list.size() == 2);
}


BOOST_AUTO_TEST_SUITE_END()
