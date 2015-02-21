#include "MyShapeokoTinyg.h"
#include "ModuleInterface.h"


using namespace std;

const char* g_ShapeokoTinygDeviceName= "ShapeokoTinyG";


const char* g_SerialNumberProp = "SerialNumber";
const char* g_ModelNumberProp = "ModelNumber";
const char* g_SWVersionProp = "SoftwareVersion";

const char* g_StepSizeXProp = "StepSizeX";
const char* g_StepSizeYProp = "StepSizeY";
const char* g_MaxVelocityProp = "MaxVelocity";
const char* g_AccelProp = "Acceleration";
const char* g_MoveTimeoutProp = "MoveTimeoutMs";

const long xAxisMaxSteps = 2200000L;   // maximum number of steps in X
const long yAxisMaxSteps = 1500000L;   // maximum number of steps in Y
const double stepSizeUm = 0.05;        // step size in microns
const double accelScale = 13.7438;     // scaling factor for acceleration
const double velocityScale = 134218.0; // scaling factor for velocity

const char* g_SyncStepProp = "SyncStep";

const char* g_versionProp = "Version";
const char* g_normalLogicString = "Normal";
const char* g_invertedLogicString = "Inverted";

// static lock
MMThreadLock MyShapeokoTinyg::lock_;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
/**
 * List all supported hardware devices here
 */
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_ShapeokoTinygDeviceName, MM::XYStageDevice, "ShapeokoTinyG");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   // decide which device class to create based on the deviceName parameter
   if (strcmp(deviceName, g_ShapeokoTinygDeviceName) == 0)
   {
      // create camera
      return new MyShapeokoTinyg();
   }

   // ...supplied name not recognized
   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}


// From EVA's XYStage
///////////////////////////////////////////////////////////////////////////////
// CommandThread class
// (for executing move commands)
///////////////////////////////////////////////////////////////////////////////

class MyShapeokoTinyg::CommandThread : public MMDeviceThreadBase
{
   public:
      CommandThread(MyShapeokoTinyg* stage) :
         stop_(false), moving_(false), stage_(stage), errCode_(DEVICE_OK) {}

      virtual ~CommandThread() {}
	  
      int svc()
      {
		if (!stage_->IsPortAvailable()) {
			return ERR_NO_PORT_SET;
		}
		while(1)
		{
			int ret = stage_->GetStatus();
			if(ret != DEVICE_OK){
				moving_ = false;
				break;
			}
			char buf[30]="";
			ret = stage_->GetProperty("Status",buf);
			if((strcmp(buf,"Idle") == 0 )){
				moving_ = false;
				break;
			}
			else
				moving_ = true;
			CDeviceUtils::SleepMs(10);
		}

         return 0;
      }
      void Stop() {stop_ = true;}
      bool GetStop() {return stop_;}
      int GetErrorCode() {return errCode_;}
      bool IsMoving()  {  return moving_;}

   private:
      void Reset() {stop_ = false; errCode_ = DEVICE_OK; moving_ = false;}
      enum Command {MOVE, MOVEREL, HOME};
      bool stop_;
      bool moving_;
      MyShapeokoTinyg* stage_;
      long x_;
      long y_;
      Command cmd_;
      Command lastCmd_;
      int errCode_;
};


