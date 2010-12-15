#ifndef FLOW_CONTROL__H
#define FLOW_CONTROL__H

////////////////////////////////////////////////////////////////////////////////

#include <NetSetGo/NetCore/NetCoreExport.h>

////////////////////////////////////////////////////////////////////////////////

namespace net {

   /**
    * FlowControl
    *
    * A simple algorithm to track the latency of the network over time and
    * adapt an optimal rate of packets to be sent per second. In this simple
    * implementation there are simply two modes, good and bad, with good mode
    * meaning 30 packets/second and bad mode meaning 10 packets/second.
    */
   class NETCORE_EXPORT FlowControl
   {
   public:
      FlowControl();

      void Reset();

      /**
       * @param deltaTime The time in seconds since the last Update()
       * @param rtt The current measured round trip time for network packets
       */
      void Update(float deltaTime, float rtt);
      float GetSendRate() const;

   private:
      enum Mode
      {
         Good,
         Bad
      };

      Mode mMode;
      float mPenaltyTime;
      float mGoodConditionsTime;
      float mPenaltyReductionAccumulator;
   };

} // namespace net

////////////////////////////////////////////////////////////////////////////////

#endif // FLOW_CONTROL__H
