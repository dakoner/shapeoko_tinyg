#ifndef _MY_SHAPEOKO_TINYG_H_
#define _MY_SHAPEOKO_TINYG_H_


#include "MMDevice.h"
#include "DeviceBase.h"

class MyShapeokoTinyg : public CXYStageBase<MyShapeokoTinyg>
{
public:
	MyShapeokoTinyg(void);
	~MyShapeokoTinyg(void);
	
	// MMDevice API
	int Initialize();
	int Shutdown();
  
   void GetName(char* name) const;      
   bool Busy();

   // XYStage API
   // -----------
   int SetPositionSteps(long x, long y);
   int SetRelativePositionSteps(long x, long y);
   int GetPositionSteps(long& x, long& y);
   int Home();
   int Stop();
   int SetOrigin();
   int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax);
   int GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax);
   double GetStepSizeXUm();
   double GetStepSizeYUm();
   int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}

   // action interface
   // ----------------
   int OnStepSizeX(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnStepSizeY(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnMaxVelocity(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnAcceleration(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnMoveTimeout(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnSyncStep(MM::PropertyBase* pProp, MM::ActionType eAct);   
   int SetPositionUm(double x, double y);
   int SetRelativePositionUm(double dx, double dy);
   int GetPositionUm(double& x, double& y);

private:
	
   bool initialized_;            // true if the device is intitalized



};


#endif  // _MY_SHAPEOKO_TINYG_H_