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
 */
#endif // FIPA_SERVICES_HPP
