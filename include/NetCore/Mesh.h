#ifndef MESH_H
#define MESH_H

#include <NetCore/NetCoreExport.h>

#include <NetCore/Address.h>
#include <NetCore/PacketParser.h>

#include <NetCore/NetworkTopology.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

// node mesh
//  + manages node connect and disconnect
//  + updates each node with set of currently connected nodes

/**
 * Mesh
 *
 * This represents the network at large, and is only instantiated on the host or
 * server. Among other tasks, the Mesh regularly pings connected nodes to
 * confirm and maintain a stable network connection.
 */
class NETCORE_EXPORT Mesh : public NetworkTopology
{
public:
   Mesh(unsigned int protocolId, int maxNodes = 255, float sendRate = 0.25f, float timeout = 10.0f);

   unsigned int GetProtocolID() const { return mProtocolID; }
   void Stop();
   void Update(float deltaTime);

   NodeID FindFirstUnreservedNode() const; // NODEID_INVALID means none found
   // todo: merge this into NetworkTopology::ConnectNode()
   void Reserve(NodeID nodeID, const Address& address);

   // implementations of pure virtual methods
   std::string GetIdentity() const;

protected:
   void SendPackets(float deltaTime);
   void CheckForTimeouts(float deltaTime);
   void ClearData();

private:
   class MeshPacketParser : public PacketParser
   {
   public:
      MeshPacketParser(Mesh& mesh) : mMesh(mesh) {}

      // implementations of pure virtual methods
      bool ParsePacket(const Address& sender, const unsigned char data[], size_t size) const;

   protected:
   private:
      Mesh& mMesh;
   };

   const int kMaxNodes;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // MESH_H
