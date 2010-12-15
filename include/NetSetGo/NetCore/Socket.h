#ifndef SOCKET_H
#define SOCKET_H

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/NetCoreExport.h>

#include <NetSetGo/NetCore/Address.h>
#include <NetSetGo/NetCore/NetPlatform.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

// socket functionality

/**
 * These calls must be issued to startup and shutdown socket capability
 * generally. Note that the constructor of NetworkEngine, a singleton, calls
 * these for us so only when we are not using the NetworkEngine do we need to
 * issue these calls directly.
 */
bool NETCORE_EXPORT InitializeSockets();
void NETCORE_EXPORT ShutdownSockets();

/**
 * Socket
 *
 * An object-oriented wrapper for a low-level network socket. Note that this can
 * be used for broad-casting or multi-casting.
 */
class NETCORE_EXPORT Socket
{
public:
   enum Options // | these together as desired
   {
      NonBlocking    = 1 << 0,
      Broadcast      = 1 << 1,
      AllowMultiBind = 1 << 2
   };

   Socket(int options = NonBlocking);
   ~Socket();

   bool Open(unsigned short port);
   void Close();
   bool IsOpen() const;
   unsigned short GetPort() const { return mPort; }

   bool Send(const net::Address& destination, const void* data, int size);
   int Receive(net::Address& sender, void* data, int size); // returns number of bytes read

   /** multi-cast calls
    * Think of multi-cast signals as a newsletter, and we can
    * subscribe and unsubscribe to this newsletter.
    *
    * Note that net::Address::IsMulticastAddress() can be used
    * to confirm an address is a multi-cast address.
    *
    * Also note that one does not have to be subscribed in order
    * to send to a multi-cast address.
    */
   bool Subscribe(const net::Address& multicastAddress);
   bool Unsubscribe(const net::Address& multicastAddress);

private:
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
   typedef unsigned int* SocketExternalType;
#else
   typedef int SocketExternalType;
#endif

   SocketExternalType mSocket;
   int mOptions;
   unsigned short mPort;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // SOCKET_H
