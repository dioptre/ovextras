
#pragma once

#include <openvibe/ovCMatrix.h>

class Sink {
public:

	bool initialize(const char *xmlFile);
	bool uninitialize(void);
	bool pushChunk(const OpenViBE::CMatrix& chunk);


};

