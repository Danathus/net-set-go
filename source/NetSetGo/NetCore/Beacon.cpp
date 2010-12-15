#include <NetSetGo/NetCore/Beacon.h>

#include <cassert>
#include <stdio.h>

#include <NetSetGo/NetCore/netassert.h>
#include <NetSetGo/NetCore/Serialization.h>

namespace net {

////////////////////////////////////////////////////////////////////////////////

   BeaconHeader::Data::Data()
      : mZero(0)
      , mProtocolID(0)
      , mServerPort(0)
      , mNameLength(0)
   {
      SetName("<Undefined>");
   }

   bool BeaconHeader::Data::SetName(const std::string& name)
   {
      bool success = name.length() < sizeof(mName)-1;

      if (success)
      {
         mNameLength = name.length();
         memcpy(mName, &name[0], mNameLength);
         mName[mNameLength] = '\0';
      }

      return success;
   }

   size_t BeaconHeader::Serialize(unsigned char* buffer, size_t numBytes) const
   {
      bool success = true;

      size_t position = 0;

      assert(mData.mNameLength < 63);

      if (success = success && (position + sizeof(mData.mZero)       <= numBytes)) { position += WriteInteger(&buffer[position], mData.mZero);       }
      if (success = success && (position + sizeof(mData.mProtocolID) <= numBytes)) { position += WriteInteger(&buffer[position], mData.mProtocolID); }
      if (success = success && (position + sizeof(mData.mServerPort) <= numBytes)) { position += WriteInteger(&buffer[position], mData.mServerPort); }
      if (success = success && (position + sizeof(mData.mNameLength) <= numBytes)) { position += WriteByte(&buffer[position], mData.mNameLength);    }
      if (success = success && (position +        mData.mNameLength  <= numBytes)) { memcpy(&buffer[position], &mData.mName[0], mData.mNameLength); position += mData.mNameLength; }

      return position;
   }

   size_t BeaconHeader::Deserialize(const unsigned char* buffer, size_t numBytes)
   {
      bool success = true;

      size_t position = 0;

      if (success = success && (position + sizeof(mData.mZero) <= numBytes))
      {
         unsigned int zero;
         position += ReadInteger(&buffer[position], zero);
         success = success && zero == mData.mZero;
      }
      if (success = success && (position + sizeof(mData.mProtocolID) <= numBytes)) { position += ReadInteger(&buffer[position], mData.mProtocolID); }
      if (success = success && (position + sizeof(mData.mServerPort) <= numBytes)) { position += ReadInteger(&buffer[position], mData.mServerPort); }
      if (success = success && (position + sizeof(mData.mNameLength) <= numBytes))
      {
         position += ReadByte(&buffer[position], mData.mNameLength);
         success = success && mData.mNameLength < 63;
      }
      if (success = success && (position +        mData.mNameLength  <= numBytes))
      {
         memcpy(mData.mName, &buffer[position], mData.mNameLength); position += mData.mNameLength;
         mData.mName[mData.mNameLength] = '\0';
      }

      return position;
   }

   size_t BeaconHeader::GetSize() const
   {
      const size_t size =
         sizeof(int)  +   // mZero
         sizeof(int)  +   // mProtocolID
         sizeof(int)  +   // mServerPort
         sizeof(char) +   // mNameLength
         sizeof(char)*64; // mName
      return size;
   }

////////////////////////////////////////////////////////////////////////////////

   BeaconTransmitter::BeaconTransmitter(Serializable* userData)
      : mIsConfigured(false)
      , mIsRunning(false)
      , mSocket(Socket::Broadcast | Socket::NonBlocking)
      , mListenerPort(0)
      , mpUserData(userData)
      , mDelayBetweenBeacons(0.1f) // by default send at 10 Hz
      , mTimeAccumulator(0.0f)
   {
   }

   BeaconTransmitter::~BeaconTransmitter()
   {
      if (IsRunning())
      {
         Stop();
      }
   }

   void BeaconTransmitter::Configure(const std::string& name, unsigned int protocolID, unsigned int listenerPort, unsigned int serverPort, Serializable* userData)
   {
      GetHeader().GetData().SetName(name);
      GetHeader().GetData().mProtocolID = protocolID;
      mListenerPort = listenerPort;
      GetHeader().GetData().mServerPort = serverPort;
      mpUserData = userData;

      mIsConfigured = true;
   }

