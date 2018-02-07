
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ovCMatrix.h>

#include "Marker.h"

class Dataset {
public:

	std::vector<Marker> m_vMarkers;           // Replace with IStimulationSet? 
	std::vector<OpenViBE::CMatrix> m_vEEG;    // Signal
	uint64_t m_samplingRate;
	uint64_t m_Duration;
	uint64_t m_chunkSize;

	std::string m_Filename;

};

