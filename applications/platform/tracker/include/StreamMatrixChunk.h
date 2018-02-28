
#pragma once

#include <openvibe/ov_all.h>

#include "StreamChunk.h"

class StreamMatrixChunk : public StreamChunk
{
public:
	OpenViBE::CMatrix data;

};

