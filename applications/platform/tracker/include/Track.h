
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ovCMatrix.h>

#include "Marker.h"
#include "Range.h"
#include "Selection.h"
#include "Dataset.h"

class Track {
public:

//	Track(const std::string& trackName) : m_Name(trackName) { };

	// Load a new EEG dataset as the track
	bool initialize(const char *filename);
	bool uninitialize(void);

	// Basically an iterator; should only return part covered by m_vSelection; @todo may need re-epoching
	bool getNextChunk(uint64_t idx, const OpenViBE::CMatrix** output);

	Dataset m_Dataset;				// The actual EEG data and markers
	Selection m_Selection;          // Which parts of the dataset have been selected

	std::string m_Name;

};

