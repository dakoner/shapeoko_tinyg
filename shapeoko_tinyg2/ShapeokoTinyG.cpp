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
#include "XYStage.h"
#include "ZStage.h"
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
const char* g_ZStageDeviceName = "DZStage";
const char* g_HubDeviceName = "DHub";
const char* g_versionProp = "Version";

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////

MODULE_API void InitializeModuleData()
{
  RegisterDevice(g_XYStageDeviceName, MM::XYStageDevice, "ShapeokoTinyG XY stage");
  RegisterDevice(g_ZStageDeviceName, MM::StageDevice, "ShapeokoTinyG Z stage");
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
  if (strcmp(deviceName, g_ZStageDeviceName) == 0)
  {
    // create stage
    return new CShapeokoTinyGZStage();
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

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}



ShapeokoTinyGHub::ShapeokoTinyGHub():
    initialized_(false),
    busy_(false)
{
  LogMessage("TinyG Constructor");
  CPropertyAction* pAct  = new CPropertyAction(this, &ShapeokoTinyGHub::OnPort);
  CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
}

ShapeokoTinyGHub::~ShapeokoTinyGHub() { Shutdown();}

int ShapeokoTinyGHub::Initialize()
{
  LogMessage("TinyG Initialize");
  /* From EVA's XYStage */
  int ret = DEVICE_ERR;

  // initialize device and get hardware information


  // confirm that the device is supported


  // check if we are already homed

  CPropertyAction* pAct;

  // Step size
  //CreateProperty(g_StepSizeXProp, CDeviceUtils::ConvertToString(stepSizeUm), MM::Float, true);
  // CreateProperty(g_StepSizeYProp, CDeviceUtils::ConvertToString(stepSizeUm), MM::Float, true);

  // Max Speed
  // pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnMaxVelocity);
  // CreateProperty(g_MaxVelocityProp, "100.0", MM::Float, false, pAct);
  //SetPropertyLimits(g_MaxVelocityProp, 0.0, 31999.0);

  // Acceleration
  // pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnAcceleration);
  // CreateProperty(g_AccelProp, "100.0", MM::Float, false, pAct);
  //SetPropertyLimits("Acceleration", 0.0, 150);

  // Move timeout
  // pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnMoveTimeout);
  // CreateProperty(g_MoveTimeoutProp, "10000.0", MM::Float, false, pAct);
  //SetPropertyLimits("Acceleration", 0.0, 150);

  // // Sync Step
  // pAct = new CPropertyAction (this, &MyShapeokoTinyg::OnSyncStep);
  // CreateProperty(g_SyncStepProp, "1.0", MM::Float, false, pAct);


  // ret = UpdateStatus();
  // if (ret != DEVICE_OK)
  //   return ret;

  /* From EVA_NDE_Grbl */

  MMThreadGuard myLock(lock_);

  pAct = new CPropertyAction(this, &ShapeokoTinyGHub::OnCommand);
  ret = CreateProperty("Command","", MM::String, false, pAct);
  if (DEVICE_OK != ret)
     return ret;

  // // turn off verbose serial debug messages
  GetCoreCallback()->SetDeviceProperty(port_.c_str(), "Verbose", "1");
  // synchronize all properties
  // --------------------------

  PurgeComPortH();
  std::string expected; 
  /*
  LogMessage(std::string("Sending reset!"));
  SetAnswerTimeoutMs(10000.0);
  const char* controlx = "";
  ret = WriteToComPortH((const unsigned char*) controlx, 1);
  if (ret != DEVICE_OK)
    return ret;
  string an;
  CDeviceUtils::SleepMs(600);
  ret = GetSerialAnswerComPortH(an,"\r");
  expected = "{\"r\":{\"fv\":0.970,\"fb\":438.02,\"hp\":1,\"hv\":8,\"id\":\"1H4973-ENT\",\"msg\":\"SYSTEM READY\"},\"f\":[1,0,0,1418]}\n";
  if (ret != DEVICE_OK) {
    return ret;
  }
  if (an != expected) {
    LogMessage("failed to get expected string, got:");
    LogMessage(an);
    return DEVICE_ERR;
  }

  PurgeComPortH();
  */
  std::string answer;
  ret = SendConfigCommand("$ee=0", answer);
  if (ret != DEVICE_OK)
    return ret;
  expected = "[ee]";
  if (answer.find(expected) == std::string::npos) {
    LogMessage("Got unexpected response to disable echo.");
    return DEVICE_ERR;
  }

  PurgeComPortH();

  ret = SendConfigCommand("$tv=0", answer);
  if (ret != DEVICE_OK)
    return ret;
  expected = "[tv]";
  if (answer.find(expected) == std::string::npos) {
    LogMessage("Got unexpected response to disable verbosity.");
    return DEVICE_ERR;
  }


  PurgeComPortH();
  ret = GetControllerVersion(version_);
  if( DEVICE_OK != ret)
    return ret;
  pAct = new CPropertyAction(this, &ShapeokoTinyGHub::OnVersion);
  std::ostringstream sversion;
  sversion << version_;
  CreateProperty(g_versionProp, sversion.str().c_str(), MM::String, true, pAct);

  PurgeComPortH();

  string command = "G90";
  LogMessage("Writing relative mode to com port");
  LogMessage(command);
  ret = SendCommandNoResponse(command);
  if (ret != DEVICE_OK)
    return ret;

  PurgeComPortH();

  ret = GetStatus();
  if (ret != DEVICE_OK)
    return ret;

  ret = UpdateStatus();
  if (ret != DEVICE_OK)
    return ret;

  initialized_ = true;
  return DEVICE_OK;
}

int ShapeokoTinyGHub::Shutdown() {initialized_ = false; return DEVICE_OK;};
bool ShapeokoTinyGHub::Busy() {   LogMessage("TinyG busy");
return busy_;} ;

// private and expects caller to:
// 1. guard the port
// 2. purge the port
int ShapeokoTinyGHub::GetControllerVersion(string& version)
{
  LogMessage("TinyG GetControllerVersion");
  int ret = DEVICE_OK;
  const char* command = "$fv";
  version = "";

  LogMessage("Writing to com port");
  LogMessage(command);
  std::string answer;
  ret = SendCommand(command, answer);
  if (ret != DEVICE_OK)
    return ret;
  LogMessage("Got answer:");
  LogMessage(answer.c_str());
  version = answer;
  return ret;

}
int ShapeokoTinyGHub::DetectInstalledDevices()
{
  LogMessage("TinyG DetectInstalledDevices");
  ClearInstalledDevices();

  // make sure this method is called before we look for available devices
  InitializeModuleData();

  char hubName[MM::MaxStrLength];
  GetName(hubName); // this device name
  for (unsigned i=0; i<GetNumberOfDevices(); i++)
  {
    char deviceName[MM::MaxStrLength];
    LogMessage("Get device");
    bool success = GetDeviceName(i, deviceName, MM::MaxStrLength);
    if (success && (strcmp(hubName, deviceName) != 0))
    {
      LogMessage("Got device");
      LogMessage(deviceName);
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

int ShapeokoTinyGHub::OnVersion(MM::PropertyBase* pProp, MM::ActionType pAct)
{
  LogMessage("TinyG OnVersion");
  if (pAct == MM::BeforeGet)
  {
    pProp->Set(version_.c_str());
  }
  return DEVICE_OK;
}
int ShapeokoTinyGHub::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
  LogMessage("TinyG OnPort");
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

int ShapeokoTinyGHub::OnCommand(MM::PropertyBase* pProp, MM::ActionType pAct)
{
  LogMessage("TinyG OnCommand");
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

int ShapeokoTinyGHub::SendCommand(std::string command, std::string &returnString)
{
  LogMessage("TinyG SendCommand");
  LogMessage("command=" + command);
  if(!portAvailable_)
    return ERR_NO_PORT_SET;
  // needs a lock because the other Thread will also use this function
  MMThreadGuard(this->executeLock_);
  PurgeComPortH();
  int ret = DEVICE_OK;
  SetAnswerTimeoutMs(300.0); //for normal command

  LogMessage("Write command.");
  ret = SetCommandComPortH(command.c_str(),"\r");
  LogMessage("set command, ret=" + ret);
  if (ret != DEVICE_OK)
  {
    LogMessage("command write fail");
    return ret;
  }
  std::string an;
  try
  {

    ret = GetSerialAnswerComPortH(an,"\r");
    if (ret != DEVICE_OK)
    {
      LogMessage(std::string("answer get error!_"));
      return ret;
    }
    LogMessage("answer:");
    LogMessage(an);
    returnString = an;
  }
  catch(...)
  {
    LogMessage("Exception in send command!");
    return DEVICE_ERR;
  }
  return DEVICE_OK;
}

int ShapeokoTinyGHub::SendMotionCommand(std::string command)
{
  LogMessage("TinyG SendMotionCommand");
  LogMessage("command=" + command);
  if(!portAvailable_)
    return ERR_NO_PORT_SET;
  // needs a lock because the other Thread will also use this function
  MMThreadGuard(this->executeLock_);
  PurgeComPortH();
  int ret = DEVICE_OK;
  SetAnswerTimeoutMs(300.0); //for normal command

  LogMessage("Write command.");
  ret = SetCommandComPortH(command.c_str(),"\r");
  LogMessage("set command, ret=" + ret);
  if (ret != DEVICE_OK)
  {
    LogMessage("command write fail");
    return ret;
  }
  bool done = false;
  while(!done) {
    std::string an;
    try
    {
      ret = GetSerialAnswerComPortH(an,"\r");
      if (ret != DEVICE_OK)
      {
        LogMessage(std::string("answer get error!_"));
        return ret;
      }
      LogMessage("answer:");
      LogMessage(an);
      std::vector<std::string> result = split(an, ',');
      for(std::vector<std::string>::iterator item = result.begin(); item != result.end(); ++item) {
        std::vector<std::string> p = split(*item, ':');
        if (p[0] == "stat" && p[1] == "3\n") {
          LogMessage("Move done.");
          done = true;
          break;
        }
      }
    }
    catch(...)
    {
      LogMessage("Exception in send command!");
      return DEVICE_ERR;
    }
  }
  return DEVICE_OK;
}

int ShapeokoTinyGHub::SendCommandNoResponse(std::string command)
{
  LogMessage("TinyG SendCommand");
  LogMessage("command=" + command);
  if(!portAvailable_)
    return ERR_NO_PORT_SET;
  // needs a lock because the other Thread will also use this function
  MMThreadGuard(this->executeLock_);
  PurgeComPortH();
  int ret = DEVICE_OK;
  SetAnswerTimeoutMs(300.0); //for normal command

  LogMessage("Write command.");
  ret = SetCommandComPortH(command.c_str(),"\r");
  LogMessage("set command, ret=" + ret);
  if (ret != DEVICE_OK)
  {
    LogMessage("command write fail");
    return ret;
  }
  return DEVICE_OK;
}


int ShapeokoTinyGHub::SendConfigCommand(string command, string& answer)
{
  LogMessage("TinyG SendConfigCommand");
  if(!portAvailable_)
    return ERR_NO_PORT_SET;
  // needs a lock because the other Thread will also use this function
  MMThreadGuard(this->executeLock_);
  PurgeComPortH();
  int ret = DEVICE_OK;
  SetAnswerTimeoutMs(10000.0); //for normal command

  LogMessage("Writing command to com port");
  LogMessage(command);
  ret = SetCommandComPortH(command.c_str(),"\r");
  LogMessage("set command, ret=" + ret);
  if (ret != DEVICE_OK)
  {
    LogMessage("command write fail");
    return ret;
  }
  try
  {

    LogMessage("Reading answer.");
    ret = GetSerialAnswerComPortH(answer,"\r");
    if (ret != DEVICE_OK)
    {
      LogMessage(std::string("answer get error!_"));
      return ret;
    }
    LogMessage("answer:");
    LogMessage(std::string(answer));
    if (answer.length() <1)
      return DEVICE_ERR;
  }
  catch(...)
  {
    LogMessage("Exception in send command!");
    return DEVICE_ERR;
  }
  return DEVICE_OK;
}

MM::DeviceDetectionStatus ShapeokoTinyGHub::DetectDevice(void)
{
  LogMessage("TinyG DetectDevice");
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
      GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate, "115200" );
      GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
      // Arduino timed out in GetControllerVersion even if AnswerTimeout  = 300 ms
      GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "500.0");
      GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs", "0");
      MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str());
      pS->Initialize();
      // The first second or so after opening the serial port, the Arduino is waiting for firmwareupgrades.  Simply sleep 1 second.
      CDeviceUtils::SleepMs(2000);
      MMThreadGuard myLock(executeLock_);
      PurgeComPort(port_.c_str());
      int ret = GetStatus();
      // later, Initialize will explicitly check the version #
      if( DEVICE_OK != ret )
      {
        LogMessage("Got:");
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

int ShapeokoTinyGHub::SetAnswerTimeoutMs(double timeout)
{
  LogMessage("TinyG SetAnswerTimeoutMs");
  if(!portAvailable_)
    return ERR_NO_PORT_SET;
  GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout",  CDeviceUtils::ConvertToString(timeout));
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

// private and expects caller to:
// 1. guard the port
// 2. purge the port
int ShapeokoTinyGHub::GetStatus()
{
  LogMessage("TinyG GetStatus");
  std::string cmd;
  cmd.assign("$sr"); // x step/mm
  std::string returnString;

  if(!portAvailable_)
    return ERR_NO_PORT_SET;
  // needs a lock because the other Thread will also use this function
  MMThreadGuard(this->executeLock_);
  PurgeComPortH();
  int ret = DEVICE_OK;
  SetAnswerTimeoutMs(1000.0); //for normal command

  LogMessage("Write command.");
  ret = SetCommandComPortH(cmd.c_str(),"\r");
  LogMessage("set command, ret=" + ret);
  if (ret != DEVICE_OK)
  {
    LogMessage("command write fail");
    return ret;
  }


  while(true) {
    try
    {
      string an;
      ret = GetSerialAnswerComPortH(an,"\r");
      if (ret != DEVICE_OK)
      {
        LogMessage(std::string("answer get error!_"));
        return ret;
      }
      LogMessage("answer:");
      LogMessage(an);
      if (an.length() <1)
        return DEVICE_ERR;
      returnString += an;
      if (an.find("Machine state:") != std::string::npos) {
        break;
      }
    }
    catch(...)
    {
      LogMessage("Exception in send command!");
      return DEVICE_ERR;
    }
  }
  LogMessage("ReturnString:");
  LogMessage(returnString);

  std::vector<std::string> tokenInput;
  //      char* pEnd;
  CDeviceUtils::Tokenize(returnString, tokenInput, "\r\n");
  for(std::vector<std::string>::iterator i = tokenInput.begin(); i != tokenInput.end(); ++i) {
    LogMessage("Token input: ");
    LogMessage(*i);
    string x;
    if (i->substr(0, 10) == "X position") {
      x = i->substr(21,10);
      std::vector<std::string> spl;
      spl = split(x, ' ');
      MPos[0] = stringToNum<double>(spl[0]);
    }
    if (i->substr(0, 10) == "Y position") {
      x = i->substr(21,10);
      std::vector<std::string> spl;
      spl = split(x, ' ');
      MPos[1] = stringToNum<double>(spl[0]);
    }
    if (i->substr(0, 10) == "Z position") {
      x = i->substr(21,10);
      std::vector<std::string> spl;
      spl = split(x, ' ');
      MPos[2] = stringToNum<double>(spl[0]);
    }
    if (i->substr(0, 9) == "Velocity:") {
      x = i->substr(21,10);
    }
    if (i->substr(0, 6) == "Units:") {
      // TODO(dek): correct these if wrong.
      x = i->substr(21,10);
    }
    if (i->substr(0, 18) == "Coordinate system:") {
      x = i->substr(21,10);
    }
    if (i->substr(0, 14) == "Distance mode:") {
      // TODO(dek): correct these if wrong.
      x = i->substr(21,10);
    }
    if (i->substr(0, 14) == "Machine state:") {
      x = i->substr(21,10);
      SetProperty("Status",x.c_str());
    }
    if (!x.empty()) {
      LogMessage("Parsed line:");
      LogMessage(x);
    }

  }
  return DEVICE_OK;
}

int ShapeokoTinyGHub::ReadFromComPortH(unsigned char* answer, unsigned maxLen, unsigned long& bytesRead)
{
  LogMessage("TinyG ReadFromComPortH");
  return ReadFromComPort(port_.c_str(), answer, maxLen, bytesRead);
}
int ShapeokoTinyGHub::SetCommandComPortH(const char* command, const char* term)
{
  LogMessage("TinyG SetCommandComPortH");
  return SendSerialCommand(port_.c_str(),command,term);
}
int ShapeokoTinyGHub::GetSerialAnswerComPortH (std::string& ans,  const char* term)
{
  LogMessage("TinyG GetSerialAnswerComPortH");
  return GetSerialAnswer(port_.c_str(),term,ans);
}

int ShapeokoTinyGHub::PurgeComPortH() {  LogMessage("TinyG PurgeComPortH");
return PurgeComPort(port_.c_str());}
int ShapeokoTinyGHub::WriteToComPortH(const unsigned char* command, unsigned len) {LogMessage("TinyG WriteToComPortH"); return WriteToComPort(port_.c_str(), command, len);}
