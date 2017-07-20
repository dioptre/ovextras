
#if defined(TARGET_HAS_ThirdPartyGTK)

#include "ovpCBoxAlgorithmKappaCoefficient.h"
#include "../algorithms/ovpCAlgorithmConfusionMatrix.h"

#include <xml/IXMLHandler.h>
#include <map>
#include <sstream>
#include <vector>
#include <iomanip>

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Evaluation;

namespace{
	const uint32 c_ui32ClassLabelOffset = 1;
}

bool CBoxAlgorithmKappaCoefficient::initialize(void)
{
	//Initialize input/output
	m_oTargetStimulationDecoder.initialize(*this, 0);
	m_oClassifierStimulationDecoder.initialize(*this, 1);

	m_oOutputMatrixEncoder.initialize(*this, 0);

	//Confusion matrix algorithm
	m_pConfusionMatrixAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConfusionMatrix));
	m_pConfusionMatrixAlgorithm->initialize();

	TParameterHandler<bool> ip_bPercentages(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage));
	ip_bPercentages = false;

	TParameterHandler<bool> ip_bSums(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums));
	ip_bSums = true;

	m_ui32AmountClass=getBoxAlgorithmContext()->getStaticBoxContext()->getSettingCount() - c_ui32ClassLabelOffset;
	vector < uint64 > l_vClassCodes;
	l_vClassCodes.resize(m_ui32AmountClass);
	for(uint32 i = 0; i< m_ui32AmountClass; i++)
	{
		CString l_sClassValue;
		getStaticBoxContext().getSettingValue(i+c_ui32ClassLabelOffset, l_sClassValue); // classes are settings from 2 to n
		l_vClassCodes[i] =(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i+c_ui32ClassLabelOffset);
	}

	// Let's check that each identifier is unique
	for(uint32 i = 0; i< m_ui32AmountClass; i++)
	{
		for(uint32 j = i+1; j< m_ui32AmountClass; j++)
		{
			if(l_vClassCodes[i] == l_vClassCodes[j])
			{
				CString l_sClassValue;
				getStaticBoxContext().getSettingValue(i + c_ui32ClassLabelOffset, l_sClassValue);
				getLogManager() << LogLevel_Error << "You must use unique classes to compute a Kappa coefficient. Class "<<i+1<<" and "<<j+1<< " are the same ("<<l_sClassValue.toASCIIString()<<").\n";
				return false;
			}
		}
	}

	TParameterHandler < OpenViBE::IStimulationSet* > ip_pClassesCodes(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes));
	for(uint32 i = 0 ; i<l_vClassCodes.size(); i++)
	{
		ip_pClassesCodes->appendStimulation(l_vClassCodes[i],0,0);
	}

	//Link all input/output
	TParameterHandler < IStimulationSet* > ip_pClassifierStimulationSet(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet));
	ip_pClassifierStimulationSet.setReferenceTarget(m_oClassifierStimulationDecoder.getOutputStimulationSet());

	TParameterHandler < IStimulationSet* > ip_pTargetStimulationSet(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet));
	ip_pTargetStimulationSet.setReferenceTarget(m_oTargetStimulationDecoder.getOutputStimulationSet());

	op_pConfusionMatrix.initialize(m_pConfusionMatrixAlgorithm->getOutputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix));

	::GtkTable* l_pTable = GTK_TABLE(gtk_table_new(2, 1, false));

	m_pKappaLabel = gtk_label_new("x");
	gtk_table_attach(
		l_pTable, m_pKappaLabel,
		0, 1, 0, 5,
		(::GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
		(::GtkAttachOptions)(GTK_EXPAND|GTK_FILL),
		0, 0);


	m_visualizationContext = dynamic_cast<OpenViBEVisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationContext));
	m_visualizationContext->setWidget(*this, GTK_WIDGET(l_pTable));

	PangoContext *l_pPangoContext = gtk_widget_get_pango_context(GTK_WIDGET(m_pKappaLabel));
	PangoFontDescription *l_pFontDescription = pango_context_get_font_description(l_pPangoContext);
	pango_font_description_set_size(l_pFontDescription, 40 * PANGO_SCALE);
	gtk_widget_modify_font(m_pKappaLabel, l_pFontDescription);

	return true;
}

bool CBoxAlgorithmKappaCoefficient::uninitialize(void)
{
	//Log for the automatic test
	this->getLogManager() << LogLevel_Info << "Final value of Kappa " << m_f64KappaCoefficient << "\n";
	m_pConfusionMatrixAlgorithm->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pConfusionMatrixAlgorithm);

	m_oOutputMatrixEncoder.uninitialize();
	m_oTargetStimulationDecoder.uninitialize();
	m_oClassifierStimulationDecoder.uninitialize();

	this->releasePluginObject(m_visualizationContext);

	return true;
}