MyShapeokoTinyg::MyShapeokoTinyg() :
   CXYStageBase<MyShapeokoTinyg>(),
   initialized_(false),
   home_(false),
   answerTimeoutMs_(1000.0),
   moveTimeoutMs_(10000.0),

   cmdThread_(0) 
{
  // From EVA's XYStage.cpp

   // set default error messages
   InitializeDefaultErrorMessages();


   // create pre-initialization properties
   // ------------------------------------

   // Name
   CreateProperty(MM::g_Keyword_Name, g_ShapeokoTinygDeviceName, MM::String, true);

   // Description
   CreateProperty(MM::g_Keyword_Description, "XY stage adapter for ShapeOko Tinyg", MM::String, true);


  cmdThread_ = new CommandThread(this);

   // From EVA_NDE_Grbl
   portAvailable_ = false;

   timedOutputActive_ = false;

   WPos[0] = 0.0;
   WPos[1] = 0.0;
   WPos[2] = 0.0;
   MPos[0] = 0.0;
   MPos[1] = 0.0;
   MPos[2] = 0.0;

   InitializeDefaultErrorMessages();

   SetErrorText(ERR_PORT_OPEN_FAILED, "Failed opening EVA_NDE_Grbl USB device");
   SetErrorText(ERR_BOARD_NOT_FOUND, "Did not find an EVA_NDE_Grbl board with the correct firmware.  Is the EVA_NDE_Grbl board connected to this serial port?");
   SetErrorText(ERR_NO_PORT_SET, "Hub Device not found.  The EVA_NDE_Grbl Hub device is needed to create this device");
   std::ostringstream errorText;
   errorText << "The firmware version on the EVA_NDE_Grbl is not compatible with this adapter.  Please use firmware version ";

   SetErrorText(ERR_VERSION_MISMATCH, errorText.str().c_str());

   CPropertyAction* pAct  = new CPropertyAction(this, &MyShapeokoTinyg::OnPort);
   //CreateProperty("ComPort", "Undifined", MM::String, false, pAct);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

   pAct = new CPropertyAction(this, &MyShapeokoTinyg::OnStatus);
   CreateProperty("Status", "-", MM::String, true, pAct);  //read only
}

MyShapeokoTinyg::~MyShapeokoTinyg()
{
   Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
// XY Stage API
// required device interface implementation
///////////////////////////////////////////////////////////////////////////////
void MyShapeokoTinyg::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_ShapeokoTinygDeviceName);
}

int MyShapeokoTinyg::Initialize()
{
  LogMessage("1");
   /* From EVA's XYStage */
   int ret = DEVICE_ERR;

   // initialize device and get hardware information


   // confirm that the device is supported


   // check if we are already homed

   CPropertyAction* pAct;

   // Step size
   CreateProperty(g_StepSizeXProp, CDeviceUtils::ConvertToString(stepSizeUm), MM::Float, true);
   CreateProperty(g_StepSizeYProp, CDeviceUtils::ConvertToString(stepSizeUm), MM::Float, true);

   // Max Speed
   pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnMaxVelocity);
   CreateProperty(g_MaxVelocityProp, "100.0", MM::Float, false, pAct);
   //SetPropertyLimits(g_MaxVelocityProp, 0.0, 31999.0);

   // Acceleration
   pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnAcceleration);
   CreateProperty(g_AccelProp, "100.0", MM::Float, false, pAct);
   //SetPropertyLimits("Acceleration", 0.0, 150);

   // Move timeout
   pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnMoveTimeout);
   CreateProperty(g_MoveTimeoutProp, "10000.0", MM::Float, false, pAct);
   //SetPropertyLimits("Acceleration", 0.0, 150);

   // Sync Step
   pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnSyncStep);
   CreateProperty(g_SyncStepProp, "1.0", MM::Float, false, pAct);
   //SetPropertyLimits("Acceleration", 0.0, 150);

  LogMessage("2");

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;
  LogMessage("3");

   /* From EVA_NDE_Grbl */

   MMThreadGuard myLock(lock_);

   pAct = new CPropertyAction(this, &MyShapeokoTinyg::OnVersion);
   std::ostringstream sversion;
   sversion << version_;
   CreateProperty(g_versionProp, sversion.str().c_str(), MM::String, true, pAct);

   // pAct = new CPropertyAction(this, &MyShapeokoTinyg::OnCommand);
   // ret = CreateProperty("Command","", MM::String, false, pAct);
   // if (DEVICE_OK != ret)
   //    return ret;
   // // turn off verbose serial debug messages
   // GetCoreCallback()->SetDeviceProperty(port_.c_str(), "Verbose", "0");
   // // synchronize all properties
   // // --------------------------
   // ret = UpdateStatus();
   // if (ret != DEVICE_OK)
   //    return ret;

   initialized_ = true;
   return DEVICE_OK;
}

