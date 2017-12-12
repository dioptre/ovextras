#ifndef __OpenViBE_AcquisitionServer_CDriverGenericTimeSignal_H__
#define __OpenViBE_AcquisitionServer_CDriverGenericTimeSignal_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverGenericTimeSignal 
	 * \author Jussi Lindgren (Inria)
	 *
	 * This driver may have some utility in debugging. For each sample, it returns the 
	 * current time as obtained from openvibe's System::Time:zgettime() converted to float seconds.
	 *
	 */
	class CDriverGenericTimeSignal : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverGenericTimeSignal(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
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

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		SettingsHelper m_oSettings;

		OpenViBE::uint64 m_ui64TotalSampleCount;
		OpenViBE::uint64 m_ui64StartTime;
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverGenericTimeSignal_H__
