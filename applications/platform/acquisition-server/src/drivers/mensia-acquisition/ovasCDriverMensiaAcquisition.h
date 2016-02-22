#ifndef __OpenViBE_AcquisitionServer_CDriverMensiaAcquisition_H__
#define __OpenViBE_AcquisitionServer_CDriverMensiaAcquisition_H__

#ifdef TARGET_OS_Windows


#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBEAcquisitionServer
{
	class CDriverMensiaAcquisition : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverMensiaAcquisition(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sDriverIdentifier);
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
		OpenViBE::boolean m_bValid;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::float32* m_pSample;

		OpenViBE::uint32 m_ui32TotalSampleCount;
		OpenViBE::uint64 m_ui64StartTime;

	private:
		// Settings
		OpenViBE::CString m_sDeviceURL;

		template<typename T>
		void loadDLLfunct(T* functionPointer, const char* functionName);
		OpenViBE::uint32 m_ui32DriverId;

	};
}

#endif // TARGET_OS_Windows

#endif // __OpenViBE_AcquisitionServer_CDriverMensiaAcquisition_H__
