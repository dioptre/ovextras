
#include "Dataset.h"

#include <iostream>

// Load a new EEG dataset as the track
bool Dataset::initialize(const char *filename) 
{ 
	std::cout << "Dataset: Initialize with " << filename << "\n";
		
	m_vEEG.clear();

	for(uint32_t i=0;i<12;i++)
	{
		OpenViBE::CMatrix* tmpMatrix = new OpenViBE::CMatrix();
		tmpMatrix->setDimensionCount(2);
		tmpMatrix->setDimensionSize(0,10);
		tmpMatrix->setDimensionSize(1,20);
		for(uint32_t i=0;i<10*20;i++)
		{
			tmpMatrix->getBuffer()[i] = (float)i;
		}

		m_vEEG.push_back(tmpMatrix);
	}

	m_samplingRate = 10;	
	
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
