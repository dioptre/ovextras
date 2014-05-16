/*
 * Receives data from OpenViBE's VRPN boxes
 *
 * See here: http://openvibe.inria.fr/vrpn-tutorial-sending-data-from-openvibe-to-an-external-application/
 *
 */

#include <iostream>
#include <sstream>

#include <vrpn_Button.h>
#include <vrpn_Analog.h>
 
#define DEFAULT_PORT 3883

void VRPN_CALLBACK vrpn_button_callback(void* user_data, vrpn_BUTTONCB button)
{
    std::cout << "Button ID : " << button.button << " / Button State : " << button.state << std::endl;
 
    if (button.button == 1)
    {
        *(bool*)user_data = false;
    }
}
 
void VRPN_CALLBACK vrpn_analog_callback(void* user_data, vrpn_ANALOGCB analog)
{
    for (int i = 0; i < analog.num_channel; i++)
    {
        std::cout << "Analog Channel : " << i << " / Analog Value : " << analog.channel[i] << std::endl;
    }
}
 
int main(int argc, char** argv)
{
    /* flag used to stop the program execution */
    bool running = true;
 
    /* VRPN Button object */
    vrpn_Button_Remote* VRPNButton;
 
    /* Binding of the VRPN Button to a callback */
	std::stringstream buttonUrl;
	buttonUrl << std::string("openvibe_vrpn_button@localhost:") << (argc>1 ? atoi(argv[1]) : DEFAULT_PORT);
	std::cout << "Server button URL = " << buttonUrl.str().c_str() << "\n";
    VRPNButton = new vrpn_Button_Remote(buttonUrl.str().c_str());
    VRPNButton->register_change_handler( &running, vrpn_button_callback );
 
    /* VRPN Analog object */
    vrpn_Analog_Remote* VRPNAnalog;
 
    /* Binding of the VRPN Analog to a callback */
	std::stringstream analogUrl;
	analogUrl << std::string("openvibe_vrpn_analog@localhost:") << (argc>1 ? atoi(argv[1]) : DEFAULT_PORT);
	std::cout << "Server analog URL = " << analogUrl.str().c_str() << "\n";
    VRPNAnalog = new vrpn_Analog_Remote(analogUrl.str().c_str());
    VRPNAnalog->register_change_handler( NULL, vrpn_analog_callback );
 
    /* The main loop of the program, each VRPN object must be called in order to process data */
    while (running)
    {
        VRPNButton->mainloop();
    }
 
    return 0;
}
