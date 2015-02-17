
#include "ovpCSignalDisplay.h"

#include <cmath>
#include <iostream>
#include <cstdlib>

#include <openvibe/ovITimeArithmetics.h>
#include <system/ovCTime.h>

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
			//@fixme should use signal decoder for a signal and really use the sampling rate from that
			m_oStreamedMatrixDecoder.initialize(*this,0);
			m_oStimulationDecoder.initialize(*this,1);

			m_pBufferDatabase = new CBufferDatabase(*this);

			//retrieve settings
			CIdentifier l_oDisplayMode                   = (uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			CIdentifier l_oScalingMode                   = (uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
			m_f64RefreshInterval                         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			float64 l_f64VerticalScale                   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			float64 l_f64VerticalOffset                  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
			float64 l_f64TimeScale                       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
			boolean l_bHorizontalRuler                   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
			boolean l_bVerticalRuler                     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
			boolean l_bMultiview                         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);

			if(m_f64RefreshInterval<0) {
				m_f64RefreshInterval = 0;
			}
			if(l_f64TimeScale<0.01) {
				l_f64TimeScale = 0.01;
			}

			this->getLogManager() << LogLevel_Debug << "l_sVerticalScale=" << l_f64VerticalScale << ", offset " << l_f64VerticalOffset << "\n";
			this->getLogManager() << LogLevel_Trace << "l_sScalingMode=" << l_oScalingMode << "\n";

			//create GUI
			m_pSignalDisplayView = new CSignalDisplayView(
				*m_pBufferDatabase,
				l_oDisplayMode,
				l_oScalingMode,
				l_f64VerticalScale,
				l_f64VerticalOffset,
				l_f64TimeScale,
				l_bHorizontalRuler,
				l_bVerticalRuler,
				l_bMultiview
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

#ifdef DEBUG
			uint64 in = System::Time::zgetTime();
#endif

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

#ifdef DEBUG
					static int count = 0; 
					std::cout << "Push chunk " << (count++) << " at " << 	l_pDynamicBoxContext->getInputChunkStartTime(0,c) << "\n";
#endif

					bool l_bReturnValue = m_pBufferDatabase->setMatrixBuffer(l_pMatrix->getBuffer(),
						l_pDynamicBoxContext->getInputChunkStartTime(0,c),
						l_pDynamicBoxContext->getInputChunkEndTime(0,c));
					if(!l_bReturnValue) 
					{
						return false;
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

#ifdef DEBUG
			out = System::Time::zgetTime();
			std::cout << "Elapsed1 " << out-in << "\n";
#endif

			return true;
		}
	};
};


