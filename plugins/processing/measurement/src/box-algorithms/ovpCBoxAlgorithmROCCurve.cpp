#include "ovpCBoxAlgorithmROCCurve.h"

#include <iostream>
#include <algorithm>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Measurement;

boolean compareCTimelineStimulationPair(const CTimelineStimulationPair& elt1, const CTimelineStimulationPair& elt2)
{
	return elt1.second < elt2.second;
}

boolean compareValueAndStimulationTimelinePair(const CTimelineStimulationPair& elt1, const CTimelineValuePair& elt2)
{
	return elt1.second < elt2.first;
}

boolean compareRocValuePair(const CRocPairValue& elt1, const CRocPairValue& elt2)
{
	return elt1.second > elt2.second;
}

boolean isPositive(const CRocPairValue &elt1)
{
	return elt1.first;
}

boolean CBoxAlgorithmROCCurve::initialize(void)
{
	m_oExpectedDecoder.initialize(*this, 0);
	m_oClassificationValueDecoder.initialize(*this, 1);

	m_oComputationTrigger = CIdentifier(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));

	m_pWidget = GTK_WIDGET(gtk_notebook_new());

	for(size_t i = 1; i < this->getStaticBoxContext().getSettingCount() ; ++i)
	{
		CIdentifier l_oClassLabel(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));
		CString l_sClassName;
		this->getStaticBoxContext().getSettingName(i, l_sClassName);

		m_oClassStimulationSet.insert(l_oClassLabel);

		m_oDrawerList.push_back(new CROCCurveDraw(GTK_NOTEBOOK(m_pWidget), i-1, l_sClassName));
	}
	getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pWidget);

	return true;
}

boolean CBoxAlgorithmROCCurve::uninitialize(void)
{

	m_oExpectedDecoder.uninitialize();
	m_oClassificationValueDecoder.uninitialize();

	for(size_t i = 0; i < m_oDrawerList.size(); ++i)
	{
		delete m_oDrawerList[i];
	}

	return true;
}


boolean CBoxAlgorithmROCCurve::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}


boolean CBoxAlgorithmROCCurve::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//First let's deal with the expected.
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oExpectedDecoder.decode(i);

		if(m_oExpectedDecoder.isHeaderReceived())
		{
			m_oStimulationTimeline.clear();
		}

		if(m_oExpectedDecoder.isBufferReceived())
		{
			IStimulationSet* l_pStimulationSet = m_oExpectedDecoder.getOutputStimulationSet();
			for(size_t k = 0; k < l_pStimulationSet->getStimulationCount(); ++k)
			{
				CIdentifier l_oStimulationIdentifier = l_pStimulationSet->getStimulationIdentifier(k);
				if(m_oClassStimulationSet.find(l_oStimulationIdentifier) != m_oClassStimulationSet.end())
				{
					m_oStimulationTimeline.push_back(CTimelineStimulationPair(l_oStimulationIdentifier, l_pStimulationSet->getStimulationDate(k)));
				}
				//We need to check if we receive the computation trigger
				if(l_oStimulationIdentifier == m_oComputationTrigger)
				{
					computeROCCurves();
				}
			}
		}
	}

	for(uint32 i = 0 ; i < l_rDynamicBoxContext.getInputChunkCount(1); ++i)
	{
		m_oClassificationValueDecoder.decode(i);
		if(m_oClassificationValueDecoder.isHeaderReceived())
		{
			m_oValueTimeline.clear();
		}
		if(m_oClassificationValueDecoder.isBufferReceived())
		{
			IMatrix* l_pMatrixValue = m_oClassificationValueDecoder.getOutputMatrix();
			//The matrix is suppose to have only one dimension
			float64* l_pArrayValue;

			if(l_pMatrixValue->getBufferElementCount() > 1)
			{
				l_pArrayValue = new float64[l_pMatrixValue->getBufferElementCount()];
				for(size_t k = 0; k < l_pMatrixValue->getBufferElementCount() ; ++k)
				{
					l_pArrayValue[k] = l_pMatrixValue->getBuffer()[k];
				}
			}
			else
			{
				l_pArrayValue = new float64[2];
				l_pArrayValue[0] = l_pMatrixValue->getBuffer()[0];
				l_pArrayValue[1] = 1 - l_pMatrixValue->getBuffer()[0];
			}

			uint64 timestamp = l_rDynamicBoxContext.getInputChunkEndTime(1, i); //the time in stimulation correspond to the end of the chunck (cf processorbox code)
			m_oValueTimeline.push_back(CTimelineValuePair(timestamp, l_pArrayValue));
		}
	}
	return true;
}

