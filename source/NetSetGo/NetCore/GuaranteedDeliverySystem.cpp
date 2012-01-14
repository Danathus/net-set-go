#include <NetSetGo/NetCore/GuaranteedDeliverySystem.h>

#include <NetSetGo/NetCore/netassert.h>
#include <NetSetGo/NetCore/NetworkEngine.h>

//#include <NetCore/Node.h> // for debug printing only, prolly should be removed...

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#define VERBOSE 0

////////////////////////////////////////////////////////////////////////////////

size_t GuaranteedDeliverySystem::GuaranteedPacket::Serialize(char* packet) const
{
   size_t bytesWritten = 0;

   memcpy(&packet[bytesWritten], &mGuaranteedSequence, sizeof(mGuaranteedSequence)); bytesWritten += sizeof(mGuaranteedSequence);
   memcpy(&packet[bytesWritten], &mLength,             sizeof(mLength));             bytesWritten += sizeof(mLength);
   memcpy(&packet[bytesWritten], mData,                mLength);                     bytesWritten += mLength;

   assert(bytesWritten == GetSize());
   //printf("GuaranteedPacket::Serialize(): seq#%d, length %d\n", mGuaranteedSequence, mLength);

   return bytesWritten;
}

size_t GuaranteedDeliverySystem::GuaranteedPacket::Deserialize(const char* packet, size_t packetLength)
{
   size_t bytesRead = 0;

   assert(packetLength > GetHeaderSize());

   memcpy(&mGuaranteedSequence, &packet[bytesRead], sizeof(mGuaranteedSequence)); bytesRead += sizeof(mGuaranteedSequence);
   memcpy(&mLength,             &packet[bytesRead], sizeof(mLength));             bytesRead += sizeof(mLength);
   assert(bytesRead == GetHeaderSize());

   // sanity checks
   assert(mLength >= 0);
   /*
   if (mLength > net::NetworkEngine::GetRef().GetNode().GetMaxGuaranteedPacketPayloadSize())
   {
      // debug print
      printf("GuaranteedDeliverySystem::GuaranteedPacket::Deserialize() received full packet: ");
      net::Node::PrintPacket((unsigned char *)packet, packetLength);
      printf("\n\tdetected sequence #%u, payload length %u\n", mGuaranteedSequence, mLength);
   }
   //*/
   assert(mLength <= size_t(net::NetworkEngine::GetRef().GetNode().GetMaxGuaranteedPacketPayloadSize()));
   assert(mLength <= packetLength - GetHeaderSize());

   //printf("GuaranteedPacket::Deserialize(): seq#%u, length %u\n", mGuaranteedSequence, mLength);
   {
      char* payload = (char *)malloc(mLength);
      if (!payload)
      {
         printf("%s:%d\timpending assertion failure. malloc failed w/ length %d\n", __FUNCTION__, __LINE__, mLength);
      }
      assert(payload);
      memcpy(payload,           &packet[bytesRead], mLength);                     bytesRead += mLength;
      mData = payload;
   }

   assert(bytesRead == GetSize());

   return bytesRead;
}

size_t GuaranteedDeliverySystem::GuaranteedPacket::GetHeaderSize()
{
   size_t bytes = 0;

   GuaranteedPacket packet;
   bytes += sizeof(packet.mGuaranteedSequence);
   bytes += sizeof(packet.mLength);

   return bytes;
}

size_t GuaranteedDeliverySystem::GuaranteedPacket::GetSize() const
{
   size_t bytes = 0;

   bytes += GetHeaderSize();
   bytes += mLength;

   return bytes;
}

////////////////////////////////////////////////////////////////////////////////

GuaranteedDeliverySystem::GuaranteedDeliverySystem(const net::ReliabilitySystem& reliabilitySystem)
   : mReliabilitySystem(reliabilitySystem)
   , mLocalGuaranteedSequenceNumber(0)
   , mRemoteGuaranteedSequenceNumber(0)
   , mGuaranteedNumberMismatchReported(false)
{
   //
}

void GuaranteedDeliverySystem::Reset()
{
   mLocalGuaranteedSequenceNumber  = 0;
   mRemoteGuaranteedSequenceNumber = 0;

   mPendingSendQueue.clear();
   mPendingAckQueue.clear();
   mPendingRecvQueue.clear();
}

