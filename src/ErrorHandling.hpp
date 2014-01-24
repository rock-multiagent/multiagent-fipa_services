#ifndef FIPA_SERVICE_ERROR_HANDLING_HPP
#define FIPA_SERVICE_ERROR_HANDLING_HPP

#include <string>
#include <stdexcept>

#define FIPA_SERVICE_EXCEPTION(NAME, BASE_MSG) \
    class NAME : public std::runtime_error {\
    public: \
       NAME() : std::runtime_error(BASE_MSG) \
       {}; \
       NAME(const std::string& msg) : std::runtime_error(BASE_MSG + msg) \
       {}; \
   };

namespace fipa {
namespace services {

FIPA_SERVICE_EXCEPTION(DuplicateEntry, "Entry already exists")
FIPA_SERVICE_EXCEPTION(NotFound, "Entry could not be found")
FIPA_SERVICE_EXCEPTION(NotImplemented, "Function has not been implemeted yet")

} // end namespace services
} // end namespace fipa

#endif // FIPA_SERVICE_ERROR_HANDLING_HPP
