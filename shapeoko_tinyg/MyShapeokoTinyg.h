#ifndef _MY_SHAPEOKO_TINYG_H_
#define _MY_SHAPEOKO_TINYG_H_


#include "MMDevice.h"
#include "DeviceBase.h"

#define ERR_UNKNOWN_POSITION 101
#define ERR_INITIALIZE_FAILED 102
#define ERR_WRITE_FAILED 103
#define ERR_CLOSE_FAILED 104
#define ERR_BOARD_NOT_FOUND 105
#define ERR_PORT_OPEN_FAILED 106
#define ERR_COMMUNICATION 107
#define ERR_NO_PORT_SET 108
#define ERR_VERSION_MISMATCH 109

#define PARAMETERS_COUNT 23

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

  // Code from EVA_NDE_Grbl (hub)
   MM::DeviceDetectionStatus DetectDevice(void);
   int DetectInstalledDevices();

   // property handlers
   int OnPort(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnVersion(MM::PropertyBase* pPropt, MM::ActionType eAct);
   int OnStatus(MM::PropertyBase* pProp, MM::ActionType pAct);
   int OnCommand(MM::PropertyBase* pProp, MM::ActionType pAct);
   // custom interface for child devices
   bool IsPortAvailable() {return portAvailable_;}
   bool IsTimedOutputActive() {return timedOutputActive_;}
   void SetTimedOutput(bool active) {timedOutputActive_ = active;}

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
          LogMessage("GetSerialAnswer:", term);
          int ret = GetSerialAnswer(port_.c_str(),term,ans);
          LogMessage("ret=" + ret);
          LogMessage("ans=" + ans);
          return ret;
	}
   static MMThreadLock& GetLock() {return lock_;}

   int SendCommand(std::string command, std::string &returnString);
   int SetAnswerTimeoutMs(double timout);
   int SetSync(int axis, double value );

   int GetParameters();
   int SetParameter(int index, double value);
   std::vector<double> parameters;
   //int Reset();
   double MPos[3];
   double WPos[3];
   int GetStatus(); 
   std::string status;

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
	   int MoveBlocking(long x, long y, bool relative = false);

	   double syncStep_;

   bool initialized_;            // true if the device is intitalized
 bool home_;                   // true if stage is homed

  
   enum MOVE_MODE {MOVE, MOVEREL, HOME};
   MOVE_MODE lastMode_;

  class CommandThread;
   CommandThread* cmdThread_;    // thread used to execute move commands

   double answerTimeoutMs_;      // max wait for the device to answer
   double moveTimeoutMs_;        // max wait for stage to finish moving
   MMThreadLock executeLock_;

   std::string commandResult_;
   std::string port_;
   std::string version_;
   bool portAvailable_;
   bool timedOutputActive_;
   static MMThreadLock lock_;



};


#endif  // _MY_SHAPEOKO_TINYG_H_
