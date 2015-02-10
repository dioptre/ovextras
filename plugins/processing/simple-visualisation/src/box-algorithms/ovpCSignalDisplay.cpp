#include "ovpCSignalDisplay.h"

#include <cmath>
#include <iostream>
#include <cstdlib>

#include <openvibe/ovITimeArithmetics.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

using namespace OpenViBEToolkit;

using namespace std;

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		/**
		* Constructor
		*/
		CSignalDisplay::CSignalDisplay(void)
			: m_pSignalDisplayView(NULL)
			,m_pBufferDatabase(NULL)
		{
		}

		boolean CSignalDisplay::initialize()
		{
			m_oStreamedMatrixDecoder.initialize(*this,0);
			m_oStimulationDecoder.initialize(*this,1);

			m_pBufferDatabase = new CBufferDatabase(*this);

			//retrieve settings
			CString l_sTimeScaleSettingValue=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			CString l_sDisplayModeSettingValue=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
			m_f64RefreshInterval=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			if(m_f64RefreshInterval<0) {
				m_f64RefreshInterval = 0;
			}

			CString l_sManualVerticalScaleSettingValue="false";
			CString l_sVerticalScaleSettingValue="100.";
			CString l_sVerticalOffset ="0.0";
			CString l_sScalingMode = getTypeManager().getEnumerationEntryNameFromValue(OVP_TypeId_SignalDisplayMode, OVP_TypeId_SignalDisplayScaling_PerChannel);

			if(this->getStaticBoxContext().getSettingCount() > 3) l_sManualVerticalScaleSettingValue=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			if(this->getStaticBoxContext().getSettingCount() > 4) l_sVerticalScaleSettingValue=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
			if(this->getStaticBoxContext().getSettingCount() > 5) l_sVerticalOffset=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
			if(this->getStaticBoxContext().getSettingCount() > 6) l_sScalingMode=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);


			this->getLogManager() << LogLevel_Debug << "l_sManualVerticalScaleSettingValue=" << l_sManualVerticalScaleSettingValue << "\n";
			this->getLogManager() << LogLevel_Debug << "l_sVerticalScaleSettingValue=" << l_sVerticalScaleSettingValue << ", offset " << l_sVerticalOffset << "\n";
			this->getLogManager() << LogLevel_Info << "l_sScalingMode=" << l_sScalingMode << "\n";

			//create GUI
			m_pSignalDisplayView = new CSignalDisplayView(
				*m_pBufferDatabase,
				::atof(l_sTimeScaleSettingValue),
	//			l_bIsMultiview,
				CIdentifier(getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_SignalDisplayMode, l_sDisplayModeSettingValue)),
				CIdentifier(getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_SignalDisplayScaling, l_sScalingMode)),
				!this->getConfigurationManager().expandAsBoolean(l_sManualVerticalScaleSettingValue),
				::atof(l_sVerticalScaleSettingValue),
				::atof(l_sVerticalOffset)
				);

			m_pBufferDatabase->setDrawable(m_pSignalDisplayView);

			//parent visualisation box in visualisation tree
			::GtkWidget* l_pWidget=NULL;
			::GtkWidget* l_pToolbarWidget=NULL;
			dynamic_cast<CSignalDisplayView*>(m_pSignalDisplayView)->getWidgets(l_pWidget, l_pToolbarWidget);
			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(l_pWidget);
			if(l_pToolbarWidget != NULL)
			{
				getBoxAlgorithmContext()->getVisualisationContext()->setToolbar(l_pToolbarWidget);
			}

			m_ui64LastScaleRefreshTime = 0;

			return true;
		}

		boolean CSignalDisplay::uninitialize()
		{
			m_oStimulationDecoder.uninitialize();
			m_oStreamedMatrixDecoder.uninitialize();

			delete m_pSignalDisplayView;
			delete m_pBufferDatabase;
			m_pSignalDisplayView=NULL;
			m_pBufferDatabase=NULL;

			return true;
		}

		boolean CSignalDisplay::processInput(uint32 ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		boolean CSignalDisplay::process()
		{
			IDynamicBoxContext* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

			if(m_pBufferDatabase->getErrorStatus()) {
				this->getLogManager() << LogLevel_Error << "Buffer database reports an error. Its possible that the inputs given to the Signal Display are not supported by it.\n";
				return false;
			}

			// Stimulations in input 1
			for(uint32 c=0; c<l_pDynamicBoxContext->getInputChunkCount(1); c++)
			{
				m_oStimulationDecoder.decode(c);
				if(m_oStimulationDecoder.isBufferReceived())
				{
					const IStimulationSet* l_pStimulationSet = m_oStimulationDecoder.getOutputStimulationSet();
					const uint64 l_ui64StimulationCount = l_pStimulationSet->getStimulationCount();

					m_pBufferDatabase->setStimulationCount(static_cast<uint32>(l_ui64StimulationCount));

					for(uint32 s=0;s<l_ui64StimulationCount;s++)
					{
						const uint64 l_ui64StimulationIdentifier = l_pStimulationSet->getStimulationIdentifier(s);
						const uint64 l_ui64StimulationDate = l_pStimulationSet->getStimulationDate(s);
						const CString l_oStimulationName = getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, l_ui64StimulationIdentifier);

						((CSignalDisplayView*)m_pSignalDisplayView)->onStimulationReceivedCB(l_ui64StimulationIdentifier, l_oStimulationName);
						m_pBufferDatabase->setStimulation(s, l_ui64StimulationIdentifier, l_ui64StimulationDate);
					}
				}
			}

			const uint64 l_ui64TimeNow = getPlayerContext().getCurrentTime();
			if(m_ui64LastScaleRefreshTime == 0 || l_ui64TimeNow - m_ui64LastScaleRefreshTime > ITimeArithmetics::secondsToTime(m_f64RefreshInterval)) 
			{
//				this->getLogManager() << LogLevel_Info << "Refresh at " << ITimeArithmetics::timeToSeconds(l_ui64TimeNow) << "s \n";
				((CSignalDisplayView*)m_pSignalDisplayView)->refreshScale();
				m_ui64LastScaleRefreshTime = l_ui64TimeNow;
			}


			// Streamed matrix in input 0
			for(uint32 c=0; c<l_pDynamicBoxContext->getInputChunkCount(0); c++)
			{
				m_oStreamedMatrixDecoder.decode(c);
				if(m_oStreamedMatrixDecoder.isHeaderReceived())
				{
					const IMatrix* l_pMatrix = m_oStreamedMatrixDecoder.getOutputMatrix();

					m_pBufferDatabase->setMatrixDimensionCount(l_pMatrix->getDimensionCount());
					for(uint32 i=0;i<l_pMatrix->getDimensionCount();i++)
					{
						m_pBufferDatabase->setMatrixDimensionSize(i, l_pMatrix->getDimensionSize(i));
						for(uint32 j=0;j<l_pMatrix->getDimensionSize(i);j++) 
						{
							m_pBufferDatabase->setMatrixDimensionLabel(i, j, l_pMatrix->getDimensionLabel(i,j));
						}
					}
				}

				if(m_oStreamedMatrixDecoder.isBufferReceived())
				{
					const IMatrix* l_pMatrix = m_oStreamedMatrixDecoder.getOutputMatrix();

					bool l_bReturnValue = m_pBufferDatabase->setMatrixBuffer(l_pMatrix->getBuffer(),
						l_pDynamicBoxContext->getInputChunkStartTime(0,c),
						l_pDynamicBoxContext->getInputChunkEndTime(0,c));
					if(!l_bReturnValue) 
					{
						return false;
					}

				}
			}

			return true;
		}
	};
};


