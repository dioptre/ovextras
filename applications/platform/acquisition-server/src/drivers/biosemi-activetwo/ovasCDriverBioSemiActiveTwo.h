/*
 * ovasCDriverBioSemiActiveTwo.h
 *
 * Copyright (c) 2012, Mensia Technologies SA. All rights reserved.
 * -- Rights transferred to Inria, contract signed 21.11.2014
 */

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#ifndef __OpenViBE_AcquisitionServer_CDriverBioSemiActiveTwo_H__
#define __OpenViBE_AcquisitionServer_CDriverBioSemiActiveTwo_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include "mCBridgeBioSemiActiveTwo.h"

#include <vector>

namespace OpenViBEAcquisitionServer
{
	class CDriverBioSemiActiveTwo : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverBioSemiActiveTwo(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual void release(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);

		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void) { return &m_oHeader; }

	protected:
		
		SettingsHelper m_oSettings;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		std::vector < unsigned int > m_vImpedance;
		std::vector < OpenViBE::float32 > m_vSample;
	
		OpenViBE::CStimulationSet m_oStimulationSet;
	
		Mensia::CBridgeBioSemiActiveTwo m_oBridge;
		
		std::vector<OpenViBE::boolean> m_vTriggers;
		OpenViBE::uint64 m_ui64SampleCount;

		OpenViBE::boolean m_bCMCurrentlyInRange;
		OpenViBE::uint32 m_ui32LastWarningTime;
		OpenViBE::uint32 m_ui32StartTime;

	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverBioSemiActiveTwo_H__
#endif // TARGET_HAS_ThirdPartyBioSemiAPI