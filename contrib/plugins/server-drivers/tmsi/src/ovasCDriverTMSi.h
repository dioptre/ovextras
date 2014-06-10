/*
 * ovasCDriverTMSi.h
 *
 * Copyright (c) 2014, Mensia Technologies SA. All rights reserved.
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

#ifndef __OpenViBE_AcquisitionServer_CDriverTMSi_H__
#define __OpenViBE_AcquisitionServer_CDriverTMSi_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include "ovas_base.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows

#include <gtk/gtk.h>
// Get Signal info

#include <windows.h>
#include <map>


namespace OpenViBEAcquisitionServer
{
	class CTMSiAccess;

	class CDriverTMSi : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverTMSi(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDriverTMSi(void);
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

		// saved parameters
		OpenViBE::CString m_sConnectionProtocol;
		OpenViBE::CString m_sDeviceIdentifier;
		OpenViBE::boolean m_bCommonAverageReference;
		OpenViBE::uint64 m_ui64ActiveEEGChannels;
		OpenViBE::CString m_sActiveAdditionalChannels;
		OpenViBE::uint64 m_ui64ImpedanceLimit;

		CTMSiAccess* m_pTMSiAccess;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

	protected:
		SettingsHelper m_oSettings;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::float32 *m_pSample;
		std::vector<OpenViBE::float64> m_vImpedance;

		OpenViBE::boolean m_bValid;

		OpenViBE::uint32 m_ui32TotalSampleReceived;
		OpenViBE::CStimulationSet m_oStimulationSet;

	private:
		OpenViBE::boolean m_bIgnoreImpedanceCheck;


	};
}

#endif // TARGET_OS_Windows

#endif // __OpenViBE_AcquisitionServer_CDriverTMSi_H__
