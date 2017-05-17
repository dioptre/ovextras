#include "ovpCPowerSpectrumDisplay.h"

#include <cstdlib>

using namespace OpenViBE;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;
using namespace OpenViBEToolkit;

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{

		CPowerSpectrumDisplay::CPowerSpectrumDisplay() :
			m_pPowerSpectrumDisplayView(NULL),
			m_pPowerSpectrumDisplayDatabase(NULL)
		{
		}

		bool CPowerSpectrumDisplay::initialize()
		{
			m_oSpectrumDecoder.initialize(*this,0);

			m_pPowerSpectrumDisplayDatabase = new CPowerSpectrumDatabase(*this);

			//retrieve displayed frequency range settings
			CString l_sMinDisplayedFrequencySettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			CString l_sMaxDisplayedFrequencySettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			m_pPowerSpectrumDisplayView = new CPowerSpectrumDisplayView(*m_pPowerSpectrumDisplayDatabase,
				atof(l_sMinDisplayedFrequencySettingValue), atof(l_sMaxDisplayedFrequencySettingValue));

			m_pPowerSpectrumDisplayDatabase->setDrawable(m_pPowerSpectrumDisplayView);

			//parent visualisation box in visualisation tree
			::GtkWidget* l_pWidget=NULL;
			::GtkWidget* l_pToolbarWidget=NULL;
			dynamic_cast<CPowerSpectrumDisplayView*>(m_pPowerSpectrumDisplayView)->getWidgets(l_pWidget, l_pToolbarWidget);
			m_visualizationContext = dynamic_cast<OpenViBEVisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationContext));
			m_visualizationContext->setWidget(*this, l_pWidget);
			if(l_pToolbarWidget != NULL)
			{
				m_visualizationContext->setToolbar(*this, l_pToolbarWidget);
			}
			return true;
		}

		bool CPowerSpectrumDisplay::uninitialize()
		{
			m_oSpectrumDecoder.uninitialize();

			delete m_pPowerSpectrumDisplayView;
			delete m_pPowerSpectrumDisplayDatabase;
			this->releasePluginObject(m_visualizationContext);

			return true;
		}

		bool CPowerSpectrumDisplay::processInput(uint32_t ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		bool CPowerSpectrumDisplay::process()
		{
			IDynamicBoxContext* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

			for(uint32_t i=0; i<l_pDynamicBoxContext->getInputChunkCount(0); i++)
			{
				m_oSpectrumDecoder.decode(i);

				if(m_oSpectrumDecoder.isHeaderReceived())
				{
					const IMatrix *l_pMatrix = m_oSpectrumDecoder.getOutputMatrix();

					m_pPowerSpectrumDisplayDatabase->setChannelCount(l_pMatrix->getDimensionSize(0));
					for(uint32_t c=0;c<l_pMatrix->getDimensionSize(0);c++)
					{
						m_pPowerSpectrumDisplayDatabase->setChannelName(c, l_pMatrix->getDimensionLabel(0,c));
					}

					const IMatrix* frequencyAbscissaMatrix = m_oSpectrumDecoder.getOutputFrequencyAbscissa();

					m_pPowerSpectrumDisplayDatabase->setFrequencyAbscissaCount(l_pMatrix->getDimensionSize(1));
					for(uint32_t c=0;c<l_pMatrix->getDimensionSize(1);c++)
					{
						m_pPowerSpectrumDisplayDatabase->setFrequencyAbscissaName(c, l_pMatrix->getDimensionLabel(1,c));
						m_pPowerSpectrumDisplayDatabase->setFrequencyAbscissaValue(c, frequencyAbscissaMatrix->getBuffer()[c]);
					}
				}

				if(m_oSpectrumDecoder.isBufferReceived())
				{
					const IMatrix *l_pMatrix = m_oSpectrumDecoder.getOutputMatrix();

					m_pPowerSpectrumDisplayDatabase->setBuffer(l_pMatrix->getBuffer());
				}
			}

			return true;
		}
	};
};
