#include <NetCore/NetworkEngine.h>

#include <cassert>

#if NET_PLATFORM == NET_PLATFORM_WINDOWS
#   include <WinSock2.h>
#endif

net::NetworkEngine* net::NetworkEngine::sgSelf = 0;

namespace net {

////////////////////////////////////////////////////////////////////////////////
NetworkEngine& NetworkEngine::GetRef()
{
   if (!sgSelf)
   {
      sgSelf = new NetworkEngine();
   }
   return *sgSelf;
}


////////////////////////////////////////////////////////////////////////////////
void NetworkEngine::Destroy()
{
   delete sgSelf;
   sgSelf = NULL;
}


////////////////////////////////////////////////////////////////////////////////
std::string NetworkEngine::GetHostName()
{
   char buffer[1024];
   if (gethostname(buffer, sizeof(buffer)) == SOCKET_ERROR)
   {
      return "<ERROR>";
   }
   return std::string(buffer);
}

////////////////////////////////////////////////////////////////////////////////
net::Address NetworkEngine::GetAddressFromHostName(const std::string& hostName)
{
   struct hostent *host = gethostbyname(hostName.c_str());
   if (host == NULL)
   {
      return Address(0, 0);
   }

   // Obtain the computer's IP
   return Address(
      ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b1,
      ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b2,
      ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b3,
      ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b4,
      0
   );
}

////////////////////////////////////////////////////////////////////////////////
void NetworkEngine::HostNetwork()
{
   mAcceptConnection = true;
}

////////////////////////////////////////////////////////////////////////////////
void NetworkEngine::JoinNetwork(const net::Address& hostAddress)
{
   mAttemptConnection = true;
   mHostAddress = hostAddress;
}

////////////////////////////////////////////////////////////////////////////////
bool NetworkEngine::StartAdvertising(const std::string& name, unsigned int protocolID, unsigned int listenerPort, unsigned int serverPort, int beaconPort, Serializable* userData)
{
   mBeaconTransmitter.Configure(name, protocolID, listenerPort, serverPort, userData);
   const bool started = mBeaconTransmitter.Start(beaconPort);
   return started;
}

////////////////////////////////////////////////////////////////////////////////
void NetworkEngine::StopAdvertising()
{
   mBeaconTransmitter.Stop();
}

////////////////////////////////////////////////////////////////////////////////
void NetworkEngine::Update(float deltaTime)
{
   if (mBeaconTransmitter.IsRunning())
   {
      mBeaconTransmitter.Update(deltaTime);
   }

   //if (mNode.IsRunning())
   {
      mNode.Update(deltaTime);
   }
   // for now we prefer to update the mesh after the node...
   if (mMesh.IsRunning())
   {
      mMesh.Update(deltaTime);
   }
}

////////////////////////////////////////////////////////////////////////////////
NetworkEngine::NetworkEngine()
   : mAcceptConnection(false)
   , mAttemptConnection(false)
   , mBeaconTransmitter(NULL)
   , mNode(1234)
   , mMesh(1234)
{
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
   const bool success = net::InitializeSockets();
   assert(success);
#endif
}

////////////////////////////////////////////////////////////////////////////////
NetworkEngine::~NetworkEngine()
{
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
   net::ShutdownSockets();
#endif
}

////////////////////////////////////////////////////////////////////////////////

} // namespace net
