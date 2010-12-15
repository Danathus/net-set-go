#include <NetCore/Node.h>

#include <cassert>
#include <sstream>

#include <NetCore/netassert.h>

#define PRINT_OUTGOING_PACKETS 0 // commit as 0
#define PRINT_INCOMING_PACKETS 0 // commit as 0

namespace net {

//

   void Node::PrintPacket(const unsigned char data[], int size)
   {
      printf("packet<length:%d>(", size);
      for (int i = 0; i < size; ++i)
      {
         printf("[%3u]", data[i]);
      }
      printf("\"");
      for (int i = 0; i < size; ++i)
      {
         printf("%c", isgraph(data[i]) ? data[i] : '?');
      }
      printf("\"");
      printf(")");
   }

////////////////////////////////////////////////////////////////////////////////

   bool Node::NodePacketParser::ParsePacket(const Address& sender, const unsigned char data[], size_t size) const
   {
      assert(sender != Address());
      assert(size > 0);
      assert(data);
      // is packet from the mesh?
      if (sender == mNode.GetMeshAddress())
      {
         // *** packet sent from the mesh ***
         // ignore packets that don't have the correct protocol id
         unsigned int firstIntegerInPacket = (unsigned(data[0]) << 24) | (unsigned(data[1]) << 16) |
                                             (unsigned(data[2]) << 8)  |  unsigned(data[3]);
         if (firstIntegerInPacket != mNode.GetProtocolID())
         {
            return false;
         }
         // determine packet type
         enum PacketType { ConnectionAccepted, Update };
         PacketType packetType;
         if (data[4] == 0)
         {
            packetType = ConnectionAccepted;
         }
         else if (data[4] == 1)
         {
            packetType = Update;
         }
         else
         {
            return false;
         }
         // handle packet type
         switch (packetType)
         {
         case ConnectionAccepted:
            if (size != 7)
            {
               return false;
            }
            if (mNode.GetCurrentState() == Connecting)
            {
               mNode.SetLocalNodeID(NodeID(data[5]));
               mNode.Reserve(data[6]);
               printf("node connects as node %d of %d\n", mNode.GetLocalNodeID(), mNode.GetNumNodesReserved());
               mNode.SetCurrentState(Connected);
            }
            mNode.ClearTimeoutAccumulator();
            break;
         case Update:
            if (size != int(5 + mNode.GetNumNodesReserved() * 6))
            {
               return false;
            }
            if (mNode.GetCurrentState() == Connected)
            {
               // process update packet
               const unsigned char* ptr = &data[5];
               for (net::NodeID nodeID = net::NodeID(0); int(nodeID) < mNode.GetNumNodesReserved(); ++nodeID)
               {
                  unsigned char a = ptr[0];
                  unsigned char b = ptr[1];
                  unsigned char c = ptr[2];
                  unsigned char d = ptr[3];
                  unsigned short port = (unsigned short)ptr[4] << 8 | (unsigned short)ptr[5];
                  Address address(a, b, c, d, port);
                  NodeState* node = mNode.GetNodeByID(nodeID);
                  if (address.GetAddress() != 0)
                  {
                     // node is connected
                     mNode.ConnectNode(nodeID, address);
                  }
                  else
                  {
                     // node is not connected
                     mNode.DisconnectNode(nodeID, address);
                  }
                  ptr += 6;
               }
            }
            mNode.ClearTimeoutAccumulator();
            break;
         }
      }
      else
      {
         NodeID nodeID = mNode.GetNodeIDFromAddress(sender);
         if (nodeID != NODEID_INVALID)
         {
            NodeState* node = mNode.GetNodeByID(nodeID);
            // *** packet sent from another node ***
            assert(node);
            assert(int(nodeID) < mNode.GetNumNodesReserved());

            mNode.BufferPacket(nodeID, data, size);
         }
      }

      return true;
   }

////////////////////////////////////////////////////////////////////////////////

   Node::Node(unsigned int protocolId, float sendRate, float timeout, int maxPacketSize)
      : NetworkTopology(protocolId, new NodePacketParser(*this), sendRate, timeout, maxPacketSize)
      , mCurrentState(Disconnected)
      , mPreviousState(Disconnected)
      , mMeshReliabilitySystem(0xFFFFFFFF) // max sequence
   {
      ClearData();
   }

   void Node::Stop()
   {
      NetworkTopology::Stop();
      ClearData();
   }

   void Node::Connect(const Address& address)
   {
      printf("node connect to %d.%d.%d.%d:%d\n",
         address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort());
      ClearData();
      mCurrentState = Connecting;
      mMeshAddress = address;
   }

   void Node::Update(float deltaTime)
   {
      mPreviousState = mCurrentState;

      if (IsRunning())
      {
         // handle disconnections
         //  we do this right before updating state
         //  this will give other things a chance to respond to JustDisconnected
         for (int i = 0; i < GetNumNodesReserved(); ++i)
         {
            if (NodeJustDisconnected(NodeID(i)))
            {
               NodeState* node = GetNodeByID(NodeID(i));
               assert(node);
               node->Reset();
            }
         }

         // respect your elders
         NetworkTopology::Update(deltaTime);

         // update self
         mMeshReliabilitySystem.Update(deltaTime);
         ReceivePackets();
         SendPackets(deltaTime);
         CheckForTimeout(deltaTime);
      }
      else
      {
         mCurrentState = Disconnected;
      }
   }

