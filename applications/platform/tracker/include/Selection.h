
#pragma once

#include <string>
#include <vector>

#include "Marker.h"
#include "Range.h"

class Selection {
public:

	// Adding and removing to selection
	bool addRange(const Range& range) { return false; };
	bool removeRange(const Range& range) { return false; };
	
	// Range query
	bool isSelected(const uint64_t timeStamp) { return false; };

	std::vector<Range> m_vSelection;          // Which parts of the dataset have been selected

};

