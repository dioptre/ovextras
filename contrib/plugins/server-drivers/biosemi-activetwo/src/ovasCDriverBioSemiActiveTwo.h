/*
 * ovasCDriverBioSemiActiveTwo.h
 *
 * Copyright (c) 2012, Mensia Technologies SA. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
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