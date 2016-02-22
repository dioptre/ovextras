#ifdef TARGET_OS_Windows

#include "ovasCConfigurationDriverMensiaAcquisition.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

//TODO_JL Add the URL as parameter for configuration so it can be saved and loaded

CConfigurationDriverMensiaAcquisition::CConfigurationDriverMensiaAcquisition(IDriverContext& rDriverContext, const char* sGtkBuilderFileName)
	:CConfigurationBuilder(sGtkBuilderFileName)
	 ,m_rDriverContext(rDriverContext)
{
}

boolean CConfigurationDriverMensiaAcquisition::preConfigure(void)
{
	if (!CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	//TODO_JL call preConfigure from DLL

	return true;
}

boolean CConfigurationDriverMensiaAcquisition::postConfigure(void)
{
	if (m_bApplyConfiguration)
	{
		//TODO_JL call postConfigure from DLL
	}

	if (!CConfigurationBuilder::postConfigure())
	{
		return false;
	}

	return true;
}

#endif // TARGET_OS_Windows
