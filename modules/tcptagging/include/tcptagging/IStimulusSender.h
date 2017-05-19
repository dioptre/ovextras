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

		// Connect to the TCP Tagging plugin of the Acquisition Server.
		// If sAddress is empty string, the StimulusSender will be inactive and connect() will not print an error but returns false.
		virtual TCPTagging::boolean connect(const char* sAddress, const char* sStimulusPort) = 0;

		// Send a stimulation. Set Timestamp to 0 for immediate tagging (also the default).
		virtual TCPTagging::boolean sendStimulation(TCPTagging::uint64 ui64Stimulation, TCPTagging::uint64 ui64Timestamp = 0) = 0;

		// To allow derived class' destructor to be called
		virtual ~IStimulusSender(void) { }

	};

	// Clients are constructed via this call.
	extern OV_API TCPTagging::IStimulusSender* createStimulusSender(void);

	/*
	* \class IStimulusSender
	* \author Thierry Gaugry / Inria
	* \brief @todo Describe...
	*/
	class OV_API IStimulusMultiSender {
	public:
		virtual TCPTagging::boolean connect(const char* sAddress, const char* sStimulusPort) = 0;

		virtual void addStimulationToWaitingList(TCPTagging::uint64 ui64Stimulation, TCPTagging::uint64 ui64Timestamp = 0) = 0;

		virtual TCPTagging::boolean sendStimulations(void) = 0;

		virtual TCPTagging::boolean sendStimulationNow(TCPTagging::uint64 ui64Stimulation, TCPTagging::uint64 ui64Timestamp = 0) = 0;

		virtual bool isCurrentlySending(void) = 0;

		virtual unsigned int getExecutionIndex(void) = 0;

		virtual void setExecutionIndex(unsigned int index) = 0;

		virtual ~IStimulusMultiSender(void) { }
	};

	extern OV_API TCPTagging::IStimulusMultiSender* createStimulusMultiSender(void);

}

#endif
