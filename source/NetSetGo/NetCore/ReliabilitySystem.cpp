#include <NetSetGo/NetCore/ReliabilitySystem.h>

#include <cassert>

namespace net {

////////////////////////////////////////////////////////////////////////////////

   ReliabilitySystem::ReliabilitySystem(unsigned int max_sequence)
      : mMaxSequence(max_sequence)
   {
      Reset();
   }

   void ReliabilitySystem::Reset()
   {
      mLocalSequence        = 0;
      mRemoteSequence       = 0;
      mSentPackets          = 0;
      mRecvPackets          = 0;
      mLostPackets          = 0;
      acked_packets         = 0;
      mSentBandwidth        = 0.0f;
      mAckedBandwidth       = 0.0f;
      mRoundTripTime        = 0.0f;
      mRoundTripTimeMaximum = 1.0f;

      mSentQueue.clear();
      mReceivedQueue.clear();
      mPendingAckQueue.clear();
      mAckedQueue.clear();

      mRecentlyAckedPackets.clear();
      mRecentlyLostPackets.clear();
   }

   void ReliabilitySystem::PacketSent(int size)
   {
      if (mSentQueue.exists(mLocalSequence))
      {
         printf("local sequence %d exists\n", mLocalSequence);
         for (PacketQueue::iterator itor = mSentQueue.begin(); itor != mSentQueue.end(); ++itor)
            printf(" + %d\n", itor->mSequence);
      }
      assert(!mSentQueue.exists(mLocalSequence));
      assert(!mPendingAckQueue.exists(mLocalSequence));
      PacketData data;
      data.mSequence = mLocalSequence;
      data.mTime = 0.0f;
      data.mSize = size;
      mSentQueue.push_back(data);
      mPendingAckQueue.push_back(data);
      ++mSentPackets;
      ++mLocalSequence;
      if (mLocalSequence > mMaxSequence)
      {
         mLocalSequence = 0;
      }
   }

   void ReliabilitySystem::PacketReceived(unsigned int sequence, int size)
   {
      ++mRecvPackets;
      if (mReceivedQueue.exists(sequence))
      {
         return;
      }
      PacketData data;
      data.mSequence = sequence;
      data.mTime = 0.0f;
      data.mSize = size;
      mReceivedQueue.push_back(data);
      if (sequence_more_recent(sequence, mRemoteSequence, mMaxSequence))
      {
         mRemoteSequence = sequence;
      }
   }

   unsigned int ReliabilitySystem::GenerateAckBits()
   {
      return generate_ack_bits(GetRemoteSequence(), mReceivedQueue, mMaxSequence);
   }

   void ReliabilitySystem::ProcessAck(unsigned int ack, unsigned int ack_bits)
   {
      process_ack(ack, ack_bits, mPendingAckQueue, mAckedQueue, mAcks, acked_packets, mRoundTripTime, mMaxSequence);
   }

   void ReliabilitySystem::Update(float deltaTime)
   {
      mAcks.clear();
      AdvanceQueueTime(deltaTime);
      UpdateQueues();
      UpdateStats();
      #ifdef NET_UNIT_TEST
      assert(Validate());
      #endif
   }

   bool ReliabilitySystem::Validate() const
   {
      bool validated = true; // true until proven otherwise
      validated = validated && mSentQueue.verify_sorted(mMaxSequence);
      validated = validated && mReceivedQueue.verify_sorted(mMaxSequence);
      validated = validated && mPendingAckQueue.verify_sorted(mMaxSequence);
      validated = validated && mAckedQueue.verify_sorted(mMaxSequence);
      return validated;
   }

   // utility functions

   bool ReliabilitySystem::sequence_more_recent(unsigned int s1, unsigned int s2, unsigned int max_sequence)
   {
      return (s1 > s2) && (s1 - s2 <= max_sequence/2)
         ||  (s2 > s1) && (s2 - s1 > max_sequence/2);
   }

   int ReliabilitySystem::bit_index_for_sequence(unsigned int sequence, unsigned int ack, unsigned int max_sequence)
   {
      assert(sequence != ack);
      assert(!sequence_more_recent(sequence, ack, max_sequence));
      if (sequence > ack)
      {
         assert(ack < 33);
         assert(max_sequence >= sequence);
         return ack + (max_sequence - sequence);
      }
      else
      {
         assert(ack >= 1);
         assert(sequence <= ack - 1);
         return ack - 1 - sequence;
      }
   }

   unsigned int ReliabilitySystem::generate_ack_bits(unsigned int ack, const PacketQueue& received_queue, unsigned int max_sequence)
   {
      unsigned int ack_bits = 0;
      for (PacketQueue::const_iterator itor = received_queue.begin(); itor != received_queue.end(); itor++)
      {
         if (itor->mSequence == ack || sequence_more_recent(itor->mSequence, ack, max_sequence))
         {
            break;
         }
         int bit_index = bit_index_for_sequence(itor->mSequence, ack, max_sequence);
         if (bit_index <= 31)
         {
            ack_bits |= 1 << bit_index;
         }
      }
      return ack_bits;
   }

