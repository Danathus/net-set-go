#include <NetSetGo/NetCore/NetworkTopology.h>

#include <cassert>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#if NET_PLATFORM == NET_PLATFORM_WINDOWS
#   include <malloc.h>
#endif

#include <NetSetGo/NetCore/netassert.h>
#include <NetSetGo/NetCore/PacketParser.h>
#include <NetSetGo/NetCore/Serialization.h>

namespace net {

   // reliability header is composed of 4 ints
   const int NetworkTopology::kHeaderSize = 4*sizeof(int);

////////////////////////////////////////////////////////////////////////////////

   NetworkTopology::NodeState::NodeState()
      : mAddress(Address())
      , mPreviousState(Disconnected)
      , mCurrentState(Disconnected)
      , mGuaranteedDeliverySystem(mReliabilitySystem)
      , mTransmissionDelayAccumulator(0.0f)
      , mTimeoutAccumulator(0.0f)
      , mReserved(false)
   {
   }

   void NetworkTopology::NodeState::Reset(bool resetState)
   {
      printf("resetting NodeState\n");
      mAddress                      = Address();
      if (resetState)
      {
         mPreviousState             = Disconnected;
         mCurrentState              = Disconnected;
      }
      mReliabilitySystem.Reset();
      mFlowControl.Reset();
      mGuaranteedDeliverySystem.Reset();
      mTransmissionDelayAccumulator = 0.0f;
      mTimeoutAccumulator           = 0.0f;
      mReserved                     = false;
   }

   void NetworkTopology::NodeState::Update(float deltaTime)
   {
      mPreviousState = mCurrentState;

      // update flow control
      if (mCurrentState == Connected)
      {
         mReliabilitySystem.Update(deltaTime);
         mGuaranteedDeliverySystem.Update();
         mFlowControl.Update(deltaTime, mReliabilitySystem.GetRoundTripTime() * 1000.0f);
      }
      else
      {
         mTransmissionDelayAccumulator = 0.0f;
      }
   }

////////////////////////////////////////////////////////////////////////////////

   NetworkTopology::NetworkTopology(unsigned int protocolId, PacketParser* packetParser, float sendRate, float timeout, int maxPacketSize, unsigned int max_sequence)
      : mProtocolID(protocolId)
      , mSendRate(sendRate)
      , mTimeout(timeout)
      , mSendAccumulator(0.0f)
      //
      , mRunning(false)
      , mSocket(Socket::NonBlocking | Socket::Broadcast)
      , mMaxPacketSize(maxPacketSize)
      , mPacketParser(packetParser)
   {
      assert(mPacketParser);
   }

   NetworkTopology::~NetworkTopology()
   {
      if (IsRunning())
      {
         Stop();
      }
      assert(!IsRunning());

      ClearData();

      // Ideally, this would also be created within this class
      delete mPacketParser;
      mPacketParser = NULL;
   }

   int NetworkTopology::GetMaxGuaranteedPacketPayloadSize() const
   {
      // total packet size, subtracting out the guaranteed delivery header size, and the 4-byte protocol tag in front
      const int maxGuaranteedPacketPayloadSize = GetMaxPacketSize() - GuaranteedDeliverySystem::GetHeaderSize() - sizeof(int);
      return maxGuaranteedPacketPayloadSize;
   }

   bool NetworkTopology::Start(int port)
   {
      netassert(!IsRunning());

      if (!IsRunning())
      {
         mRunning = mSocket.Open(port);
      }

      printf("start NetworkTopology on port %d: %s\n", port, IsRunning() ? "success" : "failure");

      return IsRunning();
   }

   void NetworkTopology::Stop()
   {
      if (IsRunning())
      {
         printf("stop NetworkTopology\n");
         mSocket.Close();
         mRunning = false;
      }
   }

   void NetworkTopology::Update(float deltaTime)
   {
      // update all connected nodes
      for (size_t i = 0; i < mNodes.size(); ++i)
      {
         assert(mNodes[i]);
         mNodes[i]->Update(deltaTime);
      }
   }

   bool NetworkTopology::MulticastPacket(const net::Address& destination, const unsigned char data[], int size)
   {
      const bool success = destination.IsMulticastAddress() && mSocket.Send(destination, data, size);
      return success;
   }

