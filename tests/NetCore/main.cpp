#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <OpenThreads/Thread> // for sleeping in loops

//#include <execinfo.h>

////////////////////////////////////////////////////////////////////////////////

/*
void print_trace()
{
   void *array[10];
   size_t size;
   char **strings;
   size_t i;

   size = backtrace( array, 10 );
   strings = backtrace_symbols( array, size );

   printf( "Obtained %zd stack frames.\n", size );

   for ( i = 0; i < size; i++ )
      printf( "%s\n", strings[i] );

   free( strings );
}

#define test_assert( expression ) do { if ( !(expression) ) { print_trace(); exit(1); } } while(0)
/*/
#define test_assert( expression ) \
   do { if (!(expression)) {      \
   printf("assertion failure, line %d file %s: \"%s\"\n", __LINE__, __FILE__, #expression); \
   exit(1); } }                   \
   while (0)
//*/

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/Address.h>

void testAddress()
{
   net::Address address(1, 2, 3, 4, 5);
   test_assert(address.GetA() == 1);
   test_assert(address.GetB() == 2);
   test_assert(address.GetC() == 3);
   test_assert(address.GetD() == 4);
   test_assert(address.GetPort() == 5);
   test_assert(address.GetAddress() == DOTTED_QUAD_TO_INT(1, 2, 3, 4));
   test_assert(address == net::Address(DOTTED_QUAD_TO_INT(1, 2, 3, 4), 5));
   test_assert(address == net::Address("1.2.3.4", 5));
   test_assert(address == net::Address("1.2.3.4:5"));
   test_assert(address != net::Address(DOTTED_QUAD_TO_INT(1, 2, 3, 4), 6));
   test_assert(address < net::Address(DOTTED_QUAD_TO_INT(1, 2, 3, 4), 6));
   test_assert(address > net::Address(DOTTED_QUAD_TO_INT(1, 2, 3, 4), 4));
   test_assert(!net::Address(1, 2, 3, 4, 5).IsMulticastAddress());
   test_assert(net::Address(230, 2, 3, 4, 5).IsMulticastAddress());
   test_assert(address.ToString() == "1.2.3.4:5");
}

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/Beacon.h>

void testBeaconHeader()
{
   net::BeaconHeader header;
   test_assert(header.GetSize() == 13+64);
   test_assert(header.GetData().mZero == 0);
   {
      const int maxNameLengthVarValue = (1 << (sizeof(header.GetData().mNameLength) * 8)) - 1;
      test_assert(maxNameLengthVarValue > sizeof(header.GetData().mName));
   }
   const bool longSuccess = header.GetData().SetName("this is a name that you will find is just a little bit too long.");
   test_assert(!longSuccess);
   const bool shortSuccess = header.GetData().SetName("malarky");
   test_assert(shortSuccess);
   test_assert(strcmp(header.GetData().mName, "malarky") == 0);
   header.GetData().mServerPort = 1234;
   header.GetData().mProtocolID = 5678;

   unsigned char buffer[1024];

   // test partial serialize
   {
      memset(buffer, 0, sizeof(buffer));
      const size_t bytesWritten = header.Serialize(buffer, 10);
      test_assert(bytesWritten <= 10);
      // all the bytes past what we said we wrote should be untouched
      for (size_t i = bytesWritten; i < sizeof(buffer); ++i)
      {
         test_assert(buffer[i] == 0);
      }
   }

   // test full serialize
   const size_t fullSerializeBytesWritten = header.Serialize(buffer, sizeof(buffer));
   {
      test_assert(fullSerializeBytesWritten < header.GetSize());
      // all the bytes past what we said we wrote should be untouched
      for (size_t i = fullSerializeBytesWritten; i < sizeof(buffer); ++i)
      {
         test_assert(buffer[i] == 0);
      }
   }

   // test full deserialize
   {
      net::BeaconHeader copy;
      const size_t bytesRead = copy.Deserialize(buffer, sizeof(buffer));
      test_assert(bytesRead == fullSerializeBytesWritten);
      test_assert(copy.GetData().mZero == 0);
      test_assert(copy.GetData().mProtocolID == header.GetData().mProtocolID);
      test_assert(copy.GetData().mServerPort == header.GetData().mServerPort);
      test_assert(copy.GetData().mNameLength == header.GetData().mNameLength);
      test_assert(strcmp(copy.GetData().mName, header.GetData().mName) == 0);
   }
}

void testBeaconTransmitterAndReceiver()
{
   const std::string serverName   = "serverName";
   const int         protocolID   = 4321;
   const short       listenerPort = 1234;
   const short       serverPort   = 1243;
   const short       sendingPort  = 1324;

   net::BeaconTransmitter transmitter;
   net::BeaconReceiver receiver(listenerPort);

   // we should not be able to start transmitting before we've configured
   {
      test_assert(!transmitter.IsConfigured());
      test_assert(!transmitter.IsRunning());
      const bool falseStartResult = transmitter.Start(sendingPort);
      test_assert(!falseStartResult);
      test_assert(!transmitter.IsRunning());
   }

   // prepare
   {
      // configure
      transmitter.Configure(serverName, protocolID, listenerPort, serverPort);
      test_assert(transmitter.IsConfigured());

      // start sending
      test_assert(!transmitter.IsRunning());
      const bool startedOkay = transmitter.Start(sendingPort);
      test_assert(startedOkay);
      test_assert(transmitter.IsRunning());
   }

   // test sending and receiving beacons
   {
      const int kExpectedBeaconCount = 10;
      const float kMaxSecondsToWait  = 3.0f;
      const int kMicrosecondsToSleep = 1000; // 1 millisecond
      const double kFrameTime = 1.0 / 1000000.0 * kMicrosecondsToSleep; // time to sleep in seconds
      test_assert(kFrameTime - 0.00001 < 0.001f); // these are to make sure the kFrameTime conversion was correct
      test_assert(kFrameTime + 0.00001 > 0.001f); // (should be about 1 millisecond)

      // count the number of test beacons we receive; we should get kExpectedBeaconCount within kMaxSecondsToWait seconds
      int countTestBeaconsReceived = 0;
      float timeOut = 0; // time in seconds to wait before abandoning test

      while (countTestBeaconsReceived < kExpectedBeaconCount && timeOut < kMaxSecondsToWait)
      {
         // update beacon transmitter
         transmitter.Update(kFrameTime);

         // receive beacons
         net::Address senderAddress;
         while (receiver.ReceiveBeacon(senderAddress))
         {
            const net::BeaconHeader::Data& receivedHeaderData = receiver.GetHeader().GetData();

            test_assert(receivedHeaderData.mZero == 0);
            printf("detected beacon:\n\tprotocol id:\t%d\n\tserver port:\t%d\n\tserver name:\t%s\n",
               receivedHeaderData.mProtocolID,
               receivedHeaderData.mServerPort,
               receivedHeaderData.mName);
            // we want to screen out protocol ID so that we don't respond to beacons from other applications
            // be sure to follow the convention of picking a different protocol ID per application
            if (receivedHeaderData.mProtocolID == protocolID)
            {
               // received a test beacon (most likely one of our own)
               ++countTestBeaconsReceived;
            }
         }

         // wait for a millisecond
         OpenThreads::Thread::microSleep(kMicrosecondsToSleep);

         // increment our timeout timer
         timeOut += kFrameTime;
      }

      printf("received %d test beacons in %f seconds\n", countTestBeaconsReceived, timeOut);
      // assert that we received the beacons we were looking for before the time ran out
      test_assert(countTestBeaconsReceived >= kExpectedBeaconCount);
      test_assert(timeOut < kMaxSecondsToWait);
      // should match the expected frequency based on the delay between beacons
      {
         const float measuredFrequency = countTestBeaconsReceived / timeOut;
         const float expectedFrequency = 1.0f / transmitter.GetDelayBetweenBeacons();
         printf("measured frequency %f; expected frequency %f\n", measuredFrequency, expectedFrequency);
         test_assert(measuredFrequency - 0.001f < expectedFrequency);
         test_assert(measuredFrequency + 0.001f > expectedFrequency);
      }
   }

   // wrap up
   {
      test_assert(transmitter.IsRunning());
      transmitter.Stop();
      test_assert(!transmitter.IsRunning());
   }
}

void testBeacon()
{
   testBeaconHeader();
   testBeaconTransmitterAndReceiver();
}

////////////////////////////////////////////////////////////////////////////////

// todo: write FlowControl unit tests

////////////////////////////////////////////////////////////////////////////////

// todo: write GuaranteedDeliverySystem unit tests

////////////////////////////////////////////////////////////////////////////////

// todo: write Mesh unit tests

////////////////////////////////////////////////////////////////////////////////

// todo: write NetworkTopology unit tests

/*
// trying to send data that's too big should fail
{
// measure the max packet size
// make some space that's bigger than that
// attempt to send
// expect failure
zz;
}
//*/

////////////////////////////////////////////////////////////////////////////////

// todo: write Node unit tests

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/PacketProcessor.h>
#include <NetSetGo/NetCore/PacketParser.h>

class TestPacketParserA : public net::PacketParser
{
public:
   static const int kProtocolID = 1234;

   mutable bool parsedFlag;

   // implementation of PacketParser interface
   bool ParsePacket(const net::Address& sender, const unsigned char data[], size_t size) const
   {
      bool success = false;

      if (size >= sizeof(kProtocolID))
      {
         int readProtocolID;
         test_assert(sizeof(readProtocolID) == sizeof(kProtocolID));
         memcpy(&readProtocolID, data, sizeof(readProtocolID));
         success = readProtocolID == kProtocolID;
         test_assert(success); // it really shouldn't get in here unless the protocol ID is right

         // note: would parse other data after here...
      }

      parsedFlag = success;

      return success;
   }
};

class TestPacketParserB : public net::PacketParser
{
public:
   static const int kProtocolID = 1235;

   mutable bool parsedFlag;

   // implementation of PacketParser interface
   bool ParsePacket(const net::Address& sender, const unsigned char data[], size_t size) const
   {
      bool success = false;

      if (size >= sizeof(kProtocolID))
      {
         int readProtocolID;
         test_assert(sizeof(readProtocolID) == sizeof(kProtocolID));
         memcpy(&readProtocolID, data, sizeof(readProtocolID));
         success = readProtocolID == kProtocolID;
         test_assert(success); // it really shouldn't get in here unless the protocol ID is right

         // note: would parse other data after here...
      }

      parsedFlag = success;

      return success;
   }
};

void testPacketProcessor()
{
   net::PacketProcessor packetProcessor;
   TestPacketParserA packetParserA;
   TestPacketParserB packetParserB;

   // test adding
   test_assert(packetProcessor.GetParser(TestPacketParserA::kProtocolID) == NULL);
   packetProcessor.RegisterParser(TestPacketParserA::kProtocolID, &packetParserA);
   test_assert(packetProcessor.GetParser(TestPacketParserA::kProtocolID) == &packetParserA);
   test_assert(packetProcessor.GetParser(TestPacketParserA::kProtocolID) == static_cast<const net::PacketProcessor&>(packetProcessor).GetParser(TestPacketParserA::kProtocolID));

   // load a second parser in there to prepare for parsing tests
   packetProcessor.RegisterParser(TestPacketParserB::kProtocolID, &packetParserB);

   // run some parsing tests
   {
      net::Address sender;
      int protocolID;

      // test parsing packet A
      {
         protocolID = TestPacketParserA::kProtocolID;
         packetParserA.parsedFlag = packetParserB.parsedFlag = false;
         const bool result = packetProcessor.ProcessPacket(sender, (unsigned char *)&protocolID, sizeof(protocolID));
         test_assert(result);
         test_assert(packetParserA.parsedFlag);
         test_assert(!packetParserB.parsedFlag);
      }

      // test parsing packet B
      {
         protocolID = TestPacketParserB::kProtocolID;
         packetParserA.parsedFlag = packetParserB.parsedFlag = false;
         const bool result = packetProcessor.ProcessPacket(sender, (unsigned char *)&protocolID, sizeof(protocolID));
         test_assert(result);
         test_assert(!packetParserA.parsedFlag);
         test_assert(packetParserB.parsedFlag);
      }

      // test parsing garbage
      {
         protocolID = 0xbadf00d;
         packetParserA.parsedFlag = packetParserB.parsedFlag = false;
         const bool result = packetProcessor.ProcessPacket(sender, (unsigned char *)&protocolID, sizeof(protocolID));
         test_assert(!result);
         test_assert(!packetParserA.parsedFlag);
         test_assert(!packetParserB.parsedFlag);
      }
   }

   // test removing
   test_assert(packetProcessor.GetParser(TestPacketParserA::kProtocolID) != NULL);
   packetProcessor.RemoveParser(TestPacketParserA::kProtocolID);
   test_assert(packetProcessor.GetParser(TestPacketParserA::kProtocolID) == NULL);
}

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/PacketQueue.h>

void testPacketQueue()
{
   net::PacketQueue queue;
   const int kMaxSequence = 1000;

   test_assert(!queue.exists(0));
   {
      const size_t kNumPackets = 100;
      const size_t kNumSwaps   = 300;

      // make a list of ints 1 to kNumPackets
      std::vector<int> sequenceNumbers;
      for (size_t i = 0; i < kNumPackets; ++i)
      {
         sequenceNumbers.push_back(i % kMaxSequence);
      }

      // jumble the ordering
      for (size_t i = 0; i < kNumSwaps; ++i)
      {
         const unsigned int indexA = rand() % kNumPackets;
         const unsigned int indexB = rand() % kNumPackets;

         std::swap(sequenceNumbers[indexA], sequenceNumbers[indexB]);
      }

      // loop through inserting them one at a time
      for (size_t i = 0; i < kNumPackets; ++i)
      {
         queue.insert_sorted(net::PacketData(sequenceNumbers[i], float(i), 64), kMaxSequence);
         // make sure all inserted so far can still be found
         for (size_t j = 0; j <= i; ++j)
         {
            test_assert(queue.exists(sequenceNumbers[j]));
         }
         test_assert(queue.verify_sorted(kMaxSequence));
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

// todo: write ReliabilitySystem unit tests

////////////////////////////////////////////////////////////////////////////////

// todo: write Serialization unit tests

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/Socket.h>

void testSocket()
{
   // test opening and closing sockets
   {
      const unsigned short kOnePort   = 1234;
      const unsigned short kOtherPort = 1235;

      test_assert(kOnePort != kOtherPort);

      net::Socket socket;
      {
         test_assert(!socket.IsOpen());
         const bool openSuccess = socket.Open(kOnePort);
         test_assert(openSuccess);
         test_assert(socket.IsOpen());
         test_assert(socket.GetPort() == kOnePort);
      }

      // trying to re-open the same port should fail
      {
         net::Socket failure;
         test_assert(!failure.IsOpen());
         const bool openShouldFail = failure.Open(kOnePort);
         test_assert(!openShouldFail);
         test_assert(!failure.IsOpen());
      }

      // opening two separate ports should be okay
      {
         net::Socket otherPort;
         test_assert(!otherPort.IsOpen());
         const bool openShouldSucceed = otherPort.Open(kOtherPort);
         test_assert(openShouldSucceed);
         test_assert(otherPort.IsOpen());
         test_assert(otherPort.GetPort() == kOtherPort);

         // should automatically close when it goes out of scope...
      }

      // confirm that that last socket did in fact close when it went out of scope
      {
         net::Socket otherPortReturns;
         test_assert(!otherPortReturns.IsOpen());
         const bool openShouldSucceed = otherPortReturns.Open(kOtherPort);
         test_assert(openShouldSucceed);
         test_assert(otherPortReturns.IsOpen());
      }

      // now try closing our original socket
      socket.Close();
      test_assert(!socket.IsOpen());

      // opening another port with the same socket should be okay
      {
         const bool openSuccess = socket.Open(kOtherPort);
         test_assert(openSuccess);
         test_assert(socket.IsOpen());
      }

      // re-opening that same port with another socket should be okay
      {
         net::Socket samePortAgain;
         test_assert(!samePortAgain.IsOpen());
         const bool openShouldSucceed = samePortAgain.Open(kOnePort);
         test_assert(openShouldSucceed);
         test_assert(samePortAgain.IsOpen());
      }
   }

   // test sending and receiving
   {
      const unsigned short kSenderPort   = 1234;
      const unsigned short kReceiverPort = 1235;

      test_assert(kSenderPort != kReceiverPort);

      const float kMaxSecondsToWait  = 3.0f;
      const int kMicrosecondsToSleep = 1000; // 1 millisecond
      const double kFrameTime = 1.0 / 1000000.0 * kMicrosecondsToSleep; // time to sleep in seconds
      test_assert(kFrameTime - 0.00001 < 0.001f); // these are to make sure the kFrameTime conversion was correct
      test_assert(kFrameTime + 0.00001 > 0.001f); // (should be about 1 millisecond)

      // create two sockets
      net::Socket sender, receiver;
      {
         sender.Open(kSenderPort);
         receiver.Open(kReceiverPort);
         test_assert(sender.IsOpen());
         test_assert(receiver.IsOpen());
      }

      // test sending from one and receiving from the other
      const char sendData[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

      // note that message delivery isn't guaranteed, so what we'll
      // do is loop w/ a pause, and if time runs out without
      // reception, we'll give up and consider it a failure
      bool messageReceived = false;
      float timeOut = 0.0f;
      while (!messageReceived && timeOut < kMaxSecondsToWait)
      {
         char recvData[10];
         net::Address senderAddress;
         const std::string localhostIP = "127.0.0.1";

         // have the sender send, and confirm send success
         {
            // send to localhost, with port being that which the receiver has bound to
            const net::Address destination(localhostIP, kReceiverPort);
            const bool sendSuccess = sender.Send(destination, sendData, sizeof(sendData));
            test_assert(sendSuccess);
         }

         // trying to read with too few bytes should necessarily fail
         {
            const int numBytesRead = receiver.Receive(senderAddress, recvData, sizeof(recvData) - 1);
            test_assert(numBytesRead == 0);
         }

         // let the receiver try to receive
         {
            const int numBytesRead = receiver.Receive(senderAddress, recvData, sizeof(recvData));

            if (numBytesRead) // if we do receive something...
            {
               // should be the same size as that which was sent
               test_assert(numBytesRead == sizeof(sendData));

               // contents should match
               const int difference = memcmp(sendData, recvData, numBytesRead);
               test_assert(difference == 0);

               // sender address should be as expected
               test_assert(senderAddress == net::Address(localhostIP, kSenderPort));

               // confirm that we received the message
               messageReceived = true;
            }
         }

         // wait for a millisecond
         OpenThreads::Thread::microSleep(kMicrosecondsToSleep);

         // increment our timeout timer
         timeOut += kFrameTime;
      }

      // assert that we received the message before the time ran out
      test_assert(messageReceived);
      test_assert(timeOut < kMaxSecondsToWait);
   }

   // test broadcast sockets
   {
      const unsigned short kSenderPort   = 1236;
      const unsigned short kReceiverPort = 1237;

      test_assert(kSenderPort != kReceiverPort);

      const float kMaxSecondsToWait  = 3.0f;
      const int kMicrosecondsToSleep = 1000; // 1 millisecond
      const double kFrameTime = 1.0 / 1000000.0 * kMicrosecondsToSleep; // time to sleep in seconds
      test_assert(kFrameTime - 0.00001 < 0.001f); // these are to make sure the kFrameTime conversion was correct
      test_assert(kFrameTime + 0.00001 > 0.001f); // (should be about 1 millisecond)

      // create a broadcast socket and a listener socket
      net::Socket broadcaster(net::Socket::Broadcast), listener;

      // open the ports
      broadcaster.Open(kSenderPort);
      listener.Open(kReceiverPort);
      test_assert(broadcaster.IsOpen());
      test_assert(listener.IsOpen());

      // test sending from one and receiving from the other
      const char sendData[10] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };

      // loop through testing sending and receiving
      bool messageReceived = false;
      float timeOut = 0.0f;
      while (!messageReceived && timeOut < kMaxSecondsToWait)
      {
         const std::string broadcastIP = "255.255.255.255";

         // sending should be guaranteed success (even if it ends up never being received)
         {
            const bool sendSuccess = broadcaster.Send(net::Address(broadcastIP, kReceiverPort), sendData, sizeof(sendData));
            test_assert(sendSuccess);
         }

         // attempt reception
         {
            char recvData[10];
            net::Address senderAddress;
            const int numBytesRead = listener.Receive(senderAddress, recvData, sizeof(recvData));

            if (numBytesRead) // if we do receive something...
            {
               // should be the same size as that which was sent
               test_assert(numBytesRead == sizeof(sendData));

               // contents should match
               const int difference = memcmp(sendData, recvData, numBytesRead);
               test_assert(difference == 0);

               // note: the sending address will be whatever the sender's natural IP is on the network
               printf("received broadcast packet from %s\n", senderAddress.ToString().c_str());

               // sender port should be as expected
               test_assert(senderAddress.GetPort() == kSenderPort);

               // confirm that we received the message
               messageReceived = true;
            }
         }

         // wait for a millisecond
         OpenThreads::Thread::microSleep(kMicrosecondsToSleep);

         // increment our timeout timer
         timeOut += kFrameTime;
      }

      // assert that we received the message before the time ran out
      test_assert(messageReceived);
      test_assert(timeOut < kMaxSecondsToWait);
   }

   // test multi-cast sending and receiving
   {
      //
   }
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
   {
      const bool initOkay = net::InitializeSockets();
      test_assert(initOkay);
   }

   testAddress();
   testBeacon();
   testPacketProcessor();
   testPacketQueue();
   testSocket();

   {
      net::ShutdownSockets();
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