   bool Node::SendPacket(NodeID nodeID, const unsigned char data[], int size)
   {
      netassert(IsRunning());
      if (!IsRunning()) { return false; }
      const int numNodesReserved = GetNumNodesReserved();
      if (numNodesReserved == 0)
      {
         return false;   // not connected yet
      }
      assert(nodeID >= 0);
      netassert(nodeID < numNodesReserved);
      if (nodeID < 0 || nodeID >= net::NodeID(numNodesReserved))
      {
         return false;
      }
      if (!IsNodeConnected(nodeID))
      {
         return false;
      }
      assert(size <= GetMaxPacketSize());
      if (size > GetMaxPacketSize())
      {
         return false;
      }
      const bool sent = NetworkTopology::SendPacket(GetNodeAddress(nodeID), GetNodeByID(nodeID)->mReliabilitySystem, data, size);

#if PRINT_OUTGOING_PACKETS
      {
         printf("sent outgoing packet:\n\t");
         PrintPacket(data, size);
         printf("\n");
      }
#endif

      return sent;
   }

   int Node::ReceivePacket(NodeID& nodeID, unsigned char data[], int size)
   {
      int sizeRead = 0;

      assert(IsRunning());
      if (IsRunning())
      {
         if (!mReceivedPackets.empty())
         {
            BufferedPacket* packet = mReceivedPackets.front();
            assert(packet);
            if (int(packet->mData.size()) <= size)
            {
               nodeID = packet->mNodeId;
               size = packet->mData.size();
               memcpy(data, &packet->mData[0], size);
               delete packet;
               mReceivedPackets.pop();
               sizeRead = size;
            }
            else
            {
               delete packet;
               mReceivedPackets.pop();
            }
         }
      }

#if PRINT_INCOMING_PACKETS
      if (sizeRead)
      {
         printf("received incoming packet:\n\t");
         PrintPacket(data, size);
         printf("\n");
      }
#endif

      return sizeRead;
   }

   void Node::BufferPacket(NodeID nodeID, const unsigned char data[], int size)
   {
      BufferedPacket* packet = new BufferedPacket;
      packet->mNodeId = nodeID;
      packet->mData.resize(size);
      memcpy(&packet->mData[0], data, size);
      mReceivedPackets.push(packet);
   }

   std::string Node::GetIdentity() const
   {
      std::stringstream strstrm;
      strstrm << "node ";
      strstrm << GetLocalNodeID();
      return strstrm.str();
   }

////////////////////////////////////////////////////////////////////////////////

   void Node::SendPackets(float deltaTime)
   {
      mSendAccumulator += deltaTime;
      while (mSendAccumulator > mSendRate)
      {
         if (GetCurrentState() == Connecting)
         {
            // node is connecting: send "connect request" packets
            unsigned char packet[5];
            packet[0] = (unsigned char)((mProtocolID >> 24) & 0xFF);
            packet[1] = (unsigned char)((mProtocolID >> 16) & 0xFF);
            packet[2] = (unsigned char)((mProtocolID >> 8)  & 0xFF);
            packet[3] = (unsigned char)((mProtocolID) & 0xFF);
            packet[4] = 0;
            NetworkTopology::SendPacket(mMeshAddress, mMeshReliabilitySystem, packet, sizeof(packet));
         }
         else if (GetCurrentState() == Connected)
         {
            // node is connected: send "keep alive" packets
            unsigned char packet[5];
            packet[0] = (unsigned char)((mProtocolID >> 24) & 0xFF);
            packet[1] = (unsigned char)((mProtocolID >> 16) & 0xFF);
            packet[2] = (unsigned char)((mProtocolID >> 8)  & 0xFF);
            packet[3] = (unsigned char)((mProtocolID) & 0xFF);
            packet[4] = 1;
            NetworkTopology::SendPacket(mMeshAddress, mMeshReliabilitySystem, packet, sizeof(packet));
         }
         mSendAccumulator -= mSendRate;
      }
   }

   void Node::CheckForTimeout(float deltaTime)
   {
      if (GetCurrentState() == Connecting || GetCurrentState() == Connected)
      {
         mTimeoutAccumulator += deltaTime;
         if (mTimeoutAccumulator > mTimeout)
         {
            if (GetCurrentState() == Connecting)
            {
               printf("node connect failed\n");
               mCurrentState = ConnectFail;
            }
            else
            {
               printf("node connection timed out\n");
               mCurrentState = Disconnected;
            }
            ClearData();
         }
      }
   }

   void Node::ClearData()
   {
      NetworkTopology::ClearData();
      while (!mReceivedPackets.empty())
      {
         BufferedPacket* packet = mReceivedPackets.front();
         delete packet;
         mReceivedPackets.pop();
      }
      mSendAccumulator = 0.0f;
      mTimeoutAccumulator = 0.0f;
      mLocalNodeID = NODEID_INVALID;
      mMeshAddress = Address();
      mMeshReliabilitySystem.Reset();
   }

   ReliabilitySystem* Node::ChooseReliabilitySystem(const net::Address& nodeAddress)
   {
      ReliabilitySystem* reliabilitySystem = 0;

      // try to see if address matches the Mesh
      if (nodeAddress == mMeshAddress)
      {
         // it does -- we want the reliability system used for communicating with the mesh
         reliabilitySystem = &mMeshReliabilitySystem;
      }
      else
      {
         // it must be one of the nodes; resort to default behavior
         reliabilitySystem = NetworkTopology::ChooseReliabilitySystem(nodeAddress);
      }

      return reliabilitySystem;
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
