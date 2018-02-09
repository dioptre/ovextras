
#include "Track.h"

#include <iostream>

bool Track::initialize(const char *filename) 
{ 
	return m_Dataset.initialize(filename); 
};

bool Track::uninitialize(void)
{
	return m_Dataset.uninitialize();
}

bool Track::getNextChunk(uint64_t idx, const OpenViBE::CMatrix** output)
{
	// @todo: check selection here

	if(m_Dataset.m_vEEG.size()<=idx)
	{
		return false;
	}

	*output = m_Dataset.m_vEEG[static_cast<unsigned int>(idx)];

	return true;
}