int MyShapeokoTinyg::Shutdown()
{
   if (cmdThread_ && cmdThread_->IsMoving())
   {
      cmdThread_->Stop();
      cmdThread_->wait();
   }

   delete cmdThread_;
   cmdThread_ = 0;

   if (initialized_)
      initialized_ = false;

   return DEVICE_OK;
}

template <class Type>
Type stringToNum(const std::string& str)
{
	std::istringstream iss(str);
	Type num;
	iss >> num;
	return num;    
}

bool MyShapeokoTinyg::Busy()
{
	return false;
}

// private and expects caller to:
// 1. guard the port
// 2. purge the port
int MyShapeokoTinyg::GetStatus()
{
   std::string cmd;
   // cmd.assign("?"); // x step/mm
   // std::string returnString;
   // int ret = SendCommand(cmd,returnString);
   // if (ret != DEVICE_OK)
   // {
   //       LogMessage("command send failed!");
   //  return ret;
   // }
   // std::vector<std::string> tokenInput;
   //      char* pEnd;
   // 	CDeviceUtils::Tokenize(returnString, tokenInput, "<>,:\r\n");
   // //sample: <Idle,MPos:0.000,0.000,0.000,WPos:0.000,0.000,0.000>
   //      if(tokenInput.size() != 9)
   //      {
   //      	LogMessage(returnString.c_str());
   //      	LogMessage("echo error!");
   //      	return DEVICE_ERR;
   //      }
   //      status.assign(tokenInput[0].c_str());
   //      MPos[0] = stringToNum<double>(tokenInput[2]);
   //      MPos[1] = stringToNum<double>(tokenInput[3]);
   //      MPos[2] = stringToNum<double>(tokenInput[4]);
   //      WPos[0] = stringToNum<double>(tokenInput[6]);
   //      WPos[1] = stringToNum<double>(tokenInput[7]);
   //      WPos[2] = stringToNum<double>(tokenInput[8]);
   return DEVICE_OK;

}
int MyShapeokoTinyg::SetSync(int axis, double value ){
   std::string cmd;
   char buff[20];
   sprintf(buff, "M108P%.3fQ%d", value,axis);
   cmd.assign(buff); 
   std::string returnString;
   int ret = SendCommand(cmd,returnString);
   return ret;
}
int MyShapeokoTinyg::SetParameter(int index, double value){
   std::string cmd;
   char buff[20];
   sprintf(buff, "$%d=%.3f", index,value);
   cmd.assign(buff); 
   std::string returnString;
   int ret = SendCommand(cmd,returnString);
   return ret;
}
//int MyShapeokoTinyg::Reset(){
//	MMThreadGuard(this->executeLock_);
//   std::string cmd;
//   char buff[]={0x18,0x00};
//   cmd.assign(buff); 
//   std::string returnString;
//   SetAnswerTimeoutMs(2000.0);
//	PurgeComPortH();
//   int ret = SetCommandComPortH(cmd.c_str(),"\n");
//   if (ret != DEVICE_OK)
//    return ret;
//   std::string an;
//   ret = GetSerialAnswerComPortH(an,"]\r\n");
//   if (ret != DEVICE_OK)
//    return ret;
//   returnString = an;
//
//   std::vector<std::string> tokenInput;
//	char* pEnd;
//   	CDeviceUtils::Tokenize(returnString, tokenInput, "\r\n[");
//   if(tokenInput.size() != 2)
//	   return DEVICE_ERR;
//   version_ = tokenInput[0];
//   return ret;
//}

