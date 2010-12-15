#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include <list>

#include <NetCore/NetCoreExport.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

// packet queue to store information about sent and received packets sorted in sequence order
//  + we define ordering using the "sequence_more_recent" function, this works provided there is a large gap when sequence wrap occurs

/**
 * PacketData
 *
 * Information about a packet that was/will be sent/received. Note that the data
 * itself is not stored here, but referred to by a sequence number, intended to
 * be a key that could be used to access the data elsewhere. Note that this
 * sequence number is also used elsewhere in reliability and guaranteed delivery
 * systems.
 */
struct NETCORE_EXPORT PacketData
{
   unsigned int mSequence; // packet sequence number
   float mTime;            // time offset since packet was sent or received (depending on context)
   int mSize;              // packet size in bytes

   PacketData();
   PacketData(unsigned int sequence, float time, int size);

   bool operator==(const PacketData& packetData) const;
   bool operator<(const PacketData& packetData) const;
};

/**
 * PacketQueue
 *
 * A list of packets that is self-sorted by sequence number. This is used in the
 * reliability and guaranteed delivery systems.
 */
class NETCORE_EXPORT PacketQueue : public std::list<PacketData>
{
public:
   bool exists(unsigned int sequence) const;
   void insert_sorted(const PacketData& p, unsigned int max_sequence);
   bool verify_sorted(unsigned int max_sequence) const;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // PACKETQUEUE_H
