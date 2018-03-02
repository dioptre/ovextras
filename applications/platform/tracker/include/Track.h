
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>

#include "StreamBase.h"
#include "Marker.h"
#include "Range.h"
#include "Selection.h"

#include "DecoderBase.h"

class Track {
public:

	Track(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx) {}; 

//	Track(const std::string& trackName) : m_Name(trackName) { };

	// Load a new EEG dataset as the track
	bool initialize(const char *filename);
	bool uninitialize(void);

	// Rewind all streams
	bool rewind(void);

	// Basically an iterator; should only return part covered by m_vSelection; @todo may need re-epoching
	bool getNextStream(StreamBase** output);

//	bool getNextChunk(uint64_t idx, const StreamChunk** output);

	uint64_t getSamplingRate(void) const;

	Selection m_Selection;          // Which parts of the dataset have been selected

	std::string m_Name;

	std::map<uint64_t, StreamBase*> m_Streams;
 	std::map<uint64_t, DecoderBase*> m_Decoders;

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

};