int MyShapeokoTinyg::GetParameters()
{
	/*
	sample parameters:
	$0=250.000 (x, step/mm)
	$1=250.000 (y, step/mm)
	$2=250.000 (z, step/mm)
	$3=10 (step pulse, usec)
	$4=250.000 (default feed, mm/min)
	$5=500.000 (default seek, mm/min)
	$6=192 (step port invert mask, int:11000000)
	$7=25 (step idle delay, msec)
	$8=10.000 (acceleration, mm/sec^2)
	$9=0.050 (junction deviation, mm)
	$10=0.100 (arc, mm/segment)
	$11=25 (n-arc correction, int)
	$12=3 (n-decimals, int)
	$13=0 (report inches, bool)
	$14=1 (auto start, bool)
	$15=0 (invert step enable, bool)
	$16=0 (hard limits, bool)
	$17=0 (homing cycle, bool)
	$18=0 (homing dir invert mask, int:00000000)
	$19=25.000 (homing feed, mm/min)
	$20=250.000 (homing seek, mm/min)
	$21=100 (homing debounce, msec)
	$22=1.000 (homing pull-off, mm)
	*/
   if(!portAvailable_)
	   return ERR_NO_PORT_SET;
   std::string cmd;
   cmd.assign("$$"); // x step/mm
   std::string returnString;
   int ret = SendCommand(cmd,returnString);
   if (ret != DEVICE_OK)
    return ret;
   
   	std::vector<std::string> tokenInput;

	char* pEnd;
   	CDeviceUtils::Tokenize(returnString, tokenInput, "$=()\r\n");
   if(tokenInput.size() != PARAMETERS_COUNT*3)
	   return DEVICE_ERR;
   parameters.clear();
   std::vector<std::string>::iterator it;
   for (it=tokenInput.begin(); it!=tokenInput.end(); it+=3)
   {
	   std::string str = *(it+1);
	   parameters.push_back(stringToNum<double>(str));
   }
   return DEVICE_OK;

}
int MyShapeokoTinyg::SendCommand(std::string command, std::string &returnString)
{
   if(!portAvailable_)
	   return ERR_NO_PORT_SET;
   // needs a lock because the other Thread will also use this function
   MMThreadGuard(this->executeLock_);
   PurgeComPortH();
   int ret = DEVICE_OK;
   	if(command.c_str()[0] == '$' && command.c_str()[1] == 'H') 
	{
		// Check that we have a controller:
		ret = GetStatus();
		if( DEVICE_OK != ret) 
		return ret;
		ret = GetParameters();
		if( DEVICE_OK != ret)
		return ret;
	   	SetAnswerTimeoutMs(60000.0);
	}
   else if(command.c_str()[0] == '$' || command.c_str()[0] == '?')
	  SetAnswerTimeoutMs(5000.0);//for long command
   else
	  SetAnswerTimeoutMs(300.0); //for normal command

   ret = SetCommandComPortH(command.c_str(),"\n");
   if (ret != DEVICE_OK)
   {
	    LogMessage(std::string("command write fail"));
	   return ret;
   }
   std::string an;
   if(command.c_str()[0] == 0x18 ){
        CDeviceUtils::SleepMs(600);
	    ret = GetParameters();
	    returnString.assign("ok");
		LogMessage(std::string("Reset!"));
		return DEVICE_OK;
   }
   else if(command.c_str()[0] == '$' || command.c_str()[0] == '?'){

		ret = GetSerialAnswerComPortH(an,"ok\r\n");
	   //ret = comPort->Read(answer,3,charsRead);
	   if (ret != DEVICE_OK)
	   {
		   LogMessage(std::string("answer get error!"));
		  return ret;
	   }
	   returnString.assign(an);
	   return DEVICE_OK;

   }
   else{ 
	   try
	   {

			ret = GetSerialAnswerComPortH(an,"\r\n");
		   //ret = comPort->Read(answer,3,charsRead);
		   if (ret != DEVICE_OK)
		   {
			   LogMessage(std::string("answer get error!_"));
			  return ret;
		   }
		   LogMessage(std::string(an),true);
		   //sample:>>? >><Idle,MPos:0.000,0.000,0.000,WPos:0.000,0.000,0.000>
		   if (an.length() <1)
			  return DEVICE_ERR;
		   returnString.assign(an);
		   if (returnString.find("ok") != std::string::npos)
			   return DEVICE_OK;
		   else
			   return DEVICE_ERR;
	   }
	   catch(...)
	   {
		  LogMessage("Exception in send command!");
		  return DEVICE_ERR;
	   }

   }
}