bool CBoxAlgorithmKappaCoefficient::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

bool CBoxAlgorithmKappaCoefficient::process(void)
{
	IBoxIO& l_rDynamicBoxContext = this->getDynamicBoxContext();

	//Input 0: Targets
	for(uint32 i = 0; i < l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oTargetStimulationDecoder.decode(i);

		if(m_oTargetStimulationDecoder.isHeaderReceived())
		{
			m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget);

			m_oOutputMatrixEncoder.getInputMatrix()->setDimensionCount(1);
			m_oOutputMatrixEncoder.getInputMatrix()->setDimensionLabel(0, 0, "Kappa");
			m_oOutputMatrixEncoder.getInputMatrix()->setDimensionSize(0, 1);

			m_oOutputMatrixEncoder.encodeHeader();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			m_ui64CurrentProcessingTimeLimit = 0;
		}

		if(m_oTargetStimulationDecoder.isBufferReceived())
		{
			uint64 l_ui64ChunkEndTime = l_rDynamicBoxContext.getInputChunkEndTime(0, i);
			m_ui64CurrentProcessingTimeLimit = (l_ui64ChunkEndTime>m_ui64CurrentProcessingTimeLimit?l_ui64ChunkEndTime:m_ui64CurrentProcessingTimeLimit);
			m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget);
		}

		if(m_oTargetStimulationDecoder.isEndReceived())
		{
			m_oOutputMatrixEncoder.encodeEnd();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

	}

	//Input 1: Classifier results
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		uint64 l_ui64ChunkEndTime = l_rDynamicBoxContext.getInputChunkEndTime(1,i);
		if(l_ui64ChunkEndTime <= m_ui64CurrentProcessingTimeLimit)
		{
			m_oClassifierStimulationDecoder.decode(i);

			if(m_oClassifierStimulationDecoder.isHeaderReceived())
			{
				m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier);
			}

			if(m_oClassifierStimulationDecoder.isBufferReceived())
			{
				m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier);
				if(m_pConfusionMatrixAlgorithm->isOutputTriggerActive(OVP_Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed))
				{
					//The confusion matrix has changed so we need to update the kappa coefficient
					float64* l_pConfusionMatrix = op_pConfusionMatrix->getBuffer();
					//First we need the amount of sample that have been classified
					uint32 l_ui32Total = static_cast<uint32>(l_pConfusionMatrix[(m_ui32AmountClass+1) * (m_ui32AmountClass + 1) -1]);

					//Now we gonna compute the two sum we need to compute the kappa coefficient
					//It's more easy to use a double loop
					float64 l_f64ObservedAccurancy = 0;
					float64 l_f64ExpectedAccurancy = 0;

					for(size_t j =0; j < m_ui32AmountClass ; ++j)
					{
						//We need to take the column sum in account
						l_f64ObservedAccurancy += l_pConfusionMatrix[j*(m_ui32AmountClass+1) + j];

						l_f64ExpectedAccurancy += (l_pConfusionMatrix[(m_ui32AmountClass+1)* j + m_ui32AmountClass] *
												   l_pConfusionMatrix[(m_ui32AmountClass+1) * m_ui32AmountClass + j]);
					}
					l_f64ObservedAccurancy /= l_ui32Total;
					l_f64ExpectedAccurancy /= (l_ui32Total * l_ui32Total);

					m_f64KappaCoefficient = (l_f64ObservedAccurancy - l_f64ExpectedAccurancy)/(1 - l_f64ExpectedAccurancy);

					updateKappaValue();
					m_oOutputMatrixEncoder.getInputMatrix()->getBuffer()[0] = m_f64KappaCoefficient;
					m_oOutputMatrixEncoder.encodeBuffer();
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}
			}

			if(m_oClassifierStimulationDecoder.isEndReceived())
			{
				m_oOutputMatrixEncoder.encodeEnd();
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			}

			l_rDynamicBoxContext.markInputAsDeprecated(1, i);
		}
	}

	return true;
}

void CBoxAlgorithmKappaCoefficient::updateKappaValue()
{
	std::stringstream l_sStream;
	l_sStream << std::fixed;
	l_sStream << std::setprecision(2);
	l_sStream << m_f64KappaCoefficient;
	gtk_label_set(GTK_LABEL(m_pKappaLabel), l_sStream.str().c_str());
}


#endif

