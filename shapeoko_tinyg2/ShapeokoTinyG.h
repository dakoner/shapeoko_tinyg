///////////////////////////////////////////////////////////////////////////////
// FILE:          ShapeokoTinyG.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   The example implementation of the demo camera.
//                Simulates generic digital camera and associated automated
//                microscope devices and enables testing of the rest of the
//                system without the need to connect to the actual hardware. 
//                
// AUTHOR:        Nenad Amodaj, nenad@amodaj.com, 06/08/2005
//                
//                Karl Hoover (stuff such as programmable CCD size  & the various image processors)
//                Arther Edelstein ( equipment error simulation)
//
// COPYRIGHT:     University of California, San Francisco, 2006-2015
//                100X Imaging Inc, 2008
//
// LICENSE:       This file is distributed under the BSD license.
//                License text is included with the source distribution.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.

#ifndef _SHAPEOKO_TINYG_H_
#define _SHAPEOKO_TINYG_H_

#include "DeviceBase.h"
#include "DeviceThreads.h"
#include <string>
#include <map>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
// #define ERR_UNKNOWN_MODE         102
// #define ERR_UNKNOWN_POSITION     103
// #define ERR_IN_SEQUENCE          104
// #define ERR_SEQUENCE_INACTIVE    105
#define ERR_STAGE_MOVING         110
// #define HUB_NOT_AVAILABLE        107

#define ERR_UNKNOWN_POSITION 101
#define ERR_INITIALIZE_FAILED 102
#define ERR_WRITE_FAILED 103
#define ERR_CLOSE_FAILED 104
#define ERR_BOARD_NOT_FOUND 105
#define ERR_PORT_OPEN_FAILED 106
#define ERR_COMMUNICATION 107
#define ERR_NO_PORT_SET 108
#define ERR_VERSION_MISMATCH 109


const char* NoHubError = "Parent Hub not defined.";


////////////////////////
// ShapeokoTinyGHub
//////////////////////

class ShapeokoTinyGHub : public HubBase<ShapeokoTinyGHub>
{
public:
  ShapeokoTinyGHub();
  ~ShapeokoTinyGHub() { Shutdown();}

   // Device API
   // ---------
   int Initialize();
  int Shutdown() {initialized_ = false; return DEVICE_OK;};
   void GetName(char* pName) const; 
   bool Busy() { return busy_;} ;

   // property handlers
   int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnCommand(MM::PropertyBase* pProp, MM::ActionType pAct);

   // HUB api
   int DetectInstalledDevices();

   int SendCommand(std::string command, std::string &returnString);
   int SetAnswerTimeoutMs(double timout);
   MM::DeviceDetectionStatus DetectDevice(void);
   int PurgeComPortH() {return PurgeComPort(port_.c_str());}
   int WriteToComPortH(const unsigned char* command, unsigned len) {return WriteToComPort(port_.c_str(), command, len);}
   int ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
   {
      return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
   }
   int SetCommandComPortH(const char* command, const char* term)
   {
           return SendSerialCommand(port_.c_str(),command,term);
   }
    int GetSerialAnswerComPortH (std::string& ans,  const char* term)
        {
                return GetSerialAnswer(port_.c_str(),term,ans);
        }
   int GetStatus(); 

private:
   void GetPeripheralInventory();
   std::vector<std::string> peripherals_;
   bool initialized_;
   bool busy_;

   std::string port_;
   bool portAvailable_;
   std::string commandResult_;
   MMThreadLock executeLock_;
   double MPos[3];
   double WPos[3];
};



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
#endif //_SHAPEOKO_TINYG_H_
