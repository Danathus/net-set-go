#include <NetSetGo/NetCore/FlowControl.h>

#include <stdio.h>

#define VERBOSE 0

namespace net {

////////////////////////////////////////////////////////////////////////////////

   FlowControl::FlowControl()
   {
      //printf("flow control initialized\n");
      Reset();
   }

   void FlowControl::Reset()
   {
      mMode = Bad;
      mPenaltyTime = 4.0f;
      mGoodConditionsTime = 0.0f;
      mPenaltyReductionAccumulator = 0.0f;
   }

   void FlowControl::Update(float deltaTime, float rtt)
   {
      const float RTT_Threshold = 250.0f;

      if (mMode == Good)
      {
         if (rtt > RTT_Threshold)
         {
#if VERBOSE
            printf("%s:%d\t*** dropping to bad mode ***\n", __FUNCTION__, __LINE__);
#endif
            mMode = Bad;
            if (mGoodConditionsTime < 10.0f && mPenaltyTime < 60.0f)
            {
               mPenaltyTime *= 2.0f;
               if (mPenaltyTime > 60.0f)
               {
                  mPenaltyTime = 60.0f;
               }
#if VERBOSE
               printf("%s:%d\tpenalty time increased to %f\n", __FUNCTION__, __LINE__, mPenaltyTime);
#endif
            }
            mGoodConditionsTime = 0.0f;
            mPenaltyReductionAccumulator = 0.0f;
            return;
         }

         mGoodConditionsTime += deltaTime;
         mPenaltyReductionAccumulator += deltaTime;

         if (mPenaltyReductionAccumulator > 10.0f && mPenaltyTime > 1.0f)
         {
            mPenaltyTime /= 2.0f;
            if (mPenaltyTime < 1.0f)
            {
               mPenaltyTime = 1.0f;
            }
#if VERBOSE
            printf("%s:%d\tpenalty time reduced to %f\n", __FUNCTION__, __LINE__, mPenaltyTime);
#endif
            mPenaltyReductionAccumulator = 0.0f;
         }
      }

      if (mMode == Bad)
      {
         if (rtt <= RTT_Threshold)
         {
            mGoodConditionsTime += deltaTime;
         }
         else
         {
            mGoodConditionsTime = 0.0f;
         }

         if (mGoodConditionsTime > mPenaltyTime)
         {
#if VERBOSE
            printf("%s:%d\t*** upgrading to good mode ***\n", __FUNCTION__, __LINE__);
#endif
            mGoodConditionsTime = 0.0f;
            mPenaltyReductionAccumulator = 0.0f;
            mMode = Good;
         }
      }
   }

   float FlowControl::GetSendRate() const
   {
      const float sendRate = mMode == Good ? 30.0f : 10.0f;
      return sendRate;
   }

////////////////////////////////////////////////////////////////////////////////

} // namespace net
