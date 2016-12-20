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

#include "HidDeviceNotifier.h"

// Register Device Notification method
int registerNotification(HidDeviceNotifier* notifier);

#endif
#endif // TARGET_OS_Windows
