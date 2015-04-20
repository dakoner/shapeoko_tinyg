#include "ShapeokoTinyG.h"
#include "XYStage.h"
const char* g_StepSizeProp = "Step Size";
const char* g_MaxVelocityProp = "Maximum Velocity";
const char* g_AccelProp = "Acceleration";

///////////////////////////////////////////////////////////////////////////////
// CShapeokoTinyGXYStage implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~

CShapeokoTinyGXYStage::CShapeokoTinyGXYStage() :
    CXYStageBase<CShapeokoTinyGXYStage>(),
    stepSize_um_(0.025),
    max_velocity_(1000),
    acceleration_(100),
    posX_um_(0.0),
    posY_um_(0.0),
    busy_(false),
    timeOutTimer_(0),
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

  CPropertyAction* pAct = new CPropertyAction (this, &CShapeokoTinyGXYStage::OnStepSize);
  CreateProperty(g_StepSizeProp, CDeviceUtils::ConvertToString(stepSize_um_), MM::Float, false, pAct);

  // Max Speed
  pAct = new CPropertyAction (this, &CShapeokoTinyGXYStage::OnMaxVelocity);
  CreateProperty(g_MaxVelocityProp, CDeviceUtils::ConvertToString(max_velocity_), MM::Float, false, pAct);
  SetPropertyLimits(g_MaxVelocityProp, 0.0, 1000.0);

  // Acceleration
  pAct = new CPropertyAction (this, &CShapeokoTinyGXYStage::OnAcceleration);
  CreateProperty(g_AccelProp, CDeviceUtils::ConvertToString(acceleration_), MM::Float, false, pAct);
  SetPropertyLimits("Acceleration", 0.0, 1000);




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

double CShapeokoTinyGXYStage::GetStepSize() {return stepSize_um_;}

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

  // TODO(dek): if no position change, don't send new position.
  char buff[100];
  sprintf(buff, "G0 X%f Y%f", posX_um_/1000., posY_um_/1000.);
  std::string buffAsStdStr = buff;
  ShapeokoTinyGHub* pHub = static_cast<ShapeokoTinyGHub*>(GetParentHub());
  int ret = pHub->SendMotionCommand(buffAsStdStr);
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

int CShapeokoTinyGXYStage::Home() {   LogMessage("TinyG XYStage home.");
return DEVICE_OK; }
int CShapeokoTinyGXYStage::Stop() { LogMessage("TinyG XYStage stop.");
return DEVICE_OK; }

int CShapeokoTinyGXYStage::SetOrigin() { LogMessage("TinyG XYStage set origin."); return DEVICE_OK; }

int CShapeokoTinyGXYStage::GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax)
{
  LogMessage("TinyG XYStage get limits um");
  xMin = lowerLimit_; xMax = upperLimit_;
  yMin = lowerLimit_; yMax = upperLimit_;
  return DEVICE_OK;
}

int CShapeokoTinyGXYStage::GetStepLimits(long& /*xMin*/, long& /*xMax*/, long& /*yMin*/, long& /*yMax*/)
{   LogMessage("TinyG XYStage get limits um");
return DEVICE_UNSUPPORTED_COMMAND; }

double CShapeokoTinyGXYStage::GetStepSizeXUm() {   LogMessage("TinyG XYStage get step size x um");
return stepSize_um_; }
double CShapeokoTinyGXYStage::GetStepSizeYUm() {   LogMessage("TinyG XYStage get step size y um");
return stepSize_um_; }
int CShapeokoTinyGXYStage::Move(double /*vx*/, double /*vy*/) {LogMessage("TinyG XYStage move"); return DEVICE_OK;}

int CShapeokoTinyGXYStage::IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}

int CShapeokoTinyGXYStage::OnStepSize(MM::PropertyBase* pProp, MM::ActionType eAct)
{
LogMessage("TinyG XYStage OnStepSize");
  if (eAct == MM::BeforeGet)
  {
        LogMessage("TinyG XYStage OnStepSizex");

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

// TODO(dek): these should send actual commands to update the device
int CShapeokoTinyGXYStage::OnMaxVelocity(MM::PropertyBase* pProp, MM::ActionType eAct)
{
LogMessage("TinyG XYStage OnmaxVelocity");
  if (eAct == MM::BeforeGet)
  {
        

    pProp->Set(max_velocity_);
  }
  else if (eAct == MM::AfterSet)
  {
    if (initialized_)
    {
      double max_velocity;
      pProp->Get(max_velocity);
      max_velocity_ = max_velocity;
    }

  }

   
  LogMessage("Set velocity");

  return DEVICE_OK;
}


int CShapeokoTinyGXYStage::OnAcceleration(MM::PropertyBase* pProp, MM::ActionType eAct)
{
LogMessage("TinyG XYStage OnAcceleration");
  if (eAct == MM::BeforeGet)
  {
        

    pProp->Set(acceleration_);
  }
  else if (eAct == MM::AfterSet)
  {
    if (initialized_)
    {
      double acceleration;
      pProp->Get(acceleration);
      acceleration_ = acceleration;
    }

  }

   
  LogMessage("Set acceleration");

  return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
// none implemented
