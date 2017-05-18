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

		private:
			Engine * m_pMatlabEngine;
			OpenViBE::CString m_sBoxInstanceVariableName; //must be unique

			OpenViBE::CString escapeMatlabString(OpenViBE::CString sStringToEscape);
		};
	};
};

#endif // TARGET_HAS_ThirdPartyMatlab

#endif // __OpenViBEPlugins_MatlabHelper_H__