void GuaranteedDeliverySystem::QueueOutgoingPacket(const char* packet, size_t length)
{
   GuaranteedPacket guaranteedPacket;

   guaranteedPacket.mGuaranteedSequence = mLocalGuaranteedSequenceNumber;
   // make a local copy of the packet
   {
      char* payload = (char *)malloc(length);
      memcpy(payload, packet, length);
      guaranteedPacket.mData = payload;
   }
   guaranteedPacket.mLength = length;

   //printf("GuaranteedDeliverySystem queueing outgoing packet, guaranteed sequence number %d, packet size %d\n", mLocalGuaranteedSequenceNumber, length);
   mPendingSendQueue.push_back(guaranteedPacket);

   // increment sequence number for next time
   ++mLocalGuaranteedSequenceNumber;
}

bool GuaranteedDeliverySystem::DequeueReceivedPacket(int nodeID, char*& packet, size_t& length)
{
   bool success = false;

   if (!mPendingRecvQueue.empty())
   {
      GuaranteedPacket& front = mPendingRecvQueue.front();
      if (front.mGuaranteedSequence != mRemoteGuaranteedSequenceNumber)
      {
         if (!mGuaranteedNumberMismatchReported)
         {
#if VERBOSE
            printf("%s:%d\tWARNING: for node %d, front.mGuaranteedSequence (%d) != mRemoteGuaranteedSequenceNumber (%d)\n",
               __FUNCTION__, __LINE__, nodeID, front.mGuaranteedSequence, mRemoteGuaranteedSequenceNumber);
#endif
         }
         mGuaranteedNumberMismatchReported = true;
      }
      netassert(front.mGuaranteedSequence == mRemoteGuaranteedSequenceNumber);
      if (front.mGuaranteedSequence == mRemoteGuaranteedSequenceNumber)
      {
         if (mGuaranteedNumberMismatchReported)
         {
            mGuaranteedNumberMismatchReported = false;
#if VERBOSE
            printf("%s:%d\t...for node %d, Local and remote guaranteed sequence numbers match again!\n",
               __FUNCTION__, __LINE__, nodeID);
#endif
         }

         // if the arguments passed in indicate we need to allocate, do so
         if (!packet || length == 0)
         {
            packet = (char *)malloc(front.mLength);
            length = front.mLength;
         }
         // make sure there's enough space in which to write our data
         netassert(length >= front.mLength);
         if (length >= front.mLength)
         {
            // write the data out
            memcpy(packet, front.mData, front.mLength);
            length = front.mLength;

            // remove from queue and clean up
            free((void *)front.mData);
            mPendingRecvQueue.pop_front();

            // update so we're looking for the next sequence number next time
            ++mRemoteGuaranteedSequenceNumber;

            // report success
            success = true;
         }
      }
      /*
      else
      {
         printf("found sequence #%u, expected #%u\n", front.mGuaranteedSequence, mRemoteGuaranteedSequenceNumber);
      }
      //*/
   }

   if (!success)
   {
      length = 0;
   }

   return success;
}

size_t GuaranteedDeliverySystem::SerializePacket(char* packet, size_t maxLength)
{
   size_t bytesWritten = 0;

   // try to send next outgoing packet...
   if (mPendingSendQueue.size())
   {
      GuaranteedPacket& guaranteedPacket = mPendingSendQueue.front();

      // attempt to write
      if (guaranteedPacket.GetSize() <= maxLength)
      {
         assert(packet);
         bytesWritten += guaranteedPacket.Serialize(packet);
         assert(bytesWritten > 0);

         // move it to pending ack queue
         IssuedGuaranteedPacket sentPacket;
         //
         sentPacket.mReliabilitySequence = mReliabilitySystem.GetLocalSequence();
         sentPacket.guaranteedPacket = guaranteedPacket;
         //
         mPendingAckQueue.push_back(sentPacket);
         mPendingSendQueue.pop_front();
      }
      else
      {
#if VERBOSE
         printf("%s:%d\tguaranteed packet of size %d is too big to fit within max length %d\n", __FUNCTION__, __LINE__, guaranteedPacket.GetSize(), maxLength);
#endif
      }
   }

   return bytesWritten;
}

size_t GuaranteedDeliverySystem::DeserializePacket(const char* packet, size_t length)
{
   GuaranteedPacket guaranteedPacket;

   const size_t bytesRead = guaranteedPacket.Deserialize(packet, length);

   /* debug print
   printf("GuaranteedDeliverySystem::DeserializePacket(): ");
   net::Node::PrintPacket((unsigned char *)packet, bytesRead);
   printf("\n");
   //*/

   // only insert if this packet isn't already in the queue and isn't one we've already processed
   if (guaranteedPacket.mGuaranteedSequence >= mRemoteGuaranteedSequenceNumber && !FindInPacketList(mPendingRecvQueue, guaranteedPacket))
   {
      InsertSorted(mPendingRecvQueue, guaranteedPacket);
   }

   return bytesRead;
}

