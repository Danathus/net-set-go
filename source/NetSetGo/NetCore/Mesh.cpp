#include <NetSetGo/NetCore/Mesh.h>

// these are only included to satisfy an aggravating special case
#include <NetSetGo/NetCore/NetworkEngine.h>

#include <cassert>

namespace net {

////////////////////////////////////////////////////////////////////////////////

   bool Mesh::MeshPacketParser::ParsePacket(const Address& sender, const unsigned char data[], size_t size) const
   {
      assert(size > 0);
      assert(data);

      // ignore packets that don't have the correct protocol id
      unsigned int firstIntegerInPacket = (unsigned(data[0]) << 24) | (unsigned(data[1]) << 16) |
                                          (unsigned(data[2]) << 8)  |  unsigned(data[3]);
      if (firstIntegerInPacket != mMesh.GetProtocolID())
      {
         return false;
      }

      // determine packet type
      enum PacketType { ConnectRequest, KeepAlive };
      PacketType packetType;
      if (data[4] == 0)
      {
         packetType = ConnectRequest;
      }
      else if (data[4] == 1)
      {
         packetType = KeepAlive;
      }
      else
      {
         return false;
      }

      // process packet type
      switch (packetType)
      {
      case ConnectRequest:
         {
            const NodeID nodeID = mMesh.GetNodeIDFromAddress(sender);
            // is address already connecting or connected?
            if (nodeID != NODEID_INVALID)
            {
               NodeState* node = mMesh.GetNodeByID(nodeID);
               if (node->mCurrentState == Connecting)
               {
                  // reset timeout accumulator, but only while connecting
                  node->mTimeoutAccumulator = 0.0f;
               }
            }
            else
            {
               // no entry for address, start connect process...
               const NodeID freeSlot = mMesh.FindFirstUnreservedNode();
               NodeState* node = mMesh.GetNodeByID(freeSlot);
               if (node)
               {
                  printf("mesh accepts %d.%d.%d.%d:%d as node %d\n",
                     sender.GetA(), sender.GetB(), sender.GetC(), sender.GetD(), sender.GetPort(), freeSlot);

                  assert(node->mCurrentState == Disconnected);
                  mMesh.Reserve(freeSlot, sender);
               }
            }
         }
         break;
      case KeepAlive:
         {
            const NodeID nodeID = mMesh.GetNodeIDFromAddress(sender);
            if (nodeID != NODEID_INVALID)
            {
               NodeState* node = mMesh.GetNodeByID(nodeID);
               // progress from "connection accept" to "connected"
               if (node->mCurrentState == NetworkTopology::Connecting)
               {
                  node->mCurrentState = NetworkTopology::Connected;
                  node->mReserved = false;
                  printf("mesh completes connection of node %d\n", nodeID);
               }
               // reset timeout accumulator for node
               node->mTimeoutAccumulator = 0.0f;
            }
         }
         break;
      }

      return true;
   }

////////////////////////////////////////////////////////////////////////////////

   Mesh::Mesh(unsigned int protocolId, int maxNodes, float sendRate, float timeout)
      : NetworkTopology(protocolId, new MeshPacketParser(*this), sendRate, timeout, 256) // using 256 for max packet size?
      , kMaxNodes(maxNodes)
   {
      assert(kMaxNodes >= 1);
      assert(kMaxNodes <= 255);
      NetworkTopology::Reserve(kMaxNodes);
   }

   void Mesh::Stop()
   {
      NetworkTopology::Stop();
      ClearData();
   }

   void Mesh::Update(float deltaTime)
   {
      if (IsRunning())
      {
         // respect your elders
         NetworkTopology::Update(deltaTime);

         // update self
         ReceivePackets(); // this is where we can accept new connections
         CheckForTimeouts(deltaTime); // this is where we close out old connections
         SendPackets(deltaTime); // this is where we inform nodes of any changes
      }
   }

   NodeID Mesh::FindFirstUnreservedNode() const
   {
      NodeID freeSlot = NODEID_INVALID;
      for (int i = 0; i < GetNumNodesReserved(); ++i)
      {
         if (GetNodeCurrentState(NodeID(i)) == NetworkTopology::Disconnected)
         {
            freeSlot = NodeID(i);
            break;
         }
      }
      return freeSlot;
   }

   void Mesh::Reserve(NodeID nodeID, const Address& address)
   {
      assert(nodeID > NODEID_INVALID);
      assert(nodeID < NodeID(GetNumNodesReserved()));
      printf("mesh reserves node id %d for %d.%d.%d.%d:%d\n",
         nodeID, address.GetA(), address.GetB(), address.GetC(), address.GetD(), address.GetPort());

      NodeState* node = GetNodeByID(nodeID);
      node->mCurrentState = NetworkTopology::Connecting;
      node->mAddress      = address;
      node->mReserved     = true;
      mAddrToNodeID.insert(std::make_pair(address, nodeID));
   }

