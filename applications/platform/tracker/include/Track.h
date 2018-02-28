
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>

#include "Stream.h"
#include "Marker.h"
#include "Range.h"
#include "Selection.h"

class Track {
public:

	Track(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx) {}; 

//	Track(const std::string& trackName) : m_Name(trackName) { };

	// Load a new EEG dataset as the track
	bool initialize(const char *filename);
	bool uninitialize(void);

	// Basically an iterator; should only return part covered by m_vSelection; @todo may need re-epoching

	bool getNextStream(Stream** output);

//	bool getNextChunk(uint64_t idx, const StreamChunk** output);

	uint32_t getSamplingRate(void);

	Selection m_Selection;          // Which parts of the dataset have been selected

	std::string m_Name;

	std::map<uint64_t, Stream*> m_Streams;
	std::map<uint64_t, uint64_t> m_StreamPosition;

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

};

