#ifndef ADDRESS_H
#define ADDRESS_H

#include <NetSetGo/NetCore/NetCoreExport.h>

#include <string>

#define DOTTED_QUAD_TO_INT(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

namespace net {

////////////////////////////////////////////////////////////////////////////////

// internet address

/**
 * Address
 *
 * This is a class that wraps around the dotted quad and port used in an IPv4 address.
 */
class NETCORE_EXPORT Address
{
public:
   Address();
   Address(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port);
   Address(unsigned int ipAddress, unsigned short port);
   Address(const std::string& ipAddress, unsigned short port);
   Address(const std::string& dotted_quad_colon_port);

   unsigned int GetAddress() const { return mAddress; }

   unsigned char GetA() const { return (unsigned char)(mAddress >> 24); }
   unsigned char GetB() const { return (unsigned char)(mAddress >> 16); }
   unsigned char GetC() const { return (unsigned char)(mAddress >> 8); }
   unsigned char GetD() const { return (unsigned char)(mAddress); }
   unsigned short GetPort() const { return mPort; }

   bool operator==(const Address& other) const
   {
      return mAddress == other.mAddress && mPort == other.mPort;
   }

   bool operator!=(const Address& other) const
   {
      return !(*this == other);
   }

   bool operator <(const Address& other) const
   {
      // note: this is so we can use address as a key in std::map
      const bool isLessThan = mAddress < other.mAddress || (mAddress == other.mAddress && mPort < other.mPort);
      return isLessThan;
   }

   bool operator >(const Address& other) const
   {
      const bool isGreaterThan = mAddress > other.mAddress || (mAddress == other.mAddress && mPort > other.mPort);
      return isGreaterThan;
   }

   bool IsMulticastAddress() const
   {
      const bool isMulticastAddress = GetA() >= 224 && GetA() <= 239;
      return isMulticastAddress;
   }

   std::string ToString() const;

private:
   unsigned int mAddress;
   unsigned short mPort;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // ADDRESS_H
