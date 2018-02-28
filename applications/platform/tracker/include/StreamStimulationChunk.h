
#pragma once

#include <openvibe/ov_all.h>

#include "StreamChunk.h"

class StreamStimulationChunk : public StreamChunk
{
public:
	OpenViBE::CStimulationSet stimSet;

};

