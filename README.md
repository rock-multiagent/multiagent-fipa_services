# FIPA Services
This library is collection of services whose main drivers and specification is given by FIPA.

The collection does represent a selection of services which are needed to implement the FIPA Abstract Service Architecture. While we try to comply to the semantics given in the FIPA specifications, the focus lies on practicality in case of doubt.

This library currently implements fipa::services::message_transport::MessageTransport which can be used with or without UDT (http://udt.sourceforge.net). Additional services are fipa::services::ServiceDirectory and fipa::services::DistributedServiceDirectory which relies on Avahi (http://avahi.org/).

## Documentation
To generate the doxygen based documentation you should first boostrap this package as part of an autoproj installation (see http://rock-robotics.org for more information).
When all dependencies are available (and doxygen has been installed) you can do:
```
 mkdir build; cd build
 cmake ..
 make doc
```

## Usage examples
### ServiceDiscovery

The distributed service directory implements a yellow and white pages system. It allows to search for services of a given name and also services that match a certain description.

The distributed service directory relies on the service_discovery library which uses avahi for detecting distributed services. A scope is mandatory (here: _fipa_service_directory._udp) and focuses the search on services that fall under this scope.

```
    #include <fipa_services/DistributedServiceDirectory.hpp>

    ServiceDirectory::Ptr serviceDirectory(new DistributedServiceDirectory("_fipa_service_directory._udp"));
    bool doThrow = false;
    std::string regexp = ".*";
    ServiceDirectoryList allServices = serviceDirectory->search(regexp, ServiceDirectoryEntry::NAME, doThrow);

    ServiceDirectoryList::const_iterator cit = allServices.begin();
    for(; cit != allServices.end(); ++cit)
    {
       printf("Service: %s\n", cit->getName());
    }
```

### MessageTransport

```
#include <boost/bind.hpp>
#include <fipa_services/MessageTransport.hpp>
#include <fipa_services/DistributedServiceDirectory.hpp>

class Postman
{
public:
    bool deliverLetterLocally(const std::string& receiverName, const fipa::acl::Letter& letter)
    {
        // deliver letter to receiver of given name
        return true;
    }
};

...

int main()
{
    using namespace fipa::services;

    fipa::acl::AgentID serviceName("mts-name");
    ServiceDirectory::Ptr serviceDirectory(new DistributedServiceDirectory("_fipa_service_directory._udp"));
    message_transport::MessageTransport messageTransport(serviceName, serviceDirectory);


    // Transport activation based on the selected set of transports
    std::vector<std::string> transports;
    transports.push_back("UDT");
    messageTransport->activateTransports(transports);

    // Register local message delivery
    Postman postman;
    messageTransport->registerMessageTransport("local-delivery",
                                            boost::bind(&Postman::deliverLetterLocally, &postman,_1,_2));

    ...
    return 0;
}
```
## License
FIPA Services is distributed under the [LGPL license v2 or
later](https://www.gnu.org/licenses/old-licenses/lgpl-2.0.txt)

## Maintainer / Authors / Contributors
 * see manifest.xml

Copyright 2013-2017 DFKI GmbH / Robotics Innovation Center