MM::DeviceDetectionStatus MyShapeokoTinyg::DetectDevice(void)
{
  if (initialized_)
      return MM::CanCommunicate;

   // all conditions must be satisfied...
   MM::DeviceDetectionStatus result = MM::Misconfigured;
   char answerTO[MM::MaxStrLength];
   
   try
   {
      std::string portLowerCase = port_;
      for( std::string::iterator its = portLowerCase.begin(); its != portLowerCase.end(); ++its)
      {
         *its = (char)tolower(*its);
      }
      if( 0< portLowerCase.length() &&  0 != portLowerCase.compare("undefined")  && 0 != portLowerCase.compare("unknown") )
      {
         result = MM::CanNotCommunicate;
         // record the default answer time out
         GetCoreCallback()->GetDeviceProperty(port_.c_str(), "AnswerTimeout", answerTO);

         // device specific default communication parameters
         // for Arduino Duemilanova
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking, "Off");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, "9600" );
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
         // Arduino timed out in GetControllerVersion even if AnswerTimeout  = 300 ms
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "500.0");
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs", "0");
         MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str());
         pS->Initialize();
         // The first second or so after opening the serial port, the Arduino is waiting for firmwareupgrades.  Simply sleep 1 second.
         CDeviceUtils::SleepMs(2000);
         MMThreadGuard myLock(lock_);
         PurgeComPort(port_.c_str());
         int ret = GetStatus();
         // later, Initialize will explicitly check the version #
         if( DEVICE_OK != ret )
         {
            LogMessageCode(ret,true);
         }
         else
         {
            // to succeed must reach here....
            result = MM::CanCommunicate;
         }
         pS->Shutdown();
         // always restore the AnswerTimeout to the default
         GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", answerTO);

      }
   }
   catch(...)
   {
      LogMessage("Exception in DetectDevice!",false);
   }

   return result;
}


int MyShapeokoTinyg::DetectInstalledDevices()
{
   if (MM::CanCommunicate == DetectDevice()) 
   {
      // std::vector<std::string> peripherals; 
      // peripherals.clear();
      // peripherals.push_back(g_ShapeokoTinygDeviceName);
      // //peripherals.push_back(g_DeviceNameEVA_NDE_GrblZStage);

      // for (size_t i=0; i < peripherals.size(); i++) 
      // {
      //    MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
      //    if (pDev) 
      //    {
      //       AddInstalledDevice(pDev);
      //    }
      // }
   }

   return DEVICE_OK;
}

int MyShapeokoTinyg::SetAnswerTimeoutMs(double timeout)
{
      if(!portAvailable_)
	   return ERR_NO_PORT_SET;
     GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout",  CDeviceUtils::ConvertToString(timeout));
   return DEVICE_OK;
}


int MyShapeokoTinyg::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
      pProp->Set(port_.c_str());
   }
   else if (pAct == MM::AfterSet)
   {
      pProp->Get(port_);
      portAvailable_ = true;
   }
   return DEVICE_OK;
}
int MyShapeokoTinyg::OnStatus(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
	  int ret = GetStatus();
	  if(ret != DEVICE_OK)
		  pProp->Set("-");
	  else
		  pProp->Set(status.c_str());
   }
   return DEVICE_OK;
}
int MyShapeokoTinyg::OnVersion(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
	   pProp->Set(version_.c_str());
   }
   return DEVICE_OK;
}

