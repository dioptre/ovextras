#include "ovpCBoxAlgorithmOutlierRemoval.h"
// #include <cstdio>

#include <openvibe/ovITimeArithmetics.h>

#include <algorithm>
#include <iterator>

// #include <sstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace std;

boolean CBoxAlgorithmOutlierRemoval::initialize(void)
{
	m_oStimulationDecoder.initialize(*this, 0);
	m_oFeatureVectorDecoder.initialize(*this, 1);
	
	m_oStimulationEncoder.initialize(*this, 0);
	m_oFeatureVectorEncoder.initialize(*this, 1);

	// get the quantile parameters
	m_f64LowerQuantile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_f64UpperQuantile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64Trigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	m_f64LowerQuantile = std::min<float64>(std::max<float64>(m_f64LowerQuantile, 0.0), 1.0);
	m_f64UpperQuantile = std::min<float64>(std::max<float64>(m_f64UpperQuantile, 0.0), 1.0);

	m_ui64TriggerTime = -1LL;

	return true;
}

boolean CBoxAlgorithmOutlierRemoval::uninitialize(void)
{

	m_oFeatureVectorEncoder.uninitialize();
	m_oStimulationEncoder.uninitialize();

	m_oFeatureVectorDecoder.uninitialize();
	m_oStimulationDecoder.uninitialize();

	for(uint32 i=0;i<m_vDataset.size();i++) 
	{
		delete m_vDataset[i].m_pFeatureVectorMatrix;
		m_vDataset[i].m_pFeatureVectorMatrix = NULL;
	}
	m_vDataset.clear();

	return true;
}

boolean CBoxAlgorithmOutlierRemoval::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

bool pairLess(std::pair<float64,uint32> a, std::pair<float64,uint32> b)
{   
	return a.first < b.first;
};

boolean CBoxAlgorithmOutlierRemoval::pruneSet(std::vector<SFeatureVector>& l_vPruned)
{
	if(m_vDataset.size()==0) 
	{
		// nothing to do, ok
		return true;
	}		

	const uint32 l_ui32DatasetSize = m_vDataset.size();
	const uint32 l_ui32FeatureDims = m_vDataset[0].m_pFeatureVectorMatrix->getDimensionSize(0);

	const uint32 l_ui32LowerIndex = static_cast<uint32>(m_f64LowerQuantile * l_ui32DatasetSize);
	const uint32 l_ui32UpperIndex = static_cast<uint32>(m_f64UpperQuantile * l_ui32DatasetSize);

	this->getLogManager() << LogLevel_Trace << "Examined dataset is [" << l_ui32DatasetSize << "x" << l_ui32FeatureDims << "].\n";

	std::vector<uint32> l_vKeptIndexes;
	l_vKeptIndexes.resize(l_ui32DatasetSize);
	for(uint32 i=0;i<l_ui32DatasetSize;i++)
	{
		l_vKeptIndexes[i] = i;
	}

	std::vector< std::pair<float64,uint32> > l_vFeatureValues;
	l_vFeatureValues.resize(l_ui32DatasetSize);

	for(uint32 f=0;f<l_ui32FeatureDims;f++)
	{
		for(uint32 i=0;i<l_ui32DatasetSize;i++)
		{
			l_vFeatureValues[i] = std::pair<float64, uint32>(m_vDataset[i].m_pFeatureVectorMatrix->getBuffer()[f], i);
		}

		std::sort(l_vFeatureValues.begin(), l_vFeatureValues.end(), pairLess);

		std::vector<uint32> l_vNewIndexes;
		l_vNewIndexes.resize(l_ui32UpperIndex - l_ui32LowerIndex);
		for(uint32 j=l_ui32LowerIndex,cnt=0;j<l_ui32UpperIndex;j++,cnt++)
		{
			l_vNewIndexes[cnt]=l_vFeatureValues[j].second;
		}

		this->getLogManager() << LogLevel_Trace << "For feature " << (f+1) << ", the retained range is [" << l_vFeatureValues[l_ui32LowerIndex].first 
			<<  ", " <<  l_vFeatureValues[l_ui32UpperIndex-1].first << "]\n";

		std::sort(l_vNewIndexes.begin(), l_vNewIndexes.end());   

		std::vector<uint32> l_vIntersection;
		std::set_intersection(l_vNewIndexes.begin(), l_vNewIndexes.end(), l_vKeptIndexes.begin(), l_vKeptIndexes.end(), std::back_inserter(l_vIntersection));

		l_vKeptIndexes = l_vIntersection;

		this->getLogManager() << LogLevel_Debug << "After analyzing feat " << f << ", kept " << l_vKeptIndexes.size() << " examples.\n";
	
	}

	this->getLogManager() << LogLevel_Trace << "Kept " << static_cast<uint64>(l_vKeptIndexes.size()) 
		<< " examples in total (" << (100.0 * l_vKeptIndexes.size() / static_cast<float64>(m_vDataset.size())) 
		<< "% of " << static_cast<uint64>(m_vDataset.size()) << ")\n";

	l_vPruned.clear();
	for(uint32 i=0;i<l_vKeptIndexes.size();i++) 
	{
		l_vPruned.push_back(m_vDataset[l_vKeptIndexes[i]]);
	}

	return true;
}

