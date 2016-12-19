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

#include <iostream>

#include "HidDevice.h"

#define READTHREADWAITTIMEOUT 100

HidDevice::HidDevice(unsigned short vendorID, unsigned short productID, unsigned short receivdedDataSize)
{
	setHidDeviceInfos(vendorID, productID, receivdedDataSize);
}

HidDevice::HidDevice()
{
	_hEventObject = CreateEvent(NULL, TRUE, FALSE, "HIDUSBRcv");
	_hidOverlapped.hEvent = _hEventObject;
	_hidOverlapped.Offset = 0;
	_hidOverlapped.OffsetHigh = 0;
}

HidDevice::~HidDevice()
{
	free(_deviceNotifier);
	_deviceNotifier = NULL;
}

void HidDevice::setHidDeviceInfos(unsigned short vendorID, unsigned short productID, unsigned short receivdedDataSize)
{
	_vendorID = vendorID;
	_productID = productID;
	_readDataSize = receivdedDataSize;

	// Init the connexion state
	_isDeviceAttached = false;
	_isDeviceConnected = false;
}

bool HidDevice::connect()
{
	// Get the HID class identifier
	HidD_GetHidGuid(&_hidGui);

	// Get HID information set in the PC
	HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&_hidGui, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	// Ensure that HID information set has been returned
	if (!deviceInfoSet)
	{
		std::cout << "HidDevice::connect() -> Error: Problem to load HID information set" << std::endl;
		return false;
	}

	// Browse through device interfaces
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	BOOL isDeviceFound = 1;
	unsigned int memberIndex = 0;
	PSP_INTERFACE_DEVICE_DETAIL_DATA deviceInterfaceDetailData;
	ULONG requiredSize; // Size of device detail data
	HIDD_ATTRIBUTES hidAttributes;
	while (isDeviceFound)
	{
		// Get the device interface at the index "memberIndex"
		isDeviceFound = SetupDiEnumDeviceInterfaces(deviceInfoSet, 0, &_hidGui, memberIndex, &deviceInterfaceData);

		// Ensure that a device has been found at the specified index
		if (!isDeviceFound)
		{
			isDeviceFound = 0;
		}
		else
		{
			// Get the details with null values to get the required size of the buffer
			SetupDiGetDeviceInterfaceDetail(deviceInfoSet,
				&deviceInterfaceData,
				NULL, //interfaceDetail,
				0, //interfaceDetailSize,
				&requiredSize,
				0); //infoData))

			// Allocate the buffer
			deviceInterfaceDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredSize);
			deviceInterfaceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

			// Fill the buffer with the device details
			if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData,
					deviceInterfaceDetailData, requiredSize, &requiredSize, NULL))
			{
				// Ensure to free the dynamic allocated memory
				SetupDiDestroyDeviceInfoList(deviceInfoSet);
				free(deviceInterfaceDetailData);
				std::cout << "HidDevice::connect() -> Error: failed to get device info" << std::endl;
				return false;
			}

			// Details about the device is available
			HANDLE hidDeviceObject = CreateFile(deviceInterfaceDetailData->DevicePath, 0, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

			// Ensure that the handle is valid
			if (!hidDeviceObject)
			{
				std::cout << "HidDevice::connect() -> Error: failed to get handle of the HID" << std::endl;
				return false;
			}

			// Get the attribute of the HID
			if (!HidD_GetAttributes(hidDeviceObject, &hidAttributes))
			{
				std::cout << "HidDevice::connect() -> Error: failed to get HID attributes" << std::endl;
				return false;
			}


			if (hidAttributes.VendorID == _vendorID && hidAttributes.ProductID == _productID)
			{
				// We found the targeted device ... Save the following:
				// - HID device handle
				_deviceHandle = hidDeviceObject;

				// - Version number
				_versionNumber = hidAttributes.VersionNumber;

				// `- Device Path Name
				_devicePathName = deviceInterfaceDetailData->DevicePath;

				// - Configure the device for read and write
				if (!configure())
				{
					return false;
				}

				// At this step it's guaranteed that the device has been detected and connected
				deviceIsConnected();

				// Ensure to free the dynamic memory allocated
				free(deviceInterfaceDetailData);

				// The device has been found and is connected
				return true;
			}

			// Next device
			memberIndex++;

		}

	}

	// No device corresponding to the PID and VID
	return false;
}

bool HidDevice::configure()
{
	// Get the read and write handles
	if (!getReadWriteHandle())
	{
		return false;
	}

	// Get the capabilities of the device
	if (!getCapabilities())
	{
		return false;
	}

	// The device has been correctly configured
	return true;
}