int MyShapeokoTinyg::OnCommand(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
	   pProp->Set(commandResult_.c_str());
   }
   else if (pAct == MM::AfterSet)
   {
	   std::string cmd;
      pProp->Get(cmd);
	  if(cmd.compare(commandResult_) ==0)  // command result still there
		  return DEVICE_OK;
	  int ret = SendCommand(cmd,commandResult_);
	  if(DEVICE_OK != ret){
		  commandResult_.assign("Error!");
		  return DEVICE_ERR;
	  }
   }
   return DEVICE_OK;
}

// From EVA's XYStage.cpp 

double MyShapeokoTinyg::GetStepSizeXUm()
{
  if (!IsPortAvailable()) {
    return ERR_NO_PORT_SET;
  }
   return 1000.0/(parameters)[0];
}

double MyShapeokoTinyg::GetStepSizeYUm()
{
  if (!IsPortAvailable()) {
    return ERR_NO_PORT_SET;
  }
  return 1000.0/(parameters)[1];
}

int MyShapeokoTinyg::SetPositionSteps(long x, long y)
{

   SetPositionUm(x*GetStepSizeXUm(), y*GetStepSizeYUm());
   CDeviceUtils::SleepMs(10); // to make sure that there is enough time for thread to get started

   return DEVICE_OK;   
}
 
