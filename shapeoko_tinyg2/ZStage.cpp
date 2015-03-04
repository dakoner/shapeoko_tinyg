///////////////////////////////////////////////////////////////////////////////
// FILE:       ZStage.cpp
// PROJECT:    MicroManage
// SUBSYSTEM:  DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:
// Zeiss CAN bus controller for Axioscop 2 MOT, Z-stage
//


#ifdef WIN32
   #include <windows.h>
   #define snprintf _snprintf
#endif

#include "ZStage.h"
using namespace std;

extern ZeissHub g_hub;
extern const char* g_ZStage;
extern const char* g_Keyword_LoadSample;

ZStage::ZStage() :
// http://www.shapeoko.com/wiki/index.php/Zaxis_ACME
stepSize_um_ (5.),
initialized_ (false)
{
   InitializeDefaultErrorMessages();

   SetErrorText(ERR_SCOPE_NOT_ACTIVE, "Zeiss Scope is not initialized.  It is needed for the Focus drive to work");
   SetErrorText(ERR_NO_FOCUS_DRIVE, "No focus drive found in this microscopes");
}

ZStage::~ZStage()
{
   Shutdown();
}

void ZStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_ZStage);
}

int ZStage::Initialize()
{
   if (!g_hub.initialized_)
      return ERR_SCOPE_NOT_ACTIVE;

   // set property list
   // ----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_ZStage, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Z-drive", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Position
   CPropertyAction* pAct = new CPropertyAction(this, &ZStage::OnPosition);
   ret = CreateProperty(MM::g_Keyword_Position, "0", MM::Float,false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Update lower and upper limits.  These values are cached, so if they change during a session, the adapter will need to be re-initialized
   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

int ZStage::Shutdown()
{
   initialized_ = false;

   return DEVICE_OK;
}

bool ZStage::Busy()
{
   return false;
}

int ZStage::SetPositionUm(double pos)
{
   long steps = (long)(pos / stepSize_um_ + 0.5);
   int ret = SetPositionSteps(steps);
   if (ret != DEVICE_OK)
      return ret;

   return DEVICE_OK;
}

int ZStage::GetPositionUm(double& pos)
{
   long steps;
   int ret = GetPositionSteps(steps);
   if (ret != DEVICE_OK)
      return ret;
   pos = steps * stepSize_um_;

   return DEVICE_OK;
}

/*
 * Requests movement to new z postion from the controller.  This function does the actual communication
 */
int ZStage::SetPositionSteps(long steps)
{
  LogMessage("ZStage: SetPositionSteps");
   if (timeOutTimer_ != 0)
   {
      if (!timeOutTimer_->expired(GetCurrentMMTime()))
         return ERR_STAGE_MOVING;
      delete (timeOutTimer_);
   }
   double newPosX = x * stepSize_um_;
   double newPosY = y * stepSize_um_;
   double difX = newPosX - posX_um_;
   double difY = newPosY - posY_um_;
   double distance = sqrt( (difX * difX) + (difY * difY) );
   long timeOut = (long) (distance / velocity_);
   timeOutTimer_ = new MM::TimeoutMs(GetCurrentMMTime(),  timeOut);
   posX_um_ = x * stepSize_um_;
   posY_um_ = y * stepSize_um_;

   char buff[100];
   sprintf(buff, "G0 Z%f", posX_um_/1000.);
   std::string buffAsStdStr = buff;
   ShapeokoTinyGHub* pHub = static_cast<ShapeokoTinyGHub*>(GetParentHub());
   int ret = pHub->SendCommand(buffAsStdStr,buffAsStdStr);
   if (ret != DEVICE_OK)
      return ret;

   ret = OnZStagePositionChanged(posZ_um_);
   if (ret != DEVICE_OK)
      return ret;

   // // the hard part is to get the formatting of the string right.
   // // it is a hex number from 800000 .. 7FFFFF, where everything larger than 800000 is a negative number!?
   // // We can speed up communication by skipping leading 0s, but that makes the following more complicated:
   // char tmp[98];
   // // convert the steps into a twos-complement 6bit number
   // if (steps<0)
   //    steps = steps+0xffffff+1;
   // snprintf(tmp, 9, "%08lX", steps);
   // string tmp2 = tmp;
   // ostringstream cmd;
   // cmd << "HPZT" << tmp2.substr(2,6).c_str();
   // int ret = g_hub.ExecuteCommand(*this, *GetCoreCallback(),  cmd.str().c_str());
   // if (ret != DEVICE_OK)
   //    return ret;

   // CDeviceUtils::SleepMs(100);
   return DEVICE_OK;
}

/*
 * Requests current z postion from the controller.  This function does the actual communication
 */
int ZStage::GetPositionSteps(long& steps)
{
   // const char* cmd ="HPZp" ;
   // int ret = g_hub.ExecuteCommand(*this, *GetCoreCallback(),  cmd);
   // if (ret != DEVICE_OK)
   //    return ret;

   // string response;
   // ret = g_hub.GetAnswer(*this, *GetCoreCallback(), response);
   // if (ret != DEVICE_OK)
   //    return ret;

   // if (response.substr(0,2) == "PH")
   // {
   //    steps = strtol(response.substr(2).c_str(), NULL, 16);
   // }
   // else
   //    return ERR_UNEXPECTED_ANSWER;

   // // To 'translate' 'negative' numbers according to the Zeiss schema (there must be a more elegant way of doing this:
   // long sign = strtol(response.substr(2,1).c_str(), NULL, 16);
   // if (sign > 7)  // negative numbers
   // {
   //    steps = steps - 0xFFFFFF - 1;
   // }

  // TODO(dek): implement status to get Z position

   return DEVICE_OK;
}

int ZStage::SetOrigin()
{
   // const char* cmd ="HPZP0" ;
   // int ret = g_hub.ExecuteCommand(*this, *GetCoreCallback(),  cmd);
   // if (ret != DEVICE_OK)
   //    return ret;

  // TODO(dek): run gcode to set origin to current location (G28.3 ?)

   return DEVICE_OK;
}

// TODO(dek): implement GetUpperLimit and GetLowerLimit

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
/*
 * Uses the Get and Set PositionUm functions to communicate with controller
 */
int ZStage::OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      double pos;
      int ret = GetPositionUm(pos);
      if (ret != DEVICE_OK)
         return ret;
      pProp->Set(pos);
   }
   else if (eAct == MM::AfterSet)
   {
      double pos;
      pProp->Get(pos);
      int ret = SetPositionUm(pos);
      if (ret != DEVICE_OK)
         return ret;
   }

   return DEVICE_OK;
}


// TODO(dek): implement OnStageLoad
