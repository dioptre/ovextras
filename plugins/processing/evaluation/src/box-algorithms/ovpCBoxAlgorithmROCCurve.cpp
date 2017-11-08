
#if defined(TARGET_HAS_ThirdPartyGTK)

#include "ovpCBoxAlgorithmROCCurve.h"

#include <iostream>
#include <algorithm>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Evaluation;

bool compareCTimelineStimulationPair(const CTimestampLabelPair& rElt1, const CTimestampLabelPair& rElt2)
{
	return rElt1.first < rElt2.first;
}

bool compareValueAndStimulationTimelinePair(const CTimestampLabelPair& rElt1, const CTimestampValuesPair& rElt2)
{
	return rElt1.first < rElt2.first;
}

bool compareRocValuePair(const CRocPairValue& rElt1, const CRocPairValue& rElt2)
{
	return rElt1.second > rElt2.second;
}

bool isPositive(const CRocPairValue &rElt1)
{
	return rElt1.first;
}

bool CBoxAlgorithmROCCurve::initialize(void)
{
	m_oExpectedDecoder.initialize(*this, 0);
	m_oClassificationValueDecoder.initialize(*this, 1);

	m_oComputationTrigger = CIdentifier(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));

	m_pWidget = GTK_WIDGET(gtk_notebook_new());

	for(size_t i = 2; i < this->getStaticBoxContext().getSettingCount() ; ++i)
	{
		CIdentifier l_oClassLabel(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));
		CString l_sClassName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);

		m_oClassStimulationSet.insert(l_oClassLabel);

		m_oDrawerList.push_back(new CROCCurveDraw(GTK_NOTEBOOK(m_pWidget), i-1, l_sClassName));
	}

	m_visualizationContext = dynamic_cast<OpenViBEVisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationContext));
	m_visualizationContext->setWidget(*this, m_pWidget);

	return true;
}

bool CBoxAlgorithmROCCurve::uninitialize(void)
{
	m_oExpectedDecoder.uninitialize();
	m_oClassificationValueDecoder.uninitialize();

	for(size_t i = 0; i < m_oDrawerList.size(); ++i)
	{
		delete m_oDrawerList[i];
	}

	//The m_oValueTimeline vector contains each dynamically instantiate values that need to be free'd
	for(size_t i = 0; i < m_oValueTimeline.size(); ++i)
	{
		delete m_oValueTimeline[i].second;
	}

	this->releasePluginObject(m_visualizationContext);

	return true;
}


bool CBoxAlgorithmROCCurve::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}


bool CBoxAlgorithmROCCurve::process(void)
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
					m_oStimulationTimeline.push_back(CTimestampLabelPair(l_pStimulationSet->getStimulationDate(k), l_oStimulationIdentifier.toUInteger() ));
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

			if(l_pMatrixValue->getBufferElementCount() == 0)
			{
				this->getLogManager() << LogLevel_Error << "Received zero-sized buffer\n";
				return false;
			}
			else if(l_pMatrixValue->getBufferElementCount() > 1)
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

			uint64 l_ui64timestamp = l_rDynamicBoxContext.getInputChunkEndTime(1, i); //the time in stimulation correspond to the end of the chunck (cf processorbox code)
			m_oValueTimeline.push_back(CTimestampValuesPair(l_ui64timestamp, l_pArrayValue));
		}
	}
	return true;
}

bool CBoxAlgorithmROCCurve::computeROCCurves()
{
	//Now we assiociate all values to the corresponding label
	std::sort(m_oStimulationTimeline.begin(), m_oStimulationTimeline.end(), compareCTimelineStimulationPair);//ensure the timeline is ok
	std::vector< CTimestampLabelPair >::iterator m_oBound;

	for(size_t i = 0; i < m_oValueTimeline.size(); ++i)
	{
		CTimestampValuesPair& l_rValuePair = m_oValueTimeline[i];
		m_oBound = std::lower_bound(m_oStimulationTimeline.begin(), m_oStimulationTimeline.end(), l_rValuePair, compareValueAndStimulationTimelinePair);
		if(m_oBound != m_oStimulationTimeline.begin())
		{
			--m_oBound;
			m_oLabelValueList.push_back(CLabelValuesPair(m_oBound->second, l_rValuePair.second));
		}
		else{
			//Impossible to find the corresponding stimulation
			this->getLogManager() << LogLevel_Warning << "A result of classification cannot be connected to a class. The result will be discarded.\n";
		}
	}

	//We cannot use the set because we need the correct order
	for(size_t i = 2; i < this->getStaticBoxContext().getSettingCount() ; ++i)
	{
		CIdentifier l_oClassLabel(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i));
		computeOneROCCurve(l_oClassLabel, i-2);
	}
	//Now we ask to the current page to draw itself
	const gint l_iCurrentPage = gtk_notebook_current_page(GTK_NOTEBOOK(m_pWidget));
	if(l_iCurrentPage < 0)
	{
		this->getLogManager() << LogLevel_Trace << "No page is selected. The designer is probably in no visualisation mode. Skipping the drawing phase\n";
	}
	else
	{
		m_oDrawerList[l_iCurrentPage]->forceRedraw();
	}
	return true;
}

bool CBoxAlgorithmROCCurve::computeOneROCCurve(const CIdentifier& rClassIdentifier, uint32 ui32ClassIndex)
{
	std::vector < CRocPairValue > l_oRocPairValueList;
	for(std::vector< CLabelValuesPair >::iterator it = m_oLabelValueList.begin(); it != m_oLabelValueList.end(); ++it)
	{
		CRocPairValue l_oRocPairValue;

		l_oRocPairValue.first = ((*it).first == rClassIdentifier.toUInteger());
		l_oRocPairValue.second = (*it).second[ui32ClassIndex];
		l_oRocPairValueList.push_back(l_oRocPairValue);
	}
	std::sort(l_oRocPairValueList.begin(), l_oRocPairValueList.end(), compareRocValuePair);

	uint32 l_ui32TruePositive = 0;
	uint32 l_ui32FalsePositive = 0;

	const uint32 l_ui32PositiveCount = std::count_if(l_oRocPairValueList.begin(), l_oRocPairValueList.end(), isPositive);
	const uint32 l_ui32NegativeCount = l_oRocPairValueList.size() - l_ui32PositiveCount;

	std::vector < CCoordinate >& l_oCoordinateVector = m_oDrawerList[ui32ClassIndex]->getCoordinateVector();

	for(size_t i = 0; i < l_oRocPairValueList.size(); ++i)
	{
		l_oRocPairValueList[i].first ? ++l_ui32TruePositive : ++l_ui32FalsePositive;

		l_oCoordinateVector.push_back(CCoordinate(((float64)l_ui32FalsePositive) / l_ui32NegativeCount,
												  ((float64)l_ui32TruePositive) / l_ui32PositiveCount));
	}
	m_oDrawerList[ui32ClassIndex]->generateCurve();

	return true;
}

#endif