boolean CBoxAlgorithmROCCurve::computeROCCurves()
{
	std::cout << "Start computation of ROC curves" << std::endl;
	//Now we assiociate all values to the corresponding label
	std::sort(m_oStimulationTimeline.begin(), m_oStimulationTimeline.end(), compareCTimelineStimulationPair);//ensure the timeline is ok
	std::vector< CTimelineStimulationPair >::iterator m_oBound;

	for(size_t i = 0; i < m_oValueTimeline.size(); ++i)
	{
		CTimelineValuePair& l_rValuePair = m_oValueTimeline[i];
		m_oBound = std::lower_bound(m_oStimulationTimeline.begin(), m_oStimulationTimeline.end(), l_rValuePair, compareValueAndStimulationTimelinePair);
		if(m_oBound != m_oStimulationTimeline.begin())
		{
			--m_oBound;
			m_oLabelValueList.push_back(CLabelValuesPair(m_oBound->first, l_rValuePair.second));
		}
		else{
			//Impossible to find the corresponding stimulation
		}
	}

	//Wa cannot use the set because we need the correct order
	for(size_t i = 1; i < this->getStaticBoxContext().getSettingCount() ; ++i)
	{
		CIdentifier l_oClassLabel(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));
		computeOneROCCurve(l_oClassLabel, i-1);
	}
	//Now we ask to the current page to draw itself
	m_oDrawerList[gtk_notebook_current_page(GTK_NOTEBOOK(m_pWidget))]->forceRedraw();
	return true;
}

boolean CBoxAlgorithmROCCurve::computeOneROCCurve(CIdentifier rClassIdentifier, uint32 ui32ClassIndex)
{
	std::cout << "Compute ROC curve for class " << rClassIdentifier.toString() <<  " Index " << ui32ClassIndex << std::endl;
	std::vector < CRocPairValue > l_oRocpairValue;
	CRocVectorBuilder l_oVectorBuilder(l_oRocpairValue, rClassIdentifier, ui32ClassIndex);
	std::for_each(m_oLabelValueList.begin(), m_oLabelValueList.end(), l_oVectorBuilder);

	std::sort(l_oRocpairValue.begin(), l_oRocpairValue.end(), compareRocValuePair);


	uint32 l_ui32TruePositive = 0;
	uint32 l_ui32FalsePositive = 0;

	float64 l_f64TruePositiveRate = 0.;
	float64 l_f64FalsePositiveRate = 0.;

	const uint32 l_ui32ElementCount = l_oRocpairValue.size();
	const uint32 l_ui32PositiveCount = std::count_if(l_oRocpairValue.begin(), l_oRocpairValue.end(), isPositive);
	const uint32 l_ui32NegativeCount = l_ui32ElementCount - l_ui32PositiveCount;

	std::vector < CCoordinate >& l_oCoordinateVector = m_oDrawerList[ui32ClassIndex]->getCoordinateVector();

	for(size_t i = 0; i < l_oRocpairValue.size(); ++i)
	{
		l_ui32TruePositive += l_oRocpairValue[i].first;
		l_ui32FalsePositive = (i+1) - l_ui32TruePositive;
		l_f64FalsePositiveRate = ((float64)l_ui32FalsePositive) / l_ui32NegativeCount;
		l_f64TruePositiveRate = ((float64)l_ui32TruePositive) / l_ui32PositiveCount;
		l_oCoordinateVector.push_back(CCoordinate(l_f64FalsePositiveRate, l_f64TruePositiveRate));
	}
	m_oDrawerList[ui32ClassIndex]->generateCurve();

	return true;
}
