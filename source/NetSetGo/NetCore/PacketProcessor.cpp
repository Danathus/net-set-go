#include <NetSetGo/NetCore/PacketProcessor.h>

#include <cassert>

#include <NetSetGo/NetCore/PacketParser.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

void PacketProcessor::RegisterParser(ProtocolID protocolID, PacketParser* parser)
{
   assert(parser);
   mProtocolToParserMap[protocolID] = parser;
}

void PacketProcessor::RemoveParser(ProtocolID protocolID)
{
   ProtocolToParserMap::iterator parserFound = mProtocolToParserMap.find(protocolID);
   if (parserFound != mProtocolToParserMap.end())
   {
      mProtocolToParserMap.erase(parserFound);
   }
}

PacketParser* PacketProcessor::GetParser(ProtocolID protocolID)
{
   return mProtocolToParserMap[protocolID];
}

const PacketParser* PacketProcessor::GetParser(ProtocolID protocolID) const
{
   ProtocolToParserMap::const_iterator parserFound = mProtocolToParserMap.find(protocolID);
   const PacketParser* parser = (parserFound != mProtocolToParserMap.end()) ? parserFound->second : 0;
   return parser;
}

bool PacketProcessor::ProcessPacket(const Address& sender, const unsigned char data[], size_t size) const
{
   bool processed = false;

   ProtocolID protocolID;

   // peek to find the protocol ID
   memcpy(&protocolID, data, sizeof(ProtocolID));

   // perform a lookup in the map
   ProtocolToParserMap::const_iterator parserFound = mProtocolToParserMap.find(protocolID);
   if (parserFound != mProtocolToParserMap.end())
   {
      // parser is found!
      const PacketParser* parser = parserFound->second;

      // attempt to parse the packet
      processed = parser->ParsePacket(sender, data, size);
   }

   // report results
   return processed;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace net
