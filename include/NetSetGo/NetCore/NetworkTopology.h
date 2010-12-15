#ifndef NETWORK_TOPOLOGY_H
#define NETWORK_TOPOLOGY_H

////////////////////////////////////////////////////////////////////////////////

#include <map>

#include <NetSetGo/NetCore/NetCoreExport.h>
#include <NetSetGo/NetCore/Socket.h>
#include <NetSetGo/NetCore/ReliabilitySystem.h>
#include <NetSetGo/NetCore/NodeID.h>
#include <NetSetGo/NetCore/FlowControl.h>
#include <NetSetGo/NetCore/GuaranteedDeliverySystem.h>

namespace net {

class PacketParser;

////////////////////////////////////////////////////////////////////////////////

/**
 * NetworkTopology
 *
 * A base class for data and logic stored in common between the Node and the
 * Mesh. Note that this maintains connectivity information, and manages flow
 * control, data transfer reliability, and guaranteed delivery for each party we
 * are connected to on the network.
 *
 * Usage note: a common source of errors is the max packet size being too small
 * for custom data needed by an application. Be sure this is set high enough
 * to meet your needs.
 */
class NETCORE_EXPORT NetworkTopology
{
public:
   enum State
   {
      Disconnected,
      Connecting,
      Connected,
      ConnectFail
   };
   struct NodeState
   {
      // common
      Address mAddress;
      State mPreviousState, mCurrentState;
      // for reliability and flow control
      ReliabilitySystem mReliabilitySystem; // reliability system: manages sequence numbers and acks, tracks network stats etc.
      net::FlowControl mFlowControl;
      GuaranteedDeliverySystem mGuaranteedDeliverySystem;
      float mTransmissionDelayAccumulator;

      // used only by Mesh (server)
      float mTimeoutAccumulator;
      bool mReserved;

      NodeState();
      void Reset(bool resetState = true);
      void Update(float deltaTime);
   };

   // recommended timeout: 2 on a node, 10 on a server
   NetworkTopology(unsigned int protocolId, PacketParser* packetParser, float sendRate = 0.25f, float timeout = 10.0f, int maxPacketSize = 1024, unsigned int max_sequence = 0xFFFFFFFF);
   ~NetworkTopology();

   void SetMaxPacketSize(int maxPacketSize) { mMaxPacketSize = maxPacketSize; }
   int GetMaxPacketSize() const { return mMaxPacketSize; }
   int GetMaxGuaranteedPacketPayloadSize() const;

   void SetTimeout(float timeout) { mTimeout = timeout; }
   float GetTimeout() const { return mTimeout; }

   bool Start(int port);
   virtual void Stop();
   bool IsRunning() const { return mRunning; }
   virtual void Update(float deltaTime); // don't forget to have descendents call ancestral method!

   // note: this will fail if you try to give it a non-multicast address
   bool MulticastPacket(const net::Address& destination, const unsigned char data[], int size); // send data out via multi-cast

   // node connectivity
   State GetNodeCurrentState(NodeID nodeID) const { return mNodes[nodeID]->mCurrentState; }
   bool WasNodeConnected(NodeID nodeID) const;
   bool IsNodeConnected(NodeID nodeID) const;
   bool NodeJustConnected(NodeID nodeID) const { return !WasNodeConnected(nodeID) && IsNodeConnected(nodeID); }
   bool NodeJustDisconnected(NodeID nodeID) const { return WasNodeConnected(nodeID) && !IsNodeConnected(nodeID); }
   //
   void ConnectNode(NodeID nodeID, const Address& address);
   void DisconnectNode(NodeID nodeID, const Address& address);
   const Address& GetNodeAddress(NodeID nodeId) const;
   NodeID GetNodeIDFromAddress(const Address& address) const;
   NodeState* GetNodeByID(NodeID nodeID);
   const NodeState* GetNodeByID(NodeID nodeID) const;
   //
   void Reserve(int numNodes);
   int GetNumNodesReserved() const; // can include nodes that have dropped out

   // pure virtual methods
   virtual std::string GetIdentity() const = 0;

protected:
   bool SendPacket(const net::Address& destination, ReliabilitySystem& reliabilitySystem, const unsigned char data[], int size);
   size_t WriteHeader(unsigned char* header, unsigned int sequence, unsigned int ack, unsigned int ack_bits);
   size_t ReadHeader(const unsigned char* header, unsigned int& sequence, unsigned int& ack, unsigned int& ack_bits);
   int ReceivePacket(net::Address& origin, unsigned char data[], int size);
   void ReceivePackets();
   void ClearData();

   virtual ReliabilitySystem* ChooseReliabilitySystem(const net::Address& nodeAddress);

   const unsigned int mProtocolID;
   float mSendRate;
   float mTimeout;

   float mSendAccumulator;

   //*
   // todo: move down to private
   typedef std::map<Address, NodeID> AddrToNodeID;
   AddrToNodeID mAddrToNodeID;
   //*/

private:
   bool mRunning;

   Socket mSocket;
   static const int kHeaderSize;
   int mMaxPacketSize;
   PacketParser* mPacketParser;

   /*
   // moved down to private
   typedef std::map<Address, NodeID> AddrToNodeID;
   AddrToNodeID mAddrToNodeID;
   //*/
   std::vector<NodeState*> mNodes;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // NETWORK_TOPOLOGY_H
