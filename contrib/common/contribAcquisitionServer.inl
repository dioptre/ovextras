/*
#include "openeeg-modulareeg/src/ovasCDriverOpenEEGModularEEG.h"
#include "field-trip-protocol/src/ovasCDriverFieldtrip.h"
#include "brainproducts-brainvisionrecorder/src/ovasCDriverBrainProductsBrainVisionRecorder.h"
*/

#include "ovasCPluginExternalStimulations.h"

#include "ovasCDriverBioSemiActiveTwo.h"
#include "ovasCDriverBrainmasterDiscovery.h"
#include "ovasCDriverBrainProductsActiCHamp.h"
#include "ovasCDriverBrainProductsBrainVisionRecorder.h"
#include "ovasCDriverCognionics.h"
#include "ovasCDriverCtfVsmMeg.h"
#include "ovasCDriverGTecGUSBamp.h"
#include "ovasCDriverGTecGUSBampLegacy.h"
#include "ovasCDriverGTecGMobiLabPlus.h"
#include "ovasCDriverFieldtrip.h"
#include "ovasCDriverMitsarEEG202A.h"
#include "ovasCDriverOpenALAudioCapture.h"
#include "ovasCDriverOpenEEGModularEEG.h"
#include "ovasCDriverTMSi.h"

namespace OpenViBEContributions {




	void initiateContributions(OpenViBEAcquisitionServer::CAcquisitionServerGUI* pGUI, OpenViBEAcquisitionServer::CAcquisitionServer* pAcquisitionServer, const OpenViBE::Kernel::IKernelContext& rKernelContext, std::vector<OpenViBEAcquisitionServer::IDriver*>* vDriver)
	{
#if defined TARGET_HAS_ThirdPartyBioSemiAPI
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverBioSemiActiveTwo(pAcquisitionServer->getDriverContext()));
#endif
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverBrainProductsBrainVisionRecorder(pAcquisitionServer->getDriverContext()));
#if defined WIN32
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverCognionics(pAcquisitionServer->getDriverContext()));
#endif
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverCtfVsmMeg(pAcquisitionServer->getDriverContext()));
#if defined TARGET_HAS_ThirdPartyGUSBampCAPI
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverGTecGUSBamp(pAcquisitionServer->getDriverContext()));
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverGTecGUSBampLegacy(pAcquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverGTecGMobiLabPlus(pAcquisitionServer->getDriverContext()));
#endif
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverFieldtrip(pAcquisitionServer->getDriverContext()));
#if defined TARGET_OS_Windows
 #if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverBrainmasterDiscovery(pAcquisitionServer->getDriverContext()));
 #endif
 #if defined TARGET_HAS_ThirdPartyActiCHampAPI
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverBrainProductsActiCHamp(pAcquisitionServer->getDriverContext()));
 #endif
 #if defined(TARGET_HAS_ThirdPartyMitsar)
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverMitsarEEG202A(pAcquisitionServer->getDriverContext()));
 #endif
#endif

#if defined TARGET_HAS_ThirdPartyOpenAL
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverOpenALAudioCapture(pAcquisitionServer->getDriverContext()));
#endif

		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverOpenEEGModularEEG(pAcquisitionServer->getDriverContext()));

#if defined TARGET_OS_Windows
		vDriver->push_back(new OpenViBEAcquisitionServer::CDriverTMSi(pAcquisitionServer->getDriverContext()));
#endif

		pGUI->registerPlugin(new OpenViBEAcquisitionServer::OpenViBEAcquisitionServerPlugins::CPluginExternalStimulations(rKernelContext));
	}

}
