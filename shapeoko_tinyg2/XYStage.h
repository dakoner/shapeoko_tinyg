#ifndef _SHAPEOKO_TINYG_XYSTAGE_H_
#define _SHAPEOKO_TINYG_XYSTAGE_H_

#include "DeviceBase.h"
#include "DeviceThreads.h"

class CShapeokoTinyGXYStage : public CXYStageBase<CShapeokoTinyGXYStage>
{
public:
   CShapeokoTinyGXYStage();
   ~CShapeokoTinyGXYStage();

   bool Busy();
   void GetName(char* pszName) const;

   int Initialize();
   int Shutdown();

   // XYStage API
   /* Note that only the Set/Get PositionStep functions are implemented in the adapter
    * It is best not to override the Set/Get PositionUm functions in DeviceBase.h, since
    * those implement corrections based on whether or not X and Y directionality should be
    * mirrored and based on a user defined origin
    */

   // This must be correct or the conversions between steps and Um will go wrong
   virtual double GetStepSize() {return stepSize_um_;}
   virtual int SetPositionSteps(long x, long y);
   virtual int GetPositionSteps(long& x, long& y);
   virtual int SetRelativePositionSteps(long x, long y);
   virtual int Home() { return DEVICE_OK; }
   virtual int Stop() { return DEVICE_OK; }

   /* This sets the 0,0 position of the adapter to the current position.
    * If possible, the stage controller itself should also be set to 0,0
    * Note that this differs form the function SetAdapterOrigin(), which
    * sets the coordinate system used by the adapter
    * to values different from the system used by the stage controller
    */
   virtual int SetOrigin() { return DEVICE_OK; }

   virtual int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax)
   {
      xMin = lowerLimit_; xMax = upperLimit_;
      yMin = lowerLimit_; yMax = upperLimit_;
      return DEVICE_OK;
   }

   virtual int GetStepLimits(long& /*xMin*/, long& /*xMax*/, long& /*yMin*/, long& /*yMax*/)
   { return DEVICE_UNSUPPORTED_COMMAND; }
   double GetStepSizeXUm() { return stepSize_um_; }
   double GetStepSizeYUm() { return stepSize_um_; }
   int Move(double /*vx*/, double /*vy*/) {return DEVICE_OK;}

   int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}


   // action interface
   // ----------------
   int OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   double stepSize_um_;
   double posX_um_;
   double posY_um_;
   bool busy_;
   MM::TimeoutMs* timeOutTimer_;
   double velocity_;
   bool initialized_;
   double lowerLimit_;
   double upperLimit_;
};

#endif // _SHAPEOKO_TINYG_XYSTAGE_H_