   bool NetworkTopology::WasNodeConnected(NodeID nodeId) const
   {
      assert(nodeId >= 0);
      const bool wasConnected = nodeId < int(mNodes.size())
         ? mNodes[int(nodeId)]->mPreviousState == Connected
         : false;
      return wasConnected;
   }

   bool NetworkTopology::IsNodeConnected(NodeID nodeId) const
   {
      assert(nodeId >= 0);
      const bool isConnected = nodeId < int(mNodes.size())
         ? mNodes[int(nodeId)]->mCurrentState == Connected
         : false;
      return isConnected;
   }

   void NetworkTopology::ConnectNode(NodeID nodeID, const Address& address)
   {
      assert(nodeID > NODEID_INVALID);
      assert(nodeID < NodeID(mNodes.size()));

      NodeState* node = GetNodeByID(nodeID);
      assert(node);
      if (address != node->mAddress)
      {
         printf("%s: node %d @ %d.%d.%d.%d:%d connected\n", GetIdentity().c_str(), nodeID,
            address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort());
         node->mCurrentState    = Connected;
         node->mAddress         = address;
         node->mReserved        = true;
         mAddrToNodeID[address] = nodeID;
      }
   }

   void NetworkTopology::DisconnectNode(NodeID nodeID, const Address& address)
   {
      NodeState* node = GetNodeByID(nodeID);
      if (node->mCurrentState)
      {
         printf("%s: node %d disconnected\n", GetIdentity().c_str(), nodeID);
         AddrToNodeID::iterator itor = mAddrToNodeID.find(node->mAddress);
         if (itor != mAddrToNodeID.end())
         {
            mAddrToNodeID.erase(itor);
         }
         node->mCurrentState = Disconnected;
         node->mAddress = Address();

         // reset everything but node state
         node->Reset(false);
      }
   }

   const Address& NetworkTopology::GetNodeAddress(NodeID nodeId) const
   {
      assert(nodeId > NODEID_INVALID);
      assert(nodeId < NodeID(mNodes.size()));
      const Address& address = mNodes[int(nodeId)]->mAddress;
      return address;
   }

   NodeID NetworkTopology::GetNodeIDFromAddress(const Address& address) const
   {
      NodeID nodeID = NODEID_INVALID;

      AddrToNodeID::const_iterator itor = mAddrToNodeID.find(address);
      if (itor != mAddrToNodeID.end())
      {
         nodeID = itor->second;
      }

      return nodeID;
   }

   NetworkTopology::NodeState* NetworkTopology::GetNodeByID(NodeID nodeID)
   {
      NodeState* node = 0;
      if (nodeID > NODEID_INVALID && int(nodeID) < GetNumNodesReserved())
      {
         node = mNodes[int(nodeID)];
      }
      return node;
   }

   const NetworkTopology::NodeState* NetworkTopology::GetNodeByID(NodeID nodeID) const
   {
      const NodeState* node = 0;
      if (nodeID > NODEID_INVALID && int(nodeID) < GetNumNodesReserved())
      {
         node = mNodes[int(nodeID)];
      }
      return node;
   }

   const std::vector<NetworkTopology::NodeState*>& NetworkTopology::GetAllNodes() const
   {
      return mNodes;
   }

   void NetworkTopology::Reserve(int numNodes)
   {
      const size_t prevSize = mNodes.size();

      // free all the nodes we're losing
      for (size_t i = numNodes; i < prevSize; ++i)
      {
         delete mNodes[i];
      }

      // resize
      mNodes.resize(numNodes);

      // allocate all the nodes we're gaining
      for (int i = prevSize; i < numNodes; ++i)
      {
         mNodes[i] = new NodeState();
      }
   }


////////////////////////////////////////////////////////////////////////////////

   bool NetworkTopology::SendPacket(const net::Address& destination, ReliabilitySystem& reliabilitySystem, const unsigned char data[], int size)
   {
      if (!IsRunning())
      {
         return false;
      }

      // final packet size is header size + data size
      unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(kHeaderSize + size));

      size_t bytesWritten = 0;

      // first we write the header data
      bytesWritten += WriteHeader(packet,
         reliabilitySystem.GetLocalSequence(),
         reliabilitySystem.GetRemoteSequence(),
         reliabilitySystem.GenerateAckBits());

      // then we write the user data
      memcpy(&packet[bytesWritten], data, size); bytesWritten += size;

