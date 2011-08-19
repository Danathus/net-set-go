#include <NetSetGo/NetCore/Socket.h>

#if NET_PLATFORM == NET_PLATFORM_WINDOWS
#   include <WinSock2.h>
#   include <Ws2tcpip.h>
#else
#   include <netdb.h>
#   include <fcntl.h>
#endif

#include <cassert>
#include <stdio.h>

#if NET_PLATFORM == NET_PLATFORM_WINDOWS
typedef SOCKET SocketInternalType;
#else
typedef int SocketInternalType;
#endif

////////////////////////////////////////////////////////////////////////////////

static bool sgSocketsInitialized = false;

bool net::InitializeSockets()
{
   bool result = false;

   if (!sgSocketsInitialized)
   {
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
      WSADATA WsaData;

      int resultCode = WSAStartup(MAKEWORD(2,2), &WsaData);
      result = (resultCode == NO_ERROR);

      // Print out errors
      if (!result)
      {
         printf("Winsock startup failed: WSAStartup code %d", result);
      }
#else
      result = true;
#endif
   }

   sgSocketsInitialized = result;
   return result;
}

void net::ShutdownSockets()
{
   if (sgSocketsInitialized)
   {
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
      WSACleanup();
#endif
   }
   sgSocketsInitialized = false;
}

////////////////////////////////////////////////////////////////////////////////

net::Socket::Socket(int options)
   : mPort(0)
{
   mOptions = options;
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
   mSocket = SocketExternalType(INVALID_SOCKET);
#else
   mSocket = -1;
#endif
}

net::Socket::~Socket()
{
   Close();
}

bool net::Socket::Open(unsigned short port)
{
   assert(!IsOpen());

   // create socket

   // note: PF_INET (protocol family) is for sockets, AF_INET (address family) is for addresses (both the same, for now...)
   mSocket = SocketExternalType(::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP));

   if (!IsOpen())
   {
      printf("failed to create socket\n");
      return false;
   }

   // bind to port

   sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons((unsigned short)port);
   mPort = port;

   if (bind(SocketInternalType(mSocket), (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
   {
      printf("failed to bind socket\n");
      Close();
      return false;
   }

   // set non-blocking io

   if (mOptions & NonBlocking)
   {
#if NET_PLATFORM == NET_PLATFORM_MAC || NET_PLATFORM == NET_PLATFORM_UNIX

      int nonBlocking = 1;
      if (fcntl(mSocket, F_SETFL, O_NONBLOCK, nonBlocking) == -1)
      {
         printf("failed to set non-blocking socket\n");
         Close();
         return false;
      }

#elif NET_PLATFORM == NET_PLATFORM_WINDOWS

      DWORD nonBlocking = 1;
      if (ioctlsocket(SocketInternalType(mSocket), FIONBIO, &nonBlocking) != 0)
      {
         printf("failed to set non-blocking socket\n");
         Close();
         return false;
      }

#endif
   }

   // set broadcast socket

   if (mOptions & Broadcast)
   {
      int enable = 1;
      if (setsockopt(SocketInternalType(mSocket), SOL_SOCKET, SO_BROADCAST, (const char*)&enable, sizeof(enable)) < 0)
      {
         printf("failed to set socket to broadcast\n");
         Close();
         return false;
      }
   }

   // set reuse port to on to allow multiple binds per host
   if (mOptions & AllowMultiBind)
   {
      int enable = 1;
      if ((setsockopt(SocketInternalType(mSocket), SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable))) < 0)
      {
         printf("failed to set socket to allow multiple binds\n");
         return false;
      }
   }

   return true;
}

void net::Socket::Close()
{
   if (IsOpen())
   {
#if NET_PLATFORM == NET_PLATFORM_MAC || NET_PLATFORM == NET_PLATFORM_UNIX
      close(mSocket);
      mSocket = -1;
#elif NET_PLATFORM == NET_PLATFORM_WINDOWS
      closesocket(SocketInternalType(mSocket));
      mSocket = SocketExternalType(INVALID_SOCKET);
#endif
   }
}

bool net::Socket::IsOpen() const
{
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
   return mSocket != SocketExternalType(INVALID_SOCKET);
#else
   return mSocket >= 0;
#endif
}

bool net::Socket::Send(const net::Address& destination, const void* data, int size)
{
   assert(data);
   assert(size > 0);

   if (!IsOpen())
   {
      return false;
   }

   assert(destination.GetAddress() != 0);
   assert(destination.GetPort() != 0);

   sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = htonl(destination.GetAddress());
   address.sin_port = htons((unsigned short)destination.GetPort());

   const int sent_bytes = sendto(SocketInternalType(mSocket), (const char*)data, size, 0, (sockaddr*)&address, sizeof(sockaddr_in));
   const bool success = sent_bytes == size;

   return success;
}

int net::Socket::Receive(net::Address& sender, void* data, int size)
{
   assert(data);
   assert(size > 0);

   if (!IsOpen())
   {
      return false;
   }

#if NET_PLATFORM == NET_PLATFORM_WINDOWS
   typedef int socklen_t;
#endif

   sockaddr_in from;
   socklen_t fromLength = sizeof(from);

   int received_bytes = recvfrom(SocketInternalType(mSocket), (char*)data, size, 0, (sockaddr*)&from, &fromLength);

   if (received_bytes <= 0)
   {
      return 0;
   }

   unsigned int address = ntohl(from.sin_addr.s_addr);
   unsigned short port = ntohs(from.sin_port);

   sender = Address(address, port);

   return received_bytes;
}

bool net::Socket::Subscribe(const net::Address& multicastAddress)
{
   bool success = true;

   struct ip_mreq mc_req; // multicast request structure

   // construct an IGMP join request structure
   mc_req.imr_multiaddr.s_addr = htonl(multicastAddress.GetAddress());
   mc_req.imr_interface.s_addr = htonl(INADDR_ANY);

   // send an ADD MEMBERSHIP message via setsockopt
   if ((setsockopt(SocketInternalType(mSocket), IPPROTO_IP, IP_ADD_MEMBERSHIP,
      (const char*)&mc_req, sizeof(mc_req))) < 0)
   {
      success = false;
   }

   return success;
}

bool net::Socket::Unsubscribe(const net::Address& multicastAddress)
{
   bool success = true;

   struct ip_mreq mc_req; // multicast request structure

   // construct an IGMP join request structure
   mc_req.imr_multiaddr.s_addr = htonl(multicastAddress.GetAddress());
   mc_req.imr_interface.s_addr = htonl(INADDR_ANY);

   // send a DROP MEMBERSHIP message via setsockopt
   if ((setsockopt(SocketInternalType(mSocket), IPPROTO_IP, IP_DROP_MEMBERSHIP,
      (const char*)&mc_req, sizeof(mc_req))) < 0)
   {
      success = false;
   }

   return success;
}

////////////////////////////////////////////////////////////////////////////////