void GuaranteedDeliverySystem::Update()
{
   // handle recently acked packets
   {
      const net::PacketQueue& ackedPackets = mReliabilitySystem.GetRecentlyAckedPackets();
      for (net::PacketQueue::const_iterator itor = ackedPackets.begin(); itor != ackedPackets.end(); ++itor)
      {
         // find this packet
         std::list<IssuedGuaranteedPacket>::iterator pendingAckItor = FindPendingAckPacket(itor->mSequence);

         if (pendingAckItor != mPendingAckQueue.end())
         {
            // remove from pending ack queue
            mPendingAckQueue.erase(pendingAckItor); // todo: should really properly free memory...
         }
      }
   }

   // handle recently lost packets
   {
      const net::PacketQueue& lostPackets = mReliabilitySystem.GetRecentlyLostPackets();
      for (net::PacketQueue::const_reverse_iterator itor = lostPackets.rbegin(); itor != lostPackets.rend(); ++itor)
      {
         // find this packet
         std::list<IssuedGuaranteedPacket>::iterator pendingAckItor = FindPendingAckPacket(itor->mSequence);

         if (pendingAckItor != mPendingAckQueue.end())
         {
            // we want to resend these packets
            {
               GuaranteedPacket guaranteedPacket;

               guaranteedPacket.mGuaranteedSequence = pendingAckItor->guaranteedPacket.mGuaranteedSequence;
               guaranteedPacket.mData               = pendingAckItor->guaranteedPacket.mData;
               guaranteedPacket.mLength             = pendingAckItor->guaranteedPacket.mLength;

#if VERBOSE
               printf("%s:%d\t>>>\tdetected lost guaranteed-delivery packet: guaranteed seq# %d reliability seq# %d data length %d resending...",
                  __FUNCTION__, __LINE__, guaranteedPacket.mGuaranteedSequence, itor->mSequence, guaranteedPacket.mLength);
#endif
               mPendingSendQueue.push_front(guaranteedPacket);
            }

            // and remove from pending ack queue
            mPendingAckQueue.erase(pendingAckItor); // todo: should really properly free memory...
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

std::list<GuaranteedDeliverySystem::IssuedGuaranteedPacket>::iterator GuaranteedDeliverySystem::FindPendingAckPacket(unsigned int reliabilitySequence)
{
   // find this packet
   for (std::list<IssuedGuaranteedPacket>::iterator pendingAckItor = mPendingAckQueue.begin(); pendingAckItor != mPendingAckQueue.end(); ++pendingAckItor)
   {
      if (pendingAckItor->mReliabilitySequence == reliabilitySequence)
      {
         return pendingAckItor;
      }
   }

   return mPendingAckQueue.end();
}

bool GuaranteedDeliverySystem::FindInPacketList(const std::list<GuaranteedPacket>& packetList, const GuaranteedPacket& packet) const
{
   for (std::list<GuaranteedPacket>::const_iterator itor = packetList.begin(); itor != packetList.end(); ++itor)
   {
      if (itor->mGuaranteedSequence == packet.mGuaranteedSequence)
      {
         return true;
      }
   }
   return false;
}

void GuaranteedDeliverySystem::InsertSorted(std::list<GuaranteedPacket>& packetList, const GuaranteedPacket& packet)
{
   const unsigned int max_sequence = 0xFFFFFFFF;
   if (packetList.empty())
   {
      packetList.push_back(packet);
   }
   else
   {
      if (!net::ReliabilitySystem::sequence_more_recent(packet.mGuaranteedSequence, packetList.front().mGuaranteedSequence, max_sequence))
      {
         packetList.push_front(packet);
      }
      else if (net::ReliabilitySystem::sequence_more_recent(packet.mGuaranteedSequence, packetList.back().mGuaranteedSequence, max_sequence))
      {
         packetList.push_back(packet);
      }
      else
      {
         for (std::list<GuaranteedPacket>::iterator itor = packetList.begin(); itor != packetList.end(); ++itor)
         {
            assert(itor->mGuaranteedSequence != packet.mGuaranteedSequence);
            if (net::ReliabilitySystem::sequence_more_recent(itor->mGuaranteedSequence, packet.mGuaranteedSequence, max_sequence))
            {
               packetList.insert(itor, packet);
               break;
            }
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
