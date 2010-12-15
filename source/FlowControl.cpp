#include <NetCore/FlowControl.h>
#include <dtUtil/log.h>
#include <dtUtil/stringutils.h>

#include <stdio.h>

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
            LOGN_DEBUG("NetCore", "*** dropping to bad mode ***");
            mMode = Bad;
            if (mGoodConditionsTime < 10.0f && mPenaltyTime < 60.0f)
            {
               mPenaltyTime *= 2.0f;
               if (mPenaltyTime > 60.0f)
               {
                  mPenaltyTime = 60.0f;
               }
               LOGN_DEBUG("NetCore", "penalty time increased to " + dtUtil::ToString(mPenaltyTime));
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
            LOGN_DEBUG("NetCore", "penalty time reduced to " + dtUtil::ToString(mPenaltyTime));
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
            LOGN_DEBUG("NetCore", "*** upgrading to good mode ***");
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
