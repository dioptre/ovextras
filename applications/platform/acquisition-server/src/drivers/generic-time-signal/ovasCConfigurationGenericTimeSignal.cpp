#include "ovasCConfigurationGenericTimeSignal.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

CConfigurationGenericTimeSignal::CConfigurationGenericTimeSignal(IDriverContext& rDriverContext, const char* sGtkBuilderFileName)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
{
}

boolean CConfigurationGenericTimeSignal::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	return true;
}

boolean CConfigurationGenericTimeSignal::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		// 
	}

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency)
	{
		return false;
	}

	return true;
}


