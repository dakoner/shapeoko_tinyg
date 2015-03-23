#include "ShapeokoTinyG.h"
#include "XYStage.h"
const char* g_StepSizeProp = "Step Size";

///////////////////////////////////////////////////////////////////////////////
// CShapeokoTinyGXYStage implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~

CShapeokoTinyGXYStage::CShapeokoTinyGXYStage() :
CXYStageBase<CShapeokoTinyGXYStage>(),
stepSize_um_(0.025),
posX_um_(0.0),
posY_um_(0.0),
busy_(false),
timeOutTimer_(0),
velocity_(100000.0), // in micron per second
initialized_(false),
lowerLimit_(0.0),
upperLimit_(20000.0)
{
   InitializeDefaultErrorMessages();

   // parent ID display
   CreateHubIDProperty();
}

CShapeokoTinyGXYStage::~CShapeokoTinyGXYStage()
{
   Shutdown();
}

extern const char* g_XYStageDeviceName;
const char* NoHubError = "Parent Hub not defined.";



void CShapeokoTinyGXYStage::GetName(char* Name) const
{
  LogMessage("XYStage: GetName");
   CDeviceUtils::CopyLimitedString(Name, g_XYStageDeviceName);
}

int CShapeokoTinyGXYStage::Initialize()
{
  LogMessage("XYStage: initialize");
   ShapeokoTinyGHub* pHub = static_cast<ShapeokoTinyGHub*>(GetParentHub());
   if (pHub)
   {
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateStringProperty(MM::g_Keyword_Name, g_XYStageDeviceName, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateStringProperty(MM::g_Keyword_Description, "ShapeokoTinyG XY stage driver", true);
   if (DEVICE_OK != ret)
      return ret;

   CPropertyAction* pAct1 = new CPropertyAction (this, &CShapeokoTinyGXYStage::OnStepSize);
   CreateProperty(g_StepSizeProp, CDeviceUtils::ConvertToString(stepSize_um_), MM::Float, false);

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

int CShapeokoTinyGXYStage::Shutdown()
{
   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

bool CShapeokoTinyGXYStage::Busy()
{
  LogMessage("XYStage: Busy called");
   if (timeOutTimer_ == 0)
      return false;
   if (timeOutTimer_->expired(GetCurrentMMTime()))
   {
      // delete(timeOutTimer_);
     LogMessage("XYStage: Busy return false");
      return false;
   }
     LogMessage("XYStage: Busy return true");
   return true;
}

int CShapeokoTinyGXYStage::SetPositionSteps(long x, long y)
{
  LogMessage("XYStage: SetPositionSteps");
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
   // long timeOut = (long) (distance / velocity_);
   long timeOut = 1000;
   timeOutTimer_ = new MM::TimeoutMs(GetCurrentMMTime(),  timeOut);
   posX_um_ = x * stepSize_um_;
   posY_um_ = y * stepSize_um_;

   char buff[100];
   sprintf(buff, "G0 X%f Y%f", posX_um_/1000., posY_um_/1000.);
   std::string buffAsStdStr = buff;
   ShapeokoTinyGHub* pHub = static_cast<ShapeokoTinyGHub*>(GetParentHub());
   int ret = pHub->SendCommand(buffAsStdStr,buffAsStdStr);
   if (ret != DEVICE_OK)
      return ret;

   ret = OnXYStagePositionChanged(posX_um_, posY_um_);
   if (ret != DEVICE_OK)
      return ret;

   return DEVICE_OK;
}

int CShapeokoTinyGXYStage::GetPositionSteps(long& x, long& y)
{
  LogMessage("XYStage: GetPositionSteps");
   x = (long)(posX_um_ / stepSize_um_);
   y = (long)(posY_um_ / stepSize_um_);
   return DEVICE_OK;
}

int CShapeokoTinyGXYStage::SetRelativePositionSteps(long x, long y)
{
  LogMessage("XYStage: SetRelativePositioNSteps");
   long xSteps, ySteps;
   GetPositionSteps(xSteps, ySteps);

   return this->SetPositionSteps(xSteps+x, ySteps+y);
}
int CShapeokoTinyGXYStage::OnStepSize(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        

        pProp->Set(stepSize_um_);
    }
    else if (eAct == MM::AfterSet)
    {
        if (initialized_)
        {
            double stepSize_um;
            pProp->Get(stepSize_um);
            stepSize_um_ = stepSize_um;
        }

    }

   
    LogMessage("Set step size");

    return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
// none implemented
