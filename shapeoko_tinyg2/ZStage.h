#ifndef _SHAPEOKO_TINYG_ZSTAGE_H_
#define _SHAPEOKO_TINYG_ZSTAGE_H_

#include "MMDevice.h"
#include "DeviceBase.h"
#include <string>
#include <vector>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//

#define ERR_INVALID_SPEED            10003
#define ERR_PORT_CHANGE_FORBIDDEN    10004
#define ERR_SET_POSITION_FAILED      10005
#define ERR_INVALID_STEP_SIZE        10006
#define ERR_INVALID_MODE             10008
#define ERR_CANNOT_CHANGE_PROPERTY   10009
#define ERR_UNEXPECTED_ANSWER        10010
#define ERR_INVALID_TURRET           10011
#define ERR_SCOPE_NOT_ACTIVE         10012
#define ERR_INVALID_TURRET_POSITION  10013
#define ERR_MODULE_NOT_FOUND         10014
#define ERR_NO_FOCUS_DRIVE           10015
#define ERR_NO_XY_DRIVE              10016

// Axioskope 2 Z stage
//
class CShapeokoTinyGZStage : public CStageBase<CShapeokoTinyGZStage>
{
public:
   CShapeokoTinyGZStage();
   ~CShapeokoTinyGZStage();

   bool Busy();
   void GetName(char* pszName) const;

   int Initialize();
   int Shutdown();

   // Stage API
   virtual int SetPositionUm(double pos);
   virtual int GetPositionUm(double& pos);
   virtual double GetStepSize() const {return stepSize_um_;}
   virtual int SetPositionSteps(long steps) ;
   virtual int GetPositionSteps(long& steps);
   virtual int SetOrigin();
   virtual int GetLimits(double& lower, double& upper)
   {
      lower = lowerLimit_;
      upper = upperLimit_;
      return DEVICE_OK;
   }

   bool IsContinuousFocusDrive() const {return false;}

   // action interface
   // ----------------
   int OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnLoadSample(MM::PropertyBase* pProp, MM::ActionType eAct);

   // Sequence functions (unimplemented)
   int IsStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}
   int GetStageSequenceMaxLength(long& nrEvents) const  {nrEvents = 0; return DEVICE_OK;}
   int StartStageSequence() {return DEVICE_OK;}
   int StopStageSequence() {return DEVICE_OK;}
   int ClearStageSequence() {return DEVICE_OK;}
   int AddToStageSequence(double /*position*/) {return DEVICE_OK;}
   int SendStageSequence() {return DEVICE_OK;}

private:
   int GetFocusFirmwareVersion();
   int GetUpperLimit();
   int GetLowerLimit();
   double stepSize_um_;
      double posZ_um_;


   bool initialized_;
   double lowerLimit_;
   MM::TimeoutMs* timeOutTimer_;

   double upperLimit_;
   typedef enum {
      ZMSF_MOVING = 0x0002, // trajectory is in progress
      ZMSF_SETTLE = 0x0004  // settling after movement
   } ZmStatFlags;

};

#endif // _SHAPEOKO_TINYG_ZSTAGE_H_