bool HidDevice::getReadWriteHandle()
{
	// Open the readHandle to the device
	_readDeviceHandle = CreateFile(_devicePathName, GENERIC_WRITE|GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	// Did we open the readHandle successfully?
	if (!_readDeviceHandle)
	{
		std::cout << "HidDevice::configure() -> Error: Fail to get the read handle of the device" << std::endl;
		return false;
	}

	// Open the readHandle to the device
	_writeDeviceHandle = CreateFile(_devicePathName, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, 0, NULL);

	// Did we open the readHandle successfully?
	if (!_writeDeviceHandle)
	{
		std::cout << "HidDevice::configure() -> Error: Fail to get the write handle of the device" << std::endl;
		return false;
	}

	return true;
}

bool HidDevice::getCapabilities()
{
	// Get the preparsed data in order to request the device capabilities
	PHIDP_PREPARSED_DATA preparsedData;
	BOOLEAN result = HidD_GetPreparsedData(_deviceHandle, &preparsedData);
	if (result != TRUE)
	{
		std::cout << "HidDevice::configure() -> Error: Fail to get the preparsed data" << std::endl;
		return false;
	}

	// Get the capabilities
	NTSTATUS status = HidP_GetCaps(preparsedData, &_deviceCaps);
	if (status != HIDP_STATUS_SUCCESS)
	{
		std::cout << "HidDevice::configure() -> Error: Fail to get the collection capability information" << std::endl;
		return false;
	}

	// Free memory
	BOOLEAN isFreeCompleted;
	if (preparsedData)
	{
		isFreeCompleted = HidD_FreePreparsedData(preparsedData);
	}

	// Ensure memory has been freed correcly
	if (!isFreeCompleted)
	{
		std::cout << "HidDevice::configure() -> Error: Fail to free the preparsed data in memory" << std::endl;
		return false;
	}

	return true;
}

bool HidDevice::writeToDevice(BYTE data[], int nbOfBytes)
{	
	// Make sure that the device is connected
	if (_isDeviceConnected)
	{
		DWORD dwBytesToWrite = (DWORD)nbOfBytes;
		DWORD dwBytesWritten = 0;

		BOOL writeResult = WriteFile(_writeDeviceHandle, data, dwBytesToWrite, &dwBytesWritten, NULL);

		if (!writeResult)
		{
			std::cout << "HidDevice::writeToDevice() -> Error: Problem sending message to the device" << std::endl;
			return false;
		}

		//std::cout << "HidDevice::writeToDevice() -> Data sent to the device" << std::endl;
		return true;
	}
	else
	{
		std::cout << "HidDevice::writeToDevice() -> Error: The device is not connected." << std::endl;
		return false;
	}
}

void HidDevice::readThread()
{
	_readData = new BYTE[_readDataSize];

	DWORD nbByteRead;
	DWORD waitReturn;
	int nb = 0;
	BOOL result;

	while (_isDeviceConnected)
	{
		result = ReadFile(_readDeviceHandle, _readData, _readDataSize, NULL, (LPOVERLAPPED)&_hidOverlapped);
		if (result == 1 || (result == 0 && GetLastError() == ERROR_IO_PENDING))
		{
			do
			{
				waitReturn = WaitForSingleObject(_hEventObject, READTHREADWAITTIMEOUT);
			} 
			while ((waitReturn == WAIT_TIMEOUT) && (_isDeviceConnected == true));
			switch (waitReturn)
			{
				case WAIT_OBJECT_0: // Data returned
				{
					nbByteRead = 0;
					if (GetOverlappedResult(_readDeviceHandle, &_hidOverlapped, &nbByteRead, FALSE) && nbByteRead == _readDataSize)
					{
						// Pass the data read to the client method
						if (dataReceived)
						{
							dataReceived(_readData);
						}
					}
					else
					{
						if (_isDeviceConnected)
						{
							std::cout << "HidDevice::readThread():incomplete -> Error: Reading data from device failed." << std::endl;
						}
					}
					break;
				}
				default:
				{
					if (_isDeviceConnected)
					{
						std::cout << "HidDevice::readThread():waitReturn -> Error: Reading data from device failed." << std::endl;
					}
					break;
				}
			}
		}
		else
		{
			if (_isDeviceConnected)
			{
				std::cout << "HidDevice::readThread() -> Error: Reading data from device failed." << std::endl;
			}
		}
		ResetEvent(_hEventObject);
	}
	// Free the memory allocated for reading data
	delete[] _readData;
	_readData = NULL;
	
}

void HidDevice::deviceIsConnected()
{
	// Device is attached
	_isDeviceAttached = true;

	// flag the device connexion state
	_isDeviceConnected = true;

	// Start a separate thread for reading data from the device
	_readTask = std::thread(&HidDevice::readThread, this);
	_readTask.detach();

	// Attach the client callback method
	if (deviceConnected)
	{
		deviceConnected();
	}

	// Start the Register Device Notification just once
	if (_deviceNotifier == NULL)
	{
		_deviceNotifier = new HidDeviceNotifier(_hidGui, _devicePathName);

		// Bind the callback methods
		_deviceNotifier->deviceAttached = std::bind(&HidDevice::deviceOnAttached, this);
		_deviceNotifier->deviceDetached = std::bind(&HidDevice::deviceOnDetached, this);

		// Register the device for notification
		_deviceNotifier->startRegistration();
	}
}

void HidDevice::deviceOnAttached()
{
	std::cout << "Callback - Device attached" << std::endl;
	_isDeviceAttached = true;

	// Execute the callback if defined
	if (deviceAttached)
	{
		deviceAttached();
	}
}

void HidDevice::deviceOnDetached()
{
	std::cout << "Callback - Device detached" << std::endl;
	_isDeviceAttached = false;
	_isDeviceConnected = false;
	
	// Execute the callback if defined
	if (deviceDetached)
	{
		deviceDetached();
	}
}


#endif
#endif
