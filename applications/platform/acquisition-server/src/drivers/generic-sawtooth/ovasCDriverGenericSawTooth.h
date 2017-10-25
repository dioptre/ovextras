#ifndef __OpenViBE_AcquisitionServer_CDriverGenericSawTooth_H__
#define __OpenViBE_AcquisitionServer_CDriverGenericSawTooth_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverGenericSawTooth 
	 * \author Yann Renard (INRIA)
	 */
	class CDriverGenericSawTooth : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverGenericSawTooth(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
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

		OpenViBE::uint32 m_ui32ExternalBlockSize;
		std::vector<OpenViBE::float32> m_vSample;

		OpenViBE::uint64 m_ui64TotalSampleCount;

	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverGenericSawTooth_H__
