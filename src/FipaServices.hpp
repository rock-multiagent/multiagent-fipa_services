#ifndef FIPA_SERVICES_HPP
#define FIPA_SERVICES_HPP
#include <fipa_services/MessageTransport.hpp>
#include <fipa_services/DistributedServiceDirectory.hpp>
/*! \mainpage FIPA Services Library
 *
 * This library is collection of services whose main drivers and specification
 * is given by FIPA.
 *
 * The collection does represent a selection of services which are needed to implement
 * the FIPA Abstract Service Architecture. While we try to comply to the semantics given
 * in the FIPA specifications, the focus lies on practicality in case of doubt.
 *
 * This library currently implements fipa::services::message_transport::MessageTransport which can
 * be used with or without UDT (http://udt.sourceforge.net).
 * Additional services are fipa::services::ServiceDirectory and fipa::services::DistributedServiceDirectory
 * which relies on Avahi (http://avahi.org/).
 *
 * \section example Usage examples
 * \subsection ServiceDiscovery
 * The distributed service directory implements a yellow and white pages system.
 * It allows to search for services of a given name and also services that match
 * a certain description.
 *
 * The distributed service directory relies on the service_discovery library
 * which uses avahi for detecting distributed services.
 * A scope is mandatory (here: _fipa_service_directory._udp) and focuses
 * the search on services that fall under this scope.
 *
 * \verbatim
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
 \endverbatim
 *
 * \subsection MessageTransport
 * \verbatim

 #include <functional>
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
                                             std::bind(&Postman::deliverLetterLocally, &postman,_1,_2));

     ...
     return 0;
 }

 \endverbatim
 */
#endif // FIPA_SERVICES_HPP
