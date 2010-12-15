#include <NetCore/Address.h>

#include <cassert>
#include <sstream>
#include <stdio.h> // for sscanf

namespace net {

////////////////////////////////////////////////////////////////////////////////

   Address::Address()
      : mAddress(0)
      , mPort(0)
   {
   }

   Address::Address(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port)
      : mAddress(DOTTED_QUAD_TO_INT(a,b,c,d))
      , mPort(port)
   {
   }

   Address::Address(unsigned int ipAddress, unsigned short port)
      : mAddress(ipAddress)
      , mPort(port)
   {
   }

   Address::Address(const std::string& ipAddress, unsigned short port)
      : mPort(port)
   {
      unsigned int a = 0, b = 0, c = 0, d = 0;

      sscanf(ipAddress.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);

      mAddress = DOTTED_QUAD_TO_INT(a, b, c, d);
   }

   Address::Address(const std::string& dotted_quad_colon_port)
   {
      unsigned int a = 0, b = 0, c = 0, d = 0, port = 0;

      sscanf(dotted_quad_colon_port.c_str(), "%d.%d.%d.%d:%d", &a, &b, &c, &d, &port);

      mAddress = DOTTED_QUAD_TO_INT(a, b, c, d);
      mPort = port;
   }

   std::string Address::ToString() const
   {
      std::stringstream strstrm;
      strstrm << int(GetA()) << "." << int(GetB()) << "." << int(GetC()) << "." << int(GetD()) << ":" << GetPort();
      return strstrm.str();
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
