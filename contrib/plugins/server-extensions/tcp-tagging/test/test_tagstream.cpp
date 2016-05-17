#include "../ovasCTagStream.h"

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;

int main()
{
	bool ok=false;

	CTagStream tagStream1;

#ifdef _WIN32
	ok=true;
#else
	try {
		// The construction of the second TagStream must fail because of port already in use.
		CTagStream tagStream2;
	}
	catch( std::exception& ) {
		ok=true;
	}
#endif

	// The construction must succeed because another port is used.
	CTagStream tagStream3(15362);

	if (!ok)
		return 1;

	return 0;
}