boolean CBoxAlgorithmOutlierRemoval::process(void)
{
	// IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// Stimulations
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oStimulationDecoder.decode(i);
		if(m_oStimulationDecoder.isHeaderReceived())
		{
			m_oStimulationEncoder.encodeHeader();

			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oStimulationDecoder.isBufferReceived())
		{
			const IStimulationSet *stimSet = m_oStimulationDecoder.getOutputStimulationSet();
			for(uint32 s=0;s<stimSet->getStimulationCount();s++)
			{
				if(stimSet->getStimulationIdentifier(s) == m_ui64Trigger)
				{
					std::vector<SFeatureVector> l_vPruned;
					if(!pruneSet(l_vPruned))
					{
						return false;
					}

					// encode
					for(uint32 f=0;f<l_vPruned.size();f++)
					{
						OpenViBEToolkit::Tools::Matrix::copy(*m_oFeatureVectorEncoder.getInputMatrix(), *l_vPruned[f].m_pFeatureVectorMatrix);

						m_oFeatureVectorEncoder.encodeBuffer();

						l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_vPruned[f].m_ui64StartTime, l_vPruned[f].m_ui64EndTime);
					}

					const uint64 l_ui64HalfSecondHack = ITimeArithmetics::secondsToTime(0.5);
					m_ui64TriggerTime = stimSet->getStimulationDate(s) + l_ui64HalfSecondHack;
				}
			}

			m_oStimulationEncoder.getInputStimulationSet()->clear();

			if(m_ui64TriggerTime >= l_rDynamicBoxContext.getInputChunkStartTime(0, i) && m_ui64TriggerTime < l_rDynamicBoxContext.getInputChunkEndTime(0, i))
			{
				m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(m_ui64Trigger, m_ui64TriggerTime, 0);
				m_ui64TriggerTime = -1LL;
			}

			m_oStimulationEncoder.encodeBuffer();

			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oStimulationDecoder.isEndReceived())
		{
			m_oStimulationEncoder.encodeEnd();

			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
	}

	// Feature vectors

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		m_oFeatureVectorDecoder.decode(i);
		if(m_oFeatureVectorDecoder.isHeaderReceived())
		{
			OpenViBEToolkit::Tools::Matrix::copyDescription(*m_oFeatureVectorEncoder.getInputMatrix(), *m_oFeatureVectorDecoder.getOutputMatrix());

			m_oFeatureVectorEncoder.encodeHeader();

			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));
		}
		
		// pad feature to set
		if(m_oFeatureVectorDecoder.isBufferReceived()) 
		{
			const IMatrix* pFeatureVectorMatrix = m_oFeatureVectorDecoder.getOutputMatrix();

			CBoxAlgorithmOutlierRemoval::SFeatureVector l_oFeatureVector;
			l_oFeatureVector.m_pFeatureVectorMatrix=new CMatrix();
			l_oFeatureVector.m_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
			l_oFeatureVector.m_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);

			OpenViBEToolkit::Tools::Matrix::copy(*l_oFeatureVector.m_pFeatureVectorMatrix, *pFeatureVectorMatrix);
			m_vDataset.push_back(l_oFeatureVector);
		}

		if(m_oFeatureVectorDecoder.isEndReceived()) 
		{
			m_oFeatureVectorEncoder.encodeEnd();

			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));
		}
	}

	return true;
}