   std::string Mesh::GetIdentity() const
   {
      return "mesh";
   }

////////////////////////////////////////////////////////////////////////////////

   void Mesh::SendPackets(float deltaTime)
   {
      mSendAccumulator += deltaTime;
      while (mSendAccumulator > mSendRate)
      {
         for (int i = 0; i < GetNumNodesReserved(); ++i)
         {
            switch (GetNodeCurrentState(NodeID(i)))
            {
            case NetworkTopology::Connecting:
               {
                  // node is negotiating connect: send "connection accepted" packets
                  unsigned char packet[7];
                  packet[0] = (unsigned char)((mProtocolID >> 24) & 0xFF);
                  packet[1] = (unsigned char)((mProtocolID >> 16) & 0xFF);
                  packet[2] = (unsigned char)((mProtocolID >> 8)  & 0xFF);
                  packet[3] = (unsigned char)((mProtocolID) & 0xFF);
                  packet[4] = 0;
                  packet[5] = (unsigned char)i;
                  packet[6] = (unsigned char)GetNumNodesReserved();
                  const bool success = SendPacket(GetNodeAddress(NodeID(i)), GetNodeByID(NodeID(i))->mReliabilitySystem, packet, sizeof(packet));
                  //printf("Mesh sending ConnectionAccepted packet of size %d; success: %s\n", sizeof(packet), success ? "yes" : "no");
               }
               break;
            case NetworkTopology::Connected:
               {
                  // node is connected: send "update" packets
                  const size_t packetSize = 5+6*GetNumNodesReserved();
                  //               unsigned char packet[packetSize];
                  unsigned char* packet = reinterpret_cast<unsigned char*>(alloca(packetSize));
                  packet[0] = (unsigned char)((mProtocolID >> 24) & 0xFF);
                  packet[1] = (unsigned char)((mProtocolID >> 16) & 0xFF);
                  packet[2] = (unsigned char)((mProtocolID >> 8)  & 0xFF);
                  packet[3] = (unsigned char)((mProtocolID)       & 0xFF);
                  packet[4] = 1;
                  unsigned char* ptr = &packet[5];
                  for (int j = 0; j < GetNumNodesReserved(); ++j)
                  {
                     const net::Address& address = GetNodeAddress(NodeID(j));
                     ptr[0] = (unsigned char)address.GetA();
                     ptr[1] = (unsigned char)address.GetB();
                     ptr[2] = (unsigned char)address.GetC();
                     ptr[3] = (unsigned char)address.GetD();
                     ptr[4] = (unsigned char)((address.GetPort() >> 8) & 0xFF);
                     ptr[5] = (unsigned char)((address.GetPort()) & 0xFF);
                     ptr += 6;
                  }
                  const net::Address& nodeAddress = GetNodeAddress(NodeID(i));
                  const bool success = SendPacket(nodeAddress, GetNodeByID(NodeID(i))->mReliabilitySystem, packet, packetSize);
                  //printf("Mesh sending Update packet of size %d to node %d at address %d.%d.%d.%d:%d; success: %s\n", packetSize, NodeID(i),
                  //   nodeAddress.GetA(), nodeAddress.GetB(), nodeAddress.GetC(), nodeAddress.GetD(), nodeAddress.GetPort(),
                  //   success ? "yes" : "no");
               }
               break;
            }
         }
         mSendAccumulator -= mSendRate;
      }
   }

   void Mesh::CheckForTimeouts(float deltaTime)
   {
      for (int i = 0; i < GetNumNodesReserved(); ++i)
      {
         NodeState* node = GetNodeByID(NodeID(i));
         if (GetNodeCurrentState(NodeID(i)) != NetworkTopology::Disconnected)
         {
            node->mTimeoutAccumulator += deltaTime;
            if (node->mTimeoutAccumulator > mTimeout && !node->mReserved)
            {
               printf("mesh timed out node %d\n", i);
               AddrToNodeID::iterator addr_itor = mAddrToNodeID.find(GetNodeAddress(i));
               assert(addr_itor != mAddrToNodeID.end());
               mAddrToNodeID.erase(addr_itor);
               node->Reset();

               // cheat: at this point we should disconnect the node's node, too
               {
                  net::Node& node = net::NetworkEngine::GetRef().GetNode();
                  if (node.IsRunning() && node.IsNodeConnected(i))
                  {
                     node.DisconnectNode(i, node.GetNodeAddress(i));
                  }
               }
            }
         }
      }
   }

   void Mesh::ClearData()
   {
      NetworkTopology::ClearData();
      NetworkTopology::Reserve(kMaxNodes);
      mSendAccumulator = 0.0f;
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
