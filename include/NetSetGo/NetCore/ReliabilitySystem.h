#ifndef RELIABILITYSYSTEM_H
#define RELIABILITYSYSTEM_H

////////////////////////////////////////////////////////////////////////////////

#include <vector>

#include <NetSetGo/NetCore/NetCoreExport.h>

#include <NetSetGo/NetCore/PacketQueue.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

// reliability system to support reliable connection
//  + manages sent, received, pending ack and acked packet queues
//  + separated out from reliable connection because it is quite complex and I
//    want to unit test it!

/**
 * ReliabilitySystem
 *
 * This keeps track of whether the last 32 packets, indexed by sequence number,
 * were received. This information is forwarded in a header with each new packet
 * sent to the connected party, so we are continually sharing information about
 * which packets were received. As long as we don't miss 32 consecutive packets,
 * this works just fine to provide enough information to determine when messages
 * tagged for guaranteed delivery should be resent.
 *
 * Note that this is a low-level class that the application developer will
 * generally not need to interact with directly.
 */
class NETCORE_EXPORT ReliabilitySystem
{
public:
   ReliabilitySystem(unsigned int max_sequence = 0xFFFFFFFF);

   void Reset();
   void PacketSent(int size);
   void PacketReceived(unsigned int sequence, int size);
   unsigned int GenerateAckBits();
   void ProcessAck(unsigned int ack, unsigned int ack_bits);
   void Update(float deltaTime);
   bool Validate() const;

   // utility functions
   static bool sequence_more_recent(unsigned int s1, unsigned int s2, unsigned int max_sequence);
   static int bit_index_for_sequence(unsigned int sequence, unsigned int ack, unsigned int max_sequence);
   static unsigned int generate_ack_bits(unsigned int ack, const PacketQueue& received_queue, unsigned int max_sequence);
   static void process_ack(unsigned int ack, unsigned int ack_bits,
                      PacketQueue& pending_ack_queue, PacketQueue& acked_queue,
                      std::vector<unsigned int>& acks, unsigned int& acked_packets,
                      float& rtt, unsigned int max_sequence);

   // data accessors
   unsigned int GetLocalSequence() const { return mLocalSequence; } // note: this is the sequence number for the NEXT packet to be sent
   unsigned int GetRemoteSequence() const { return mRemoteSequence; }
   unsigned int GetMaxSequence() const { return mMaxSequence; }

   //void GetAcks(unsigned int** acks, int& count);

   unsigned int GetSentPackets() const { return mSentPackets; }
   unsigned int GetReceivedPackets() const { return mRecvPackets; }
   unsigned int GetLostPackets() const { return mLostPackets; }
   unsigned int GetAckedPackets() const { return acked_packets; }
   float GetSentBandwidth() const { return mSentBandwidth; }
   float GetAckedBandwidth() const { return mAckedBandwidth; }
   float GetRoundTripTime() const { return mRoundTripTime; }
   int GetHeaderSize() const { return 12; }

   const PacketQueue& GetRecentlyAckedPackets() const { return mRecentlyAckedPackets; }
   const PacketQueue& GetRecentlyLostPackets() const { return mRecentlyLostPackets; }

protected:
   void AdvanceQueueTime(float deltaTime);
   void UpdateQueues();
   void UpdateStats();

private:
   unsigned int mMaxSequence;        // maximum sequence value before wrap around (used to test sequence wrap at low # values)
   unsigned int mLocalSequence;      // local sequence number for most recently sent packet
   unsigned int mRemoteSequence;     // remote sequence number for most recently received packet

   unsigned int mSentPackets;        // total number of packets sent
   unsigned int mRecvPackets;        // total number of packets received
   unsigned int mLostPackets;        // total number of packets lost
   unsigned int acked_packets;       // total number of packets acked

   float mSentBandwidth;             // approximate sent bandwidth over the last second
   float mAckedBandwidth;            // approximate acked bandwidth over the last second
   float mRoundTripTime;             // estimated round trip time
   float mRoundTripTimeMaximum;      // maximum expected round trip time (hard coded to one second for the moment)

#pragma warning (push)
#pragma warning (disable:4251)
   std::vector<unsigned int> mAcks;  // acked packets from last set of packet receives. cleared each update!
#pragma warning (pop)

   PacketQueue mSentQueue;           // sent packets used to calculate sent bandwidth (kept until rtt_maximum)
   PacketQueue mPendingAckQueue;     // sent packets which have not been acked yet (kept until rtt_maximum * 2)
   PacketQueue mReceivedQueue;       // received packets for determining acks to send (kept up to most recent recv sequence - 32)
   PacketQueue mAckedQueue;          // acked packets (kept until rtt_maximum * 2)

   // queues for storing recent changes; they get cleared every update
   PacketQueue mRecentlyAckedPackets;
   PacketQueue mRecentlyLostPackets; // recently lost packets (queued here, to be used as desired)
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // RELIABILITYSYSTEM_H
