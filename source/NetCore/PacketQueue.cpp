#include <NetCore/PacketQueue.h>

#include <cassert>

namespace net {

   inline bool sequence_more_recent(unsigned int s1, unsigned int s2, unsigned int max_sequence)
   {
      return (s1 > s2) && (s1 - s2 <= max_sequence/2) || (s2 > s1) && (s2 - s1 > max_sequence/2);
   }

////////////////////////////////////////////////////////////////////////////////

   PacketData::PacketData()
   {
      //
   }

   PacketData::PacketData(unsigned int sequence, float time, int size)
      : mSequence(sequence)
      , mTime(time)
      , mSize(size)
   {
      //
   }

   bool PacketData::operator==(const PacketData& packetData) const
   {
      const bool equal =
         mSequence == packetData.mSequence &&
         mTime == packetData.mTime &&
         mSize == packetData.mSize;

      return equal;
   }

   bool PacketData::operator<(const PacketData& packetData) const
   {
      const bool less =
         mSequence < packetData.mSequence || (mSequence == packetData.mSequence &&
            mTime < packetData.mTime || (mTime == packetData.mTime &&
               mSize < packetData.mSize));

      return less;
   }

////////////////////////////////////////////////////////////////////////////////

   bool PacketQueue::exists(unsigned int sequence) const
   {
      for (const_iterator itor = begin(); itor != end(); ++itor)
      {
         if (itor->mSequence == sequence)
         {
            return true;
         }
      }
      return false;
   }

   void PacketQueue::insert_sorted(const PacketData& p, unsigned int max_sequence)
   {
      if (empty())
      {
         push_back(p);
      }
      else
      {
         if (!sequence_more_recent(p.mSequence, front().mSequence, max_sequence))
         {
            push_front(p);
         }
         else if (sequence_more_recent(p.mSequence, back().mSequence, max_sequence))
         {
            push_back(p);
         }
         else
         {
            for (PacketQueue::iterator itor = begin(); itor != end(); itor++)
            {
               assert(itor->mSequence != p.mSequence);
               if (sequence_more_recent(itor->mSequence, p.mSequence, max_sequence))
               {
                  insert(itor, p);
                  break;
               }
            }
         }
      }
   }

   bool PacketQueue::verify_sorted(unsigned int max_sequence) const
   {
      bool verified = true; // true until proven otherwise
      PacketQueue::const_iterator prev = end();
      for (PacketQueue::const_iterator itor = begin(); verified && itor != end(); ++itor)
      {
         verified = verified && (itor->mSequence <= max_sequence);
         if (prev != end())
         {
            verified = verified && sequence_more_recent(itor->mSequence, prev->mSequence, max_sequence);
            prev = itor;
         }
      }
      return verified;
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
