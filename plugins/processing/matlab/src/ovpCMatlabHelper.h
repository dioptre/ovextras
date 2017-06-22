#ifndef __OpenViBEPlugins_MatlabHelper_H__
#define __OpenViBEPlugins_MatlabHelper_H__

#if defined TARGET_HAS_ThirdPartyMatlab

#include "ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <engine.h>

namespace OpenViBEPlugins
{
	namespace Matlab
	{
		class CMatlabHelper
		{
		public:
			CMatlabHelper(OpenViBE::Kernel::ILogManager& logManager, OpenViBE::Kernel::IErrorManager& errorManager)
				: m_pMatlabEngine(nullptr), m_pLogManager(logManager), m_pErrorManager(errorManager) {}

			bool setStreamedMatrixInputHeader(uint32_t ui32InputIndex, OpenViBE::IMatrix * pMatrix);
			bool setFeatureVectorInputHeader(uint32_t ui32InputIndex, OpenViBE::IMatrix * pMatrix);
			bool setSignalInputHeader(uint32_t ui32InputIndex, OpenViBE::IMatrix * pMatrix, uint64_t ui64SamplingRate);
			bool setChannelLocalisationInputHeader(uint32_t ui32InputIndex, OpenViBE::IMatrix * pMatrix, bool bDynamic);
			bool setSpectrumInputHeader(uint32_t ui32InputIndex, OpenViBE::IMatrix * pMatrix, OpenViBE::IMatrix * pFrequencyAbscissa, uint64_t samplingRate);
			bool setStimulationsInputHeader(uint32_t ui32InputIndex);
			
			// The input buffers for streamed matrix and its children streams are the same.
			bool addStreamedMatrixInputBuffer(uint32_t ui32InputIndex, OpenViBE::IMatrix * pMatrix, uint64_t ui64OpenvibeStartTime,uint64_t ui64OpenvibeEndTime);
			bool addStimulationsInputBuffer(uint32_t ui32InputIndex,OpenViBE::IStimulationSet * pStimulationSet, uint64_t ui64OpenvibeStartTime,uint64_t ui64OpenvibeEndTime);
			
			bool getStreamedMatrixOutputHeader(uint32_t ui32OutputIndex, OpenViBE::IMatrix * pMatrix);
			bool getFeatureVectorOutputHeader(uint32_t ui32OutputIndex, OpenViBE::IMatrix * pMatrix);
			bool getSignalOutputHeader(uint32_t ui32OutputIndex, OpenViBE::IMatrix * pMatrix, uint64_t & rSamplingRate);
			bool getChannelLocalisationOutputHeader(uint32_t ui32OutputIndex, OpenViBE::IMatrix * pMatrix, bool &bDynamic);
			bool getSpectrumOutputHeader(uint32_t ui32OutputIndex, OpenViBE::IMatrix * pMatrix, OpenViBE::IMatrix * pFrequencyAbscissa, uint64_t &samplingRate);
			bool getStimulationsOutputHeader(uint32_t ui32OutputIndex, OpenViBE::IStimulationSet * pStimulationSet);
			
			// The output buffers for streamed matrix and its children streams are the same.
			bool popStreamedMatrixOutputBuffer(uint32_t ui32OutputIndex, OpenViBE::IMatrix * pMatrix, uint64_t& rStartTime, uint64_t& rEndTime);
			bool popStimulationsOutputBuffer(uint32_t ui32OutputIndex,OpenViBE::IStimulationSet * pStimulationSet, uint64_t& rStartTime, uint64_t& rEndTime);
		
			void setMatlabEngine(Engine * pEngine) { m_pMatlabEngine = pEngine; }
			void setBoxInstanceVariableName(OpenViBE::CString sName) { m_sBoxInstanceVariableName = sName; }


			uint32_t getUint32FromEnv(const char* name);
			uint64_t getUint64FromEnv(const char* name);
			uint64_t genUint64FromEnvConverted(const char* name);
			std::vector<OpenViBE::CString> getNamelist(const char* name);


			OpenViBE::Kernel::ILogManager& getLogManager(void) const { return m_pLogManager; }
			OpenViBE::Kernel::IErrorManager& getErrorManager(void) const { return m_pErrorManager; }

		private:
			Engine* m_pMatlabEngine;
			OpenViBE::Kernel::ILogManager& m_pLogManager;
			OpenViBE::Kernel::IErrorManager& m_pErrorManager;

			OpenViBE::CString m_sBoxInstanceVariableName; //must be unique
		};
	};
};

#endif // TARGET_HAS_ThirdPartyMatlab

#endif // __OpenViBEPlugins_MatlabHelper_H__
