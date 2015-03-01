///////////////////////////////////////////////////////////////////////////////
// FILE:          ShapeokoTinyGCamera.cpp
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
// COPYRIGHT:     University of California, San Francisco, 2006
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

#include "ShapeokoTinyG.h"
#include <cstdio>
#include <string>
#include <math.h>
#include "ModuleInterface.h"
#include <sstream>
#include <algorithm>
#include <iostream>



using namespace std;

// External names used used by the rest of the system
// to load particular device from the "ShapeokoTinyGCamera.dll" library
const char* g_XYStageDeviceName = "DXYStage";
const char* g_HubDeviceName = "DHub";

// constants for naming pixel types (allowed values of the "PixelType" property)
const char* g_PixelType_8bit = "8bit";
const char* g_PixelType_16bit = "16bit";
const char* g_PixelType_32bitRGB = "32bitRGB";
const char* g_PixelType_64bitRGB = "64bitRGB";
const char* g_PixelType_32bit = "32bit";  // floating point greyscale

// constants for naming camera modes
const char* g_Sine_Wave = "Artificial Waves";
const char* g_Norm_Noise = "Noise";

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////

MODULE_API void InitializeModuleData()
{
   RegisterDevice(g_XYStageDeviceName, MM::XYStageDevice, "ShapeokoTinyG XY stage");
   RegisterDevice(g_HubDeviceName, MM::HubDevice, "DHub");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   if (strcmp(deviceName, g_XYStageDeviceName) == 0)
   {
      // create stage
      return new CShapeokoTinyGXYStage();
   }
   else if (strcmp(deviceName, g_HubDeviceName) == 0)
   {
	  return new ShapeokoTinyGHub();
   }

   // ...supplied name not recognized
   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////
// CShapeokoTinyGXYStage implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~

CShapeokoTinyGXYStage::CShapeokoTinyGXYStage() : 
CXYStageBase<CShapeokoTinyGXYStage>(),
stepSize_um_(0.015),
posX_um_(0.0),
posY_um_(0.0),
busy_(false),
timeOutTimer_(0),
velocity_(10.0), // in micron per second
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

void CShapeokoTinyGXYStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_XYStageDeviceName);
}

int CShapeokoTinyGXYStage::Initialize()
{
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
   if (timeOutTimer_ == 0)
      return false;
   if (timeOutTimer_->expired(GetCurrentMMTime()))
   {
      // delete(timeOutTimer_);
      return false;
   }
   return true;
}

int CShapeokoTinyGXYStage::SetPositionSteps(long x, long y)
{
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
   int ret = OnXYStagePositionChanged(posX_um_, posY_um_);
   if (ret != DEVICE_OK)
      return ret;

   return DEVICE_OK;
}

int CShapeokoTinyGXYStage::GetPositionSteps(long& x, long& y)
{
   x = (long)(posX_um_ / stepSize_um_);
   y = (long)(posY_um_ / stepSize_um_);
   return DEVICE_OK;
}

int CShapeokoTinyGXYStage::SetRelativePositionSteps(long x, long y)
{
   long xSteps, ySteps;
   GetPositionSteps(xSteps, ySteps);

   return this->SetPositionSteps(xSteps+x, ySteps+y);
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
// none implemented


int ShapeokoTinyGHub::Initialize()
{
  	initialized_ = true;
 
	return DEVICE_OK;
}

int ShapeokoTinyGHub::DetectInstalledDevices()
{  
   ClearInstalledDevices();

   // make sure this method is called before we look for available devices
   InitializeModuleData();

   char hubName[MM::MaxStrLength];
   GetName(hubName); // this device name
   for (unsigned i=0; i<GetNumberOfDevices(); i++)
   { 
      char deviceName[MM::MaxStrLength];
      bool success = GetDeviceName(i, deviceName, MM::MaxStrLength);
      if (success && (strcmp(hubName, deviceName) != 0))
      {
         MM::Device* pDev = CreateDevice(deviceName);
         AddInstalledDevice(pDev);
      }
   }
   return DEVICE_OK; 
}

void ShapeokoTinyGHub::GetName(char* pName) const
{
   CDeviceUtils::CopyLimitedString(pName, g_HubDeviceName);
}
