
#include "Sink.h"

#include <iostream>

bool Sink::initialize(const char *xmlFile) 
{ 
	std::cout << "Sink: Initializing with "	<< xmlFile << "\n";

	return false; 
}

bool Sink::uninitialize(void) 
{ 
	std::cout << "Sink: Uninitializing\n";

	return true;
}

bool Sink::pushChunk(const OpenViBE::CMatrix& chunk)
{
	std::cout << "Sink: Pushing out chunk" << "\n";

	return true;
}