int MyShapeokoTinyg::SetRelativePositionSteps(long x, long y)
{
   SetRelativePositionUm(x*GetStepSizeXUm(), y*GetStepSizeYUm());
   return DEVICE_OK;
}
int MyShapeokoTinyg::GetPositionUm(double& x, double& y){
   int ret;
   
   if (!IsPortAvailable()) {
     return ERR_NO_PORT_SET;
   }
    ret = GetStatus();
	if (ret != DEVICE_OK)
    return ret;
	x =  MPos[0]*1000.0 ;
	y =   MPos[1]*1000.0;
   ostringstream os;
   os << "GetPositionSteps(), X=" << x << ", Y=" << y;
   LogMessage(os.str().c_str(), true);

}
int MyShapeokoTinyg::GetPositionSteps(long& x, long& y)
{
   int ret;
	if (!IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
    ret = GetStatus();
	if (ret != DEVICE_OK)
    return ret;
	x =  MPos[0]*1000 /GetStepSizeXUm();
	y =   MPos[1]*1000 /GetStepSizeXUm();
   ostringstream os;
   os << "GetPositionSteps(), X=" << x << ", Y=" << y;
   LogMessage(os.str().c_str(), true);
}
int MyShapeokoTinyg::SetPositionUm(double x, double y){
	if (!IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
	int errCode_ = DEVICE_ERR;
	if(lastMode_ != MOVE){
		std::string tmp("G90");
		errCode_ = SendCommand(tmp,tmp);
		lastMode_ = MOVE;
	}
	char buff[100];
	sprintf(buff, "G00X%fY%f", x/1000.0,y/1000.0);
	std::string buffAsStdStr = buff;
	errCode_ = SendCommand(buffAsStdStr,buffAsStdStr); //stage_->MoveBlocking(x_, y_);

	ostringstream os;
	os << "Move finished with error code: " << errCode_;
	LogMessage(os.str().c_str(), true);
	return errCode_;
}
int MyShapeokoTinyg::SetRelativePositionUm(double dx, double dy){
  if (!IsPortAvailable()) {
    return ERR_NO_PORT_SET;
  }
	int errCode_ = DEVICE_ERR;
	if(lastMode_ != MOVEREL){
		std::string tmp("G91");
		errCode_ = SendCommand(tmp,tmp);
		lastMode_ = MOVEREL;
	}
	char buff[100];
	sprintf(buff, "G00X%fY%f", dx/1000.0,dy/1000.0);
	std::string buffAsStdStr = buff;
    errCode_ = SendCommand(buffAsStdStr,buffAsStdStr);  // relative move

    ostringstream os;
    os << "Move finished with error code: " << errCode_;
    LogMessage(os.str().c_str(), true);
	return errCode_;
	
}


/**
 * Performs homing for both axes
 * (required after initialization)
 */
int MyShapeokoTinyg::Home()
{
  if (!IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
	int ret;
	ret = SetProperty("Command","$H");
	if (ret != DEVICE_OK)
	{
		LogMessage(std::string("Homing error!"));
		return ret;
	}
	else
		home_ = true; // successfully homed
   // check status
}

/**
 * Stops XY stage immediately. Blocks until done.
 */
int MyShapeokoTinyg::Stop()
{
   int ret;
   return DEVICE_OK;
}

/**
 * This is supposed to set the origin (0,0) at whatever is the current position.
 * Our stage does not support setting the origin (it is fixed). The base class
 * (XYStageBase) provides the default implementation SetAdapterOriginUm(double x, double y)
 * but we are not going to use since it would affect absolute coordinates during "Calibrate"
 * command in micro-manager.
 */
int MyShapeokoTinyg::SetOrigin()
{
   // commnted oout since we do not really want to support setting the origin
   // int ret = SetAdapterOriginUm(0.0, 0.0);
   return DEVICE_OK; 
}
 
int MyShapeokoTinyg::GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax)
{
   xMin = 0.0;
   yMin = 0.0;
   xMax = xAxisMaxSteps * stepSizeUm;
   yMax = yAxisMaxSteps * stepSizeUm;

   return DEVICE_OK;
}

int MyShapeokoTinyg::GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax)
{
   xMin = 0L;
   yMin = 0L;
   xMax = xAxisMaxSteps;
   yMax = yAxisMaxSteps;

   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////


/**
 * Gets and sets the maximum speed with which the stage travels
 */
int MyShapeokoTinyg::OnMaxVelocity(MM::PropertyBase* pProp, MM::ActionType eAct) 
{
   if (eAct == MM::BeforeGet) 
   {

   } 
   else if (eAct == MM::AfterSet) 
   {
   }

   return DEVICE_OK;
}

/**
 * Gets and sets the Acceleration of the stage travels
 */
int MyShapeokoTinyg::OnAcceleration(MM::PropertyBase* pProp, MM::ActionType eAct) 
{
   if (eAct == MM::BeforeGet) 
   {
	    pProp->Set(moveTimeoutMs_);
   } 
   else if (eAct == MM::AfterSet) 
   {
	   pProp->Get(moveTimeoutMs_);

   }

   return DEVICE_OK;
}

/**
 * Gets and sets the Acceleration of the stage travels
 */
int MyShapeokoTinyg::OnMoveTimeout(MM::PropertyBase* pProp, MM::ActionType eAct) 
{
   if (eAct == MM::BeforeGet) 
   {
      pProp->Set(moveTimeoutMs_);
   } 
   else if (eAct == MM::AfterSet) 
   {
      pProp->Get(moveTimeoutMs_);
   }

   return DEVICE_OK;
}
/**
 * Gets and sets the sync signal step
 */
int MyShapeokoTinyg::OnSyncStep(MM::PropertyBase* pProp, MM::ActionType eAct) 
{
  LogMessage("A");
   if (eAct == MM::BeforeGet) 
   {

     LogMessage("B");
      pProp->Set(syncStep_);
     LogMessage("C");

   } 
   else if (eAct == MM::AfterSet) 
   {   
     if (!IsPortAvailable()) {
		  return ERR_NO_PORT_SET;
	   }

      pProp->Get(syncStep_);

	   int ret = SetSync(0,syncStep_);
	   if(ret != DEVICE_OK)
		   return ret;
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// private methods
///////////////////////////////////////////////////////////////////////////////


/**
 * Sends move command to both axes and waits for responses, blocking the calling thread.
 * If expected answers do not come within timeout interval, returns with error.
 */
int MyShapeokoTinyg::MoveBlocking(long x, long y, bool relative)
{
   int ret;

   //if (!home_)
   //   return ERR_HOME_REQUIRED; 

   //// send command to X axis
   //ret = xstage_->MoveBlocking(x, relative);
   if (ret != DEVICE_OK)
     return ret;
   // send command to Y axis

   return ret;//ystage_->MoveBlocking(y, relative);
}
