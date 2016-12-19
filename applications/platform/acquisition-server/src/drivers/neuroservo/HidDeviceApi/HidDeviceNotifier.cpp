/*
* HID driver for OpenViBE
*
* \authors (NeuroServo, NeuroTechX)
* \developer (Innocent Aguié)
* \date Wed Nov 23 00:24:00 2016
*
*/

#if defined TARGET_OS_Windows

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <dbt.h>

#include <hidsdi.h>
#include "HidDeviceNotifier.h"
#include "HidDeviceNotifierRef.h"
#include <thread>

#define WND_CLASS_NAME TEXT("SampleAppWindowClass")


HidDeviceNotifier::HidDeviceNotifier(GUID interfaceGuid, CHAR* devicePathName)
{
	_deviceGuid = interfaceGuid;
	_devicePathName = devicePathName;
	_appName = "Window Notifier";
	_isDeviceRegistered = false;
}

/* Default constructeur */
HidDeviceNotifier::HidDeviceNotifier()
{
}


HidDeviceNotifier::~HidDeviceNotifier()
{
}

void HidDeviceNotifier::startRegistration()
{
	_isDeviceRegistered = true;

	// Start a separate thread for reading data from the device
	std::thread notifierThread(registerNotification, this);
	notifierThread.detach();
}
#endif // TARGET_OS_Windows