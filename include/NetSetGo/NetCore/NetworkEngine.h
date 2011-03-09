#ifndef NETWORK_ENGINE__H
#define NETWORK_ENGINE__H

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/NetCoreExport.h>

#include <NetSetGo/NetCore/Address.h>
#include <NetSetGo/NetCore/Beacon.h>
#include <NetSetGo/NetCore/Node.h>
#include <NetSetGo/NetCore/Mesh.h>
#include <NetSetGo/NetCore/PacketProcessor.h>

////////////////////////////////////////////////////////////////////////////////

namespace net {

   /**
    * NetworkEngine
    *
    * One-stop shopping for your DataCore network API needs. Herein can be found
    * functionality sufficient for hosting, joining, advertising via beacons,
    * as well as maintaining the Mesh and Node that represent the network and
    * one's role in it.
    */
   class NETCORE_EXPORT NetworkEngine
   {
   public:
      static NetworkEngine& GetRef();
      static void Destroy();

      static std::string GetHostName();
      static Address GetAddressFromHostName(const std::string& hostName); // note: returns 0 for port
      void HostNetwork();
      bool IsHostingNetwork() const { return mAcceptConnection; }
      void JoinNetwork(const net::Address& hostAddress);
      bool IsJoiningNetwork() const { return mAttemptConnection; }
      const net::Address& GetHostAddress() const { return mHostAddress; } // get the address of the host we're connecting/connected to

      // start advertising the network by sending out a beacon
      // note this will fail (return false) iff we're not hosting
      bool StartAdvertising(const std::string& name, unsigned int protocolID, unsigned int listenerPort, unsigned int serverPort, int beaconPort, Serializable* userData);
      void StopAdvertising(); // stop if we're advertising

      net::Node& GetNode() { return mNode; }
      const net::Node& GetNode() const { return mNode; }
      net::Mesh& GetMesh() { return mMesh; }
      const net::Mesh& GetMesh() const { return mMesh; }

      PacketProcessor& GetPacketProcessor() { return mPacketProcessor; }
      const PacketProcessor& GetPacketProcessor() const { return mPacketProcessor; }

      void Update(float deltaTime);

   protected:
      NetworkEngine();
      ~NetworkEngine();

   private:
      static NetworkEngine* sgSelf;

      bool mAcceptConnection;  // serve
      bool mAttemptConnection; // join as client

      net::Address mHostAddress; // address to connect to

      net::BeaconTransmitter mBeaconTransmitter; // beacon for advertising hosted game

      // for network tracking
      net::Node mNode;
      net::Mesh mMesh;

      // for packet handling
      PacketProcessor mPacketProcessor;
   };

} // namespace net

////////////////////////////////////////////////////////////////////////////////

#endif // NETWORK_ENGINE__H
