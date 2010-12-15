#ifndef BEACON_H
#define BEACON_H

#include <string>

#include <NetCore/NetCoreExport.h>

#include <NetCore/Socket.h>
#include <NetCore/Serialization.h>

// beacon
//  + sends broadcast UDP packets to the LAN
//  + use a beacon to advertise the existence of a server on the LAN

namespace net {

////////////////////////////////////////////////////////////////////////////////

/**
 * BeaconHeader
 *
 * This is the header for the beacon sent out by the BeaconTransmitter and
 * received by the BeaconReceiver. Note that the contents of the beacon can be
 * extended by setting the UserData in the BeaconTransmitter.
 */
class NETCORE_EXPORT BeaconHeader : public Serializable
{
public:
   struct NETCORE_EXPORT Data
   {
      const unsigned int mZero;
      unsigned int mProtocolID;
      unsigned int mServerPort;
      unsigned char mNameLength;
      char mName[64];

      Data();
      bool SetName(const std::string& name);
   };

   BeaconHeader() : Serializable() {}

   Data& GetData() { return mData; }
   const Data& GetData() const { return mData; }

   // implementations of pure virtual methods
   size_t Serialize(unsigned char* buffer, size_t numBytes) const;
   size_t Deserialize(const unsigned char* buffer, size_t numBytes);
   //
   size_t GetSize() const;

protected:
private:
   Data mData;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * BeaconTransmitter
 *
 * This can be used to broadcast a beacon at regular intervals over the LAN.
 * Note that attempting to broadcast over the internet will not work (as per
 * the designers of IPv4).
 *
 * Usage notes:
 * You can extend the data to be sent in the beacon to be sent by calling
 * SetUserData(). Be sure to Configure() and then Start() and Update() every
 * frame. The NetworkEngine already has a BeaconTransmitter, so there is no
 * need to create one of these oneself.
 */
class NETCORE_EXPORT BeaconTransmitter
{
public:
   BeaconTransmitter(Serializable* userData = 0);
   ~BeaconTransmitter();

   BeaconHeader& GetHeader() { return mHeader; }
   const BeaconHeader& GetHeader() const { return mHeader; }
   void SetUserData(Serializable* userData) { mpUserData = userData; }
   Serializable* GetUserData() { return mpUserData; }
   const Serializable* GetUserData() const { return mpUserData; }

   void SetDelayBetweenBeacons(float period) { mDelayBetweenBeacons = period; }
   float GetDelayBetweenBeacons() const { return mDelayBetweenBeacons; }

   /**
    * @param name The name of the server to be included in the Beacon::Packet
    * @param protocolId An identifier for this game type (to avoid confusion with other beacons)
    * @param listenerPort The port a client should listen on to receive broadcasts of Beacon::Packets
    * @param serverPort The port a client should connect to to join the server (included in Beacon::Packet)
    */
   void Configure(const std::string& name, unsigned int protocolID, unsigned int listenerPort, unsigned int serverPort, Serializable* userData = 0);
   bool IsConfigured() const { return mIsConfigured; }

   bool IsRunning() const { return mIsRunning; }
   bool Start(int port);
   void Stop();
   void Update(float deltaTime);

protected:
   bool SendBeacon();

private:
   bool mIsConfigured;
   bool mIsRunning;
   Socket mSocket;
   unsigned int mListenerPort;

   BeaconHeader mHeader; // configured once, used for writing
   Serializable* mpUserData;

   // for timing
   float mDelayBetweenBeacons; // the period
   float mTimeAccumulator;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * BeaconReceiver.
 *
 * This can be used to receive a beacon that is sent over the LAN. Be sure that
 * the UserData is set to match that which was set on the BeaconTransmitter, as
 * appropriate.
 */
class NETCORE_EXPORT BeaconReceiver
{
public:
   BeaconReceiver(int beaconListenerPort, Serializable* userData = 0);
   ~BeaconReceiver();

   BeaconHeader& GetHeader() { return mHeader; }
   const BeaconHeader& GetHeader() const { return mHeader; }
   Serializable* GetUserData() { return mpUserData; }
   const Serializable* GetUserData() const { return mpUserData; }

   /**
    * @param senderAddress The address of the sender if a beacon was received
    * @return True iff a beacon was received
    */
   bool ReceiveBeacon(net::Address& senderAddress);

protected:
private:
   BeaconHeader mHeader; // buffer read headers here
   Serializable* mpUserData;
   net::Socket mSocket;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // BEACON_H