   void ReliabilitySystem::process_ack(unsigned int ack, unsigned int ack_bits,
                      PacketQueue& pending_ack_queue, PacketQueue& acked_queue,
                      std::vector<unsigned int>& acks, unsigned int& acked_packets,
                      float& rtt, unsigned int max_sequence)
   {
      if (pending_ack_queue.empty())
      {
         return;
      }

      PacketQueue::iterator itor = pending_ack_queue.begin();
      while (itor != pending_ack_queue.end())
      {
         bool acked = false;

         if (itor->mSequence == ack)
         {
            acked = true;
         }
         else if (!sequence_more_recent(itor->mSequence, ack, max_sequence))
         {
            int bit_index = bit_index_for_sequence(itor->mSequence, ack, max_sequence);
            if (bit_index <= 31)
            {
               acked = (ack_bits >> bit_index) & 1;
            }
         }

         if (acked)
         {
            rtt += (itor->mTime - rtt) * 0.1f;

            acked_queue.insert_sorted(*itor, max_sequence);
            acks.push_back(itor->mSequence);
            acked_packets++;
            itor = pending_ack_queue.erase(itor);
         }
         else
         {
            ++itor;
         }
      }
   }

   /*
   // data accessors
   void ReliabilitySystem::GetAcks(unsigned int** acks, int& count)
   {
      *acks = &mAcks[0];
      count = (int)mAcks.size();
   }
   //*/

////////////////////////////////////////////////////////////////////////////////

   void ReliabilitySystem::AdvanceQueueTime(float deltaTime)
   {
      for (PacketQueue::iterator itor = mSentQueue.begin(); itor != mSentQueue.end(); ++itor)
      {
         itor->mTime += deltaTime;
      }

      for (PacketQueue::iterator itor = mReceivedQueue.begin(); itor != mReceivedQueue.end(); ++itor)
      {
         itor->mTime += deltaTime;
      }

      for (PacketQueue::iterator itor = mPendingAckQueue.begin(); itor != mPendingAckQueue.end(); ++itor)
      {
         itor->mTime += deltaTime;
      }

      for (PacketQueue::iterator itor = mAckedQueue.begin(); itor != mAckedQueue.end(); ++itor)
      {
         itor->mTime += deltaTime;
      }
   }

   void ReliabilitySystem::UpdateQueues()
   {
      const float epsilon = 0.001f;

      while (mSentQueue.size() && mSentQueue.front().mTime > mRoundTripTimeMaximum + epsilon)
      {
         mSentQueue.pop_front();
      }

      if (mReceivedQueue.size())
      {
         const unsigned int latest_sequence = mReceivedQueue.back().mSequence;
         const unsigned int minimum_sequence = latest_sequence >= 34 ? (latest_sequence - 34) : mMaxSequence - (34 - latest_sequence);
         while (mReceivedQueue.size() && !sequence_more_recent(mReceivedQueue.front().mSequence, minimum_sequence, mMaxSequence))
         {
            mReceivedQueue.pop_front();
         }
      }

      mRecentlyAckedPackets.clear();
      while (mAckedQueue.size() && mAckedQueue.front().mTime > mRoundTripTimeMaximum * 2 - epsilon)
      {
         mRecentlyAckedPackets.push_back(mAckedQueue.front());
         mAckedQueue.pop_front();
      }

      mRecentlyLostPackets.clear();
      while (mPendingAckQueue.size() && mPendingAckQueue.front().mTime > mRoundTripTimeMaximum + epsilon)
      {
         //printf("ReliabilitySystem: uhoh, lost packet seq# %u\n", mPendingAckQueue.front().mSequence);
         mRecentlyLostPackets.push_back(mPendingAckQueue.front());
         mPendingAckQueue.pop_front();
         ++mLostPackets;
      }
   }

   void ReliabilitySystem::UpdateStats()
   {
      int sent_bytes_per_second = 0;
      for (PacketQueue::iterator itor = mSentQueue.begin(); itor != mSentQueue.end(); ++itor)
      {
         sent_bytes_per_second += itor->mSize;
      }
      int acked_packets_per_second = 0;
      int acked_bytes_per_second = 0;
      for (PacketQueue::iterator itor = mAckedQueue.begin(); itor != mAckedQueue.end(); ++itor)
      {
         if (itor->mTime >= mRoundTripTimeMaximum)
         {
            acked_packets_per_second++;
            acked_bytes_per_second += itor->mSize;
         }
      }
      sent_bytes_per_second = int(float(sent_bytes_per_second) / mRoundTripTimeMaximum);
      acked_bytes_per_second = int(float(acked_bytes_per_second) / mRoundTripTimeMaximum);
      mSentBandwidth = sent_bytes_per_second * (8 / 1000.0f);
      mAckedBandwidth = acked_bytes_per_second * (8 / 1000.0f);
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
