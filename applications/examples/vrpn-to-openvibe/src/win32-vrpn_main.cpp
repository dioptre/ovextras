
#if defined(WIN32)

#include "GenericVRPNServer.h"
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <windows.h>

#define DEFAULT_PORT 50555

int main(int argc, char** argv)
{
	GenericVRPNServer* vrpnServer = GenericVRPNServer::getInstance(DEFAULT_PORT);

	const char *buttonDevice = "button_test";
	const char *analogDevice = "analog_test";

	std::cout << "Creating devices [" << buttonDevice << "] and [" << analogDevice << "] using port [" << DEFAULT_PORT << "]\n";

	vrpnServer->addButton(buttonDevice, 1);
	vrpnServer->addAnalog(analogDevice, 2);

	double time = 0;
	double period = 0;

	while (true)
	{
		if (period >= 2 * M_PI)
		{
			vrpnServer->changeButtonState(buttonDevice, 0, 1 - vrpnServer->getButtonState(buttonDevice, 0));
			period = 0;
		}

		vrpnServer->changeAnalogState(analogDevice, sin(time), cos(time));

		time = time + 0.01;
		period = period + 0.01;
		
		vrpnServer->loop();

		// sleep for 10 miliseconds
		Sleep(10);
	}

	GenericVRPNServer::deleteInstance();
	vrpnServer = NULL;

	return 0;
}

#endif // WIN32
