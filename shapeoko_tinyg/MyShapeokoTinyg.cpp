#include "MyShapeokoTinyg.h"
#include "ModuleInterface.h"


using namespace std;

const char* g_ShapeokoTinygDeviceName= "ShapeokoTinyG";


///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
/**
 * List all supported hardware devices here
 */
MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_StageName, MM::XYStageDevice, "ShapeokoTinyG");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   // decide which device class to create based on the deviceName parameter
   if (strcmp(deviceName, g_StageName) == 0)
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


MyShapeokoTinyg::MyShapeokoTinyg() :
   CXYStageBase<MyShapeokoTinyg>(),
  initialized_(false)
   /*
   home_(false),
   answerTimeoutMs_(1000.0),
   moveTimeoutMs_(10000.0),

   cmdThread_(0) 
   */
{
   // set default error messages
   InitializeDefaultErrorMessages();


   // create pre-initialization properties
   // ------------------------------------

   // Name
   CreateProperty(MM::g_Keyword_Name, g_ShapeokoTinygDeviceName, MM::String, true);

   // Description
   CreateProperty(MM::g_Keyword_Description, "XY stage adapter for EvaGrbl -- by Wei Ouyang", MM::String, true);


   cmdThread_ = new CommandThread(this);
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
   CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
   if (!hub || !hub->IsPortAvailable()) {
      return ERR_NO_PORT_SET;
   }
   char hubLabel[MM::MaxStrLength];
   hub->GetLabel(hubLabel);
   SetParentID(hubLabel); // for backward comp.


   parameters_ = &hub->parameters;

   int ret = DEVICE_ERR;

   // initialize device and get hardware information


   // confirm that the device is supported


   // check if we are already homed

  

   // Step size
   CreateProperty(g_StepSizeXProp, CDeviceUtils::ConvertToString(stepSizeUm), MM::Float, true);
   CreateProperty(g_StepSizeYProp, CDeviceUtils::ConvertToString(stepSizeUm), MM::Float, true);

   // Max Speed
   CPropertyAction* pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnMaxVelocity);
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


   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

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

bool MyShapeokoTinyg::Busy()
{
	return false;
}
 
double MyShapeokoTinyg::GetStepSizeXUm()
{
	CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
   return 1000.0/(*parameters_)[0];
}

double MyShapeokoTinyg::GetStepSizeYUm()
{
	CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
   return 1000.0/(*parameters_)[1];
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
   	CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
    ret = hub->GetStatus();
	if (ret != DEVICE_OK)
    return ret;
	x =  hub->MPos[0]*1000.0 ;
	y =   hub->MPos[1]*1000.0;
   ostringstream os;
   os << "GetPositionSteps(), X=" << x << ", Y=" << y;
   LogMessage(os.str().c_str(), true);
}
int MyShapeokoTinyg::GetPositionSteps(long& x, long& y)
{
   int ret;
   	CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
    ret = hub->GetStatus();
	if (ret != DEVICE_OK)
    return ret;
	x =  hub->MPos[0]*1000 /GetStepSizeXUm();
	y =   hub->MPos[1]*1000 /GetStepSizeXUm();
   ostringstream os;
   os << "GetPositionSteps(), X=" << x << ", Y=" << y;
   LogMessage(os.str().c_str(), true);
   return DEVICE_OK;
}
int MyShapeokoTinyg::SetPositionUm(double x, double y){
	 CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
	int errCode_ = DEVICE_ERR;
	if(lastMode_ != MOVE){
		std::string tmp("G90");
		errCode_ = hub->SendCommand(tmp,tmp);
		lastMode_ = MOVE;
	}
	char buff[100];
	sprintf(buff, "G00X%fY%f", x/1000.0,y/1000.0);
	std::string buffAsStdStr = buff;
	errCode_ = hub->SendCommand(buffAsStdStr,buffAsStdStr); //stage_->MoveBlocking(x_, y_);

	ostringstream os;
	os << "Move finished with error code: " << errCode_;
	LogMessage(os.str().c_str(), true);
	return errCode_;
}
int MyShapeokoTinyg::SetRelativePositionUm(double dx, double dy){
	 CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
	int errCode_ = DEVICE_ERR;
	if(lastMode_ != MOVEREL){
		std::string tmp("G91");
		errCode_ = hub->SendCommand(tmp,tmp);
		lastMode_ = MOVEREL;
	}
	char buff[100];
	sprintf(buff, "G00X%fY%f", dx/1000.0,dy/1000.0);
	std::string buffAsStdStr = buff;
    errCode_ = hub->SendCommand(buffAsStdStr,buffAsStdStr);  // relative move

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
   	CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	if (!hub || !hub->IsPortAvailable()) {
		return ERR_NO_PORT_SET;
	}
	int ret;
	ret = hub->SetProperty("Command","$H");
	if (ret != DEVICE_OK)
	{
		LogMessage(std::string("Homing error!"));
		return ret;
	}
	else
		home_ = true; // successfully homed
   // check status
   return DEVICE_OK;
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
   if (eAct == MM::BeforeGet) 
   {

      pProp->Set(syncStep_);

   } 
   else if (eAct == MM::AfterSet) 
   {   
	   CEVA_NDE_GrblHub* hub = static_cast<CEVA_NDE_GrblHub*>(GetParentHub());
	   if (!hub || !hub->IsPortAvailable()) {
		  return ERR_NO_PORT_SET;
	   }

      pProp->Get(syncStep_);

	   int ret = hub->SetSync(0,syncStep_);
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
