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
#include "Hidsdi.h"
#include "SetupApi.h"
#include <thread>

#include "HidDeviceNotifier.h"


class HidDevice
{
public:
	HidDevice(unsigned short vendorID, unsigned short productID, unsigned short receivdedDataSize);
	HidDevice();
	~HidDevice();

public:
	/* Client available methods */
	bool connect();
	bool writeToDevice(BYTE data[], int nbOfBytes);
	bool isDeviceConnected(){ return _isDeviceConnected; }
	void setHidDeviceInfos(unsigned short vendorID, unsigned short productID, unsigned short receivdedDataSize);

	/* Callbacks methods */
	std::function<void(BYTE[])> dataReceived;
	std::function<void(void)> deviceConnected;
	std::function<void(void)> deviceDetached;
	std::function<void(void)> deviceAttached;
	

private:

	/* Methods to perform specific actions */
	bool configure();
	bool getReadWriteHandle();
	bool getCapabilities();
	void deviceIsConnected();
	void readThread();

	/* Callbacks from Register Device Notification */
	void deviceOnAttached();
	void deviceOnDetached();
	
	/* Instances */

	// Device identification
	unsigned short _vendorID;
	unsigned short _productID;
	unsigned short _versionNumber;
	GUID _hidGui;

	// Device connexion state
	bool _isDeviceAttached;
	bool _isDeviceConnected;

	// Device communication handle
	HANDLE _deviceHandle;
	HANDLE _readDeviceHandle;
	HANDLE _writeDeviceHandle;
	CHAR* _devicePathName;

	// Device capabilities
	HIDP_CAPS _deviceCaps;

	// Background worker
	std::thread _readTask;

	// To be able to read the data
	BYTE* _readData;
	unsigned short _readDataSize;

	// Device notifier
	HidDeviceNotifier* _deviceNotifier = NULL;

	HANDLE _hEventObject;
	OVERLAPPED _hidOverlapped;
};

#endif
#endif
