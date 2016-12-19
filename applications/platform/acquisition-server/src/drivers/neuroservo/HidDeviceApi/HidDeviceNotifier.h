/*
* HID driver for OpenViBE
*
* \authors (NeuroServo, NeuroTechX)
* \developer (Innocent Aguié)
* \date Wed Nov 23 00:24:00 2016
*
*/

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#pragma once

#include <Windows.h>
#include <functional>

class HidDeviceNotifier
{
public:
	HidDeviceNotifier(GUID interfaceGuid, CHAR* devicePathName);
	HidDeviceNotifier();
	~HidDeviceNotifier();
	
	void startRegistration();

	/* Callbacks methods */
	std::function<void(void)> deviceDetached;
	std::function<void(void)> deviceAttached;

	/* Member functions */
	GUID getDeviceGuid(){ return _deviceGuid; };
	CHAR* getDevicePathName(){ return _devicePathName; };
	LPCSTR getAppName(){ return _appName; };
	bool isDeviceRegistrationStarted(){ return _isDeviceRegistered; };


private:
	/* Members */
	// Device related information
	GUID _deviceGuid;
	CHAR* _devicePathName;
	
	// For informational messages and window titles
	LPCSTR _appName;

	bool _isDeviceRegistered;
};

#endif
#endif
