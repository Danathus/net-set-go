#ifndef NODE_H
#define NODE_H

#include <queue>

#include <NetCore/NetCoreExport.h>

#include <NetCore/Address.h>
#include <NetCore/PacketParser.h>

#include <NetCore/NetworkTopology.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

// node

/**
 * Node
 *
 * Each client joining a network is a Node, and should build one of these
 * classes to reflect itself. Note that this is already managed for us in the
 * NetworkEngine, and in general there is no reason to directly instantiate one
 * of these.
 */
class NETCORE_EXPORT Node : public NetworkTopology
{
public:
   static void PrintPacket(const unsigned char data[], int size);

   struct BufferedPacket
   {
      NodeID mNodeId;
      std::vector<unsigned char> mData;
   };

   Node(unsigned int protocolId, float sendRate = 0.25f, float timeout = 2.0f, int maxPacketSize = 1024);

   void Stop();
   void Connect(const Address& address);

   const State GetPreviousState() const { return mPreviousState; }
   const State GetCurrentState() const { return mCurrentState; }
   void SetCurrentState(State state) { mCurrentState = state; }
   //
   bool IsConnecting() const   { return GetCurrentState() == Connecting; }
   bool ConnectFailed() const  { return GetCurrentState() == ConnectFail; }
   bool WasConnected() const   { return GetPreviousState() == Connected; }
   bool IsConnected() const    { return GetCurrentState() == Connected; }
   bool IsDisconnected() const { return GetCurrentState() == Disconnected; }
   //
   bool JustConnected() const { return !WasConnected() && IsConnected(); }
   bool JustDisconnected() const { return WasConnected() && !IsConnected(); }

   NodeID GetLocalNodeID() const { return mLocalNodeID; }
   void SetLocalNodeID(NodeID localNodeID) { mLocalNodeID = localNodeID; }

   void Update(float deltaTime);

   bool SendPacket(NodeID nodeID, const unsigned char data[], int size); // use this to send outgoing packets
   int ReceivePacket(NodeID& nodeID, unsigned char data[], int size); // remove stowed packet from buffer, write to data
   void BufferPacket(NodeID nodeID, const unsigned char data[], int size); // copy incoming packet, stow into a buffer (used by PacketProcessor)

   unsigned int GetProtocolID() const { return mProtocolID; }
   const Address& GetMeshAddress() const { return mMeshAddress; }

   void ClearTimeoutAccumulator() { mTimeoutAccumulator = 0; }

   // implementations of pure virtual methods
   std::string GetIdentity() const;

protected:
   void SendPackets(float deltaTime);
   void CheckForTimeout(float deltaTime);
   void ClearData();

   // overriding virtual methods
   ReliabilitySystem* ChooseReliabilitySystem(const net::Address& nodeAddress);

private:
   class NodePacketParser : public PacketParser
   {
   public:
      NodePacketParser(Node& node) : mNode(node) {}

      // implementations of pure virtual methods
      bool ParsePacket(const Address& sender, const unsigned char data[], size_t size) const;

   protected:
   private:
      Node& mNode;
   };

   typedef std::queue<BufferedPacket*> PacketBuffer;
   PacketBuffer mReceivedPackets;

   float mTimeoutAccumulator;
   State mPreviousState, mCurrentState;
   Address mMeshAddress;
   NodeID mLocalNodeID;
   ReliabilitySystem mMeshReliabilitySystem; // reliability system: manages sequence numbers and acks, tracks network stats etc.
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // NODE_H
