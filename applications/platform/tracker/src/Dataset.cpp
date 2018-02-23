
#include "Dataset.h"

#include <iostream>

#include <system/ovCMath.h>

// Load a new EEG dataset as the track
bool Dataset::initialize(const char *filename) 
{ 
	std::cout << "Dataset: Initialize with " << filename << "\n";

	int numChannels = 6;
	m_samplingRate = 20;	
	m_chunkSize = 10;
	uint32_t numChunks = 50;

	m_vEEG.clear();

	uint64_t cnt = 0;

	for(uint32_t i=0;i<numChunks;i++)
	{
		OpenViBE::CMatrix* tmpMatrix = new OpenViBE::CMatrix();
		tmpMatrix->setDimensionCount(2);
		tmpMatrix->setDimensionSize(0,numChannels);
		tmpMatrix->setDimensionSize(1,m_chunkSize);
		for(uint32_t j=0;j<numChannels*m_chunkSize;j++)
		{
			tmpMatrix->getBuffer()[j] = (float)((System::Math::randomFloat32BetweenZeroAndOne()-0.5)*2.0);
		}

		m_vEEG.push_back(tmpMatrix);
	}

	return true; 
};

bool Dataset::uninitialize(void)
{
	std::cout << "Dataset: Unitialize\n";

	for(size_t i = 0; i<m_vEEG.size();i++)
	{
		delete m_vEEG[i];
	}
	m_vEEG.clear();

	return true;
}