      // now we can send our finalized packet
      const bool packetSent = mSocket.Send(destination, packet, bytesWritten);
      if (packetSent)
      {
         // inform the reliability system that we sent our packet
         reliabilitySystem.PacketSent(size);
      }

      // return results
      return packetSent;
   }

   size_t NetworkTopology::WriteHeader(unsigned char* header, unsigned int sequence, unsigned int ack, unsigned int ack_bits)
   {
      size_t bytesWritten = 0;

      // first we write the protocol ID
      bytesWritten += WriteInteger(&header[bytesWritten], mProtocolID);

      // then we write the three essential elements for the reliability system
      bytesWritten += WriteInteger(&header[bytesWritten], sequence);
      bytesWritten += WriteInteger(&header[bytesWritten], ack);
      bytesWritten += WriteInteger(&header[bytesWritten], ack_bits);

      return bytesWritten;
   }

   size_t NetworkTopology::ReadHeader(const unsigned char* header, unsigned int& sequence, unsigned int& ack, unsigned int& ack_bits)
   {
      size_t bytesRead = 0;

      // first we read the protocol ID (and verify that it matches)
      unsigned int packetProtocolID;
      bytesRead += ReadInteger(&header[bytesRead], packetProtocolID);
      assert(packetProtocolID == mProtocolID);
      if (packetProtocolID != mProtocolID)
      {
         printf("incorrect protocol id: %x received, %x expected\n", packetProtocolID, mProtocolID);
         return 0;
      }

      // then we read the three essential elements for the reliability system
      bytesRead += ReadInteger(&header[bytesRead], sequence);
      bytesRead += ReadInteger(&header[bytesRead], ack);
      bytesRead += ReadInteger(&header[bytesRead], ack_bits);

      return bytesRead;
   }

   int NetworkTopology::ReceivePacket(net::Address& origin, unsigned char data[], int size)
   {
      const size_t maxReceiveSize = kHeaderSize + size;

      unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(maxReceiveSize));
      const size_t bytesReceived = mSocket.Receive(origin, packet, maxReceiveSize);

      if (bytesReceived == 0)
      {
         return 0;
      }
      if (bytesReceived <= kHeaderSize)
      {
         return 0;
      }

      size_t bytesRead = 0;

      {
         unsigned int packet_sequence = 0;
         unsigned int packet_ack      = 0;
         unsigned int packet_ack_bits = 0;
         bytesRead += ReadHeader(&packet[bytesRead], packet_sequence, packet_ack, packet_ack_bits);

         // inform the reliability system
         ReliabilitySystem* reliabilitySystem = ChooseReliabilitySystem(origin);
         if (reliabilitySystem)
         {
            reliabilitySystem->PacketReceived(packet_sequence, bytesReceived - kHeaderSize);
            reliabilitySystem->ProcessAck(packet_ack, packet_ack_bits);
         }
      }

      // copy the data out (the rest of the bytes read)
      size_t bytesWritten = 0;
      const size_t dataPayloadSize = bytesReceived - bytesRead;
      memcpy(&data[bytesWritten], &packet[bytesRead], dataPayloadSize);
      bytesRead    += dataPayloadSize;
      bytesWritten += dataPayloadSize;

      // report the amount of data written to the buffer
      return bytesWritten;
   }

   void NetworkTopology::ReceivePackets()
   {
      Address sender;
      unsigned char* data = reinterpret_cast<unsigned char*>(alloca(mMaxPacketSize));

      while (int size = ReceivePacket(sender, data, mMaxPacketSize))
      {
         mPacketParser->ParsePacket(sender, data, size);
      }
   }

   void NetworkTopology::ClearData()
   {
      for (size_t i = 0; i < mNodes.size(); ++i)
      {
         assert(mNodes[i]);
         delete mNodes[i];
      }
      mNodes.clear();
      mAddrToNodeID.clear();
   }

   ReliabilitySystem* NetworkTopology::ChooseReliabilitySystem(const net::Address& nodeAddress)
   {
      ReliabilitySystem* reliabilitySystem = 0;

      const NodeID nodeID = GetNodeIDFromAddress(nodeAddress);
      if (nodeID != NODEID_INVALID)
      {
         NodeState* node = GetNodeByID(nodeID);
         assert(node);
         reliabilitySystem = &node->mReliabilitySystem;
      }

      return reliabilitySystem;
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
