#ifndef __TCPTagging_IStimulusSender_H__
#define __TCPTagging_IStimulusSender_H__

#include "defines.h"

namespace TCPTagging
{
	/*
	* \class IStimulusSender
	* \author Jussi T. Lindgren / Inria
	* \brief Interface of a simple client to send stimuli to Acquisition Server's TCP Tagging
	*/
	class OV_API IStimulusSender {
	public:

		// Connect to the TCP Tagging plugin of the Acquisition Server
		virtual bool connect(const char* sAddress, const char* sStimulusPort) = 0;

		// Send a stimulation. 
		// Or flags with FLAG_FPTIME if the provided time is fixed point.
		// Or flags with FLAG_AUTOSTAMP_CLIENTSIDE to set the latest timestamp before sending. Then, ui64Timestamp is ignored.
		// Or flags with FLAG_AUTOSTAMP_SERVERSIDE to request these server to stamp on receiveing. Then, ui64Timestamp is ignored.
		virtual bool sendStimulation(uint64_t ui64Stimulation, uint64_t ui64Timestamp = 0, 
			uint64_t ui64Flags = (FLAG_FPTIME | FLAG_AUTOSTAMP_CLIENTSIDE)) = 0;

		// To allow derived class' destructor to be called
		virtual ~IStimulusSender(void) { }

		// Note: duplicated in TCP Tagging plugin in AS
		enum TCP_Tagging_Flags {
			FLAG_FPTIME               = (1LL << 0),         // The time given is fixed point time.
			FLAG_AUTOSTAMP_CLIENTSIDE = (1LL << 1),         // Ignore given stamp, bake timestamp on client side before sending
			FLAG_AUTOSTAMP_SERVERSIDE = (1LL << 2)          // Ignore given stamp, bake timestamp on server side when receiving
		};

	};

	// Clients are constructed via this call.
	extern OV_API TCPTagging::IStimulusSender* createStimulusSender(void);
}

#endif