   bool BeaconTransmitter::Start(int port)
   {
      if (!IsConfigured())
      {
         return false;
      }

      netassert(!IsRunning());
      if (IsRunning())
      {
         printf("failure to start beacon on port %d; already started on another port\n", port);
         return false;
      }
      printf("start beacon on port %d\n", port);
      if (!mSocket.Open(port))
      {
         return false;
      }
      mIsRunning = true;
      mTimeAccumulator = 0.0f; // resetting time accumulator
      return IsRunning();
   }

   void BeaconTransmitter::Stop()
   {
      if (IsRunning())
      {
         printf("stop beacon\n");
         mSocket.Close();
         mIsRunning = false;
      }
   }

   void BeaconTransmitter::Update(float deltaTime)
   {
      if (IsRunning())
      {
         for (mTimeAccumulator += deltaTime; mTimeAccumulator > mDelayBetweenBeacons; mTimeAccumulator -= mDelayBetweenBeacons)
         {
            const bool success = SendBeacon();

            if (!success)
            {
               printf("failed to send broadcast packet\n");
            }
         }
      }
   }

   bool BeaconTransmitter::SendBeacon()
   {
      bool success = true; // until proven otherwise

      const size_t packetBufferSize = mHeader.GetSize() + (mpUserData ? mpUserData->GetSize() : 0);
      unsigned char* packetBuffer = reinterpret_cast<unsigned char*>(alloca(packetBufferSize));

      size_t position = 0;
      {
         if (success)
         {
            const size_t bytesWritten = mHeader.Serialize(&packetBuffer[position], packetBufferSize - position);
            position = (bytesWritten < 0) ? bytesWritten : (position + bytesWritten);
            success = bytesWritten >= 0 && bytesWritten <= mHeader.GetSize();
         }
         if (success && mpUserData)
         {
            const size_t bytesWritten = mpUserData->Serialize(&packetBuffer[position], packetBufferSize - position);
            position = (bytesWritten < 0) ? bytesWritten : (position + bytesWritten);
            success = bytesWritten >= 0 && bytesWritten <= mpUserData->GetSize();
         }
      }

      if (success)
      {
         success = mSocket.Send(Address(255,255,255,255, mListenerPort), packetBuffer, position);
      }

      return success;
   }

////////////////////////////////////////////////////////////////////////////////

   BeaconReceiver::BeaconReceiver(int beaconListenerPort, Serializable* userData)
      : mpUserData(userData)
   {
      mSocket.Open(beaconListenerPort);
   }

   BeaconReceiver::~BeaconReceiver()
   {
      mSocket.Close();

      // This needs to be both created and destroyed in the 
      // same place.  Temporarily deleting it here.
      delete mpUserData;
      mpUserData = NULL;
   }

   bool BeaconReceiver::ReceiveBeacon(net::Address& senderAddress)
   {
      bool success = false;

      const size_t packetBufferSize = mHeader.GetSize() + (mpUserData ? mpUserData->GetSize() : 0);
      unsigned char* packetBuffer = reinterpret_cast<unsigned char*>(alloca(packetBufferSize));

      if (mSocket.Receive(senderAddress, packetBuffer, packetBufferSize))
      {
         success = true; // we received data; success is true until proven otherwise
         size_t position = 0;

         if (success)
         {
            const size_t bytesRead = mHeader.Deserialize(&packetBuffer[position], packetBufferSize - position);
            position = (bytesRead < 0) ? bytesRead : (position + bytesRead);
            success = success && bytesRead >= 0 && bytesRead <= mHeader.GetSize();
         }
         if (success && mpUserData)
         {
            const size_t bytesRead = mpUserData->Deserialize(&packetBuffer[position], packetBufferSize - position);
            position = (bytesRead < 0) ? bytesRead : (position + bytesRead);
            success = success && bytesRead >= 0 && bytesRead <= mpUserData->GetSize();
         }
      }

      /*
      if (success)
      {
         printf("received beacon! %d protocol, \"%s\"@<%d.%d.%d.%d:%d>, -> %d\n",
            mHeader.GetData().mProtocolID, mHeader.GetData().mName,
            senderAddress.GetA(), senderAddress.GetB(), senderAddress.GetC(), senderAddress.GetD(),
            senderAddress.GetPort(),
            mHeader.GetData().mServerPort);
      }
      //*/

      return success;
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
