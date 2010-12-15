#ifndef PACKET_PROCESSOR__H
#define PACKET_PROCESSOR__H

////////////////////////////////////////////////////////////////////////////////

#include <map>

#include <NetSetGo/NetCore/Address.h>

namespace net
{
   class PacketParser;
}

////////////////////////////////////////////////////////////////////////////////

namespace net {

   /**
    * PacketProcessor
    *
    * Provides a means to register any number of parsers, organized by unique
    * protocol ID. It is assumed that each packet type begins with the protocol
    * ID, and the PacketProcessor will peek for this ID, and choose the
    * appropriate parser to parse the packet. Note that the data passed to the
    * PacketParser is from the beginning, meaning it will need to read over the
    * protocol ID.
    */
   class NETCORE_EXPORT PacketProcessor
   {
   public:
      typedef int ProtocolID;

      void RegisterParser(ProtocolID protocolID, PacketParser* parser);
      void RemoveParser(ProtocolID protocolID); // note: does no de-allocations
      PacketParser* GetParser(ProtocolID protocolID);
      const PacketParser* GetParser(ProtocolID protocolID) const;

      bool ProcessPacket(const Address& sender, const unsigned char data[], size_t size) const;

   protected:
   private:
      typedef std::map<ProtocolID, PacketParser*> ProtocolToParserMap;
      ProtocolToParserMap mProtocolToParserMap;
   };

} // namespace net

////////////////////////////////////////////////////////////////////////////////

#endif // PACKET_PROCESSOR__H
