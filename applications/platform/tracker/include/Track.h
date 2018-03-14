
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

// @todo might refactor this to 'streambundle'

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
	bool getNextStreamIndex(int& index) const;

	uint64_t getNumStreams(void) const { return m_Streams.size(); };
	const StreamBase* getStream(uint64_t idx) const;

//	bool getNextChunk(uint64_t idx, const StreamChunk** output);

	uint64_t getSamplingRate(void) const;

	std::vector<StreamBase*>& getAllStreams(void) { return m_Streams; };

protected:

	Selection m_Selection;          // Which parts of the dataset have been selected

	std::string m_Name;

	std::vector<StreamBase*> m_Streams;
 	std::vector<DecoderBase*> m_Decoders;

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

};

