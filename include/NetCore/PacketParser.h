#ifndef PACKET_PARSER__H
#define PACKET_PARSER__H

#include <NetCore/Address.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

/**
 * PacketParser
 *
 * A simple pattern for a type that parses a packet.
 */
class PacketParser
{
public:
   virtual ~PacketParser(){}

   /**
    * @param sender the source of the packet to be parsed
    * @param the data to be parsed
    * @param the size of the packet at the data to be parsed
    * @return true if and only if the packet was successfully parsed.
    */
   virtual bool ParsePacket(const Address& sender, const unsigned char data[], size_t size) const = 0;

protected:
private:
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // PACKET_PARSER__H
