#include "IncomingConnection.hpp"

namespace fipa {
namespace services {
namespace transports {
namespace udt {

IncomingConnection::IncomingConnection()
{}

IncomingConnection::~IncomingConnection()
{
    UDT::close(mSocket);
}

IncomingConnection::IncomingConnection(const UDTSOCKET& socket, const std::string& ip, uint16_t port)
    : Connection(ip, port)
    , mSocket(socket)
{
}

int IncomingConnection::receiveMessage(char* buffer, size_t size) const
{
    int result = UDT::recvmsg(mSocket, buffer, size);
    if(result == UDT::ERROR)
    {
        switch(UDT::getlasterror().getErrorCode())
        {
            case 6002: // no data is available to be received on a non-blocking socket
                break;
            case 2001: // connection broken before send is completed
            case 2002: // not connected
            case 5004: // invalid UDT socket
            case 5009: // wrong mode
            case 6003: // no buffer available for the non-blocing rcv call
            case 6004: // an overlapped recv is in progress
            default:
                throw std::runtime_error("fipa::services::udt::IncomingConnection: " + std::string(UDT::getlasterror().getErrorMessage()));
        }
    }

    return result;
}

} // end namespace udt
} // end namespace transport
} // end namespace services
} // end namespace fipa
