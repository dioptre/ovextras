
// @todo for clarity, the StimulusSender related code blocks should be pushed inside the class and away from here

#include "ovpCBoxAlgorithmP300SpellerVisualisation2.h"

#include <system/ovCMemory.h>

#include <list>
#include <vector>
#include <string>
#include <algorithm>

#include <tcptagging/IStimulusSender.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

namespace
{
	class _AutoCast_
	{
	public:
		_AutoCast_(const IBox& rBox, IConfigurationManager& rConfigurationManager, const uint32 ui32Index)
			: m_rConfigurationManager(rConfigurationManager)
		{
			rBox.getSettingValue(ui32Index, m_sSettingValue);
			m_sSettingValue = m_rConfigurationManager.expand(m_sSettingValue);
		}
		operator ::GdkColor (void)
		{
			::GdkColor l_oColor;
			int r=0, g=0, b=0;
			sscanf(m_sSettingValue.toASCIIString(), "%i,%i,%i", &r, &g, &b);
			l_oColor.pixel=0;
			l_oColor.red=(r*65535)/100;
			l_oColor.green=(g*65535)/100;
			l_oColor.blue=(b*65535)/100;
			return l_oColor;
		}
	protected:
		IConfigurationManager& m_rConfigurationManager;
		CString m_sSettingValue;
	};

	static void toggle_button_show_hide_cb(::GtkToggleToolButton* pToggleButton, gpointer pUserData)
	{
		if(gtk_toggle_tool_button_get_active(pToggleButton))
		{
			gtk_widget_show(GTK_WIDGET(pUserData));
		}
		else
		{
			gtk_widget_hide(GTK_WIDGET(pUserData));
		}
	}
};

// This callback flushes all accumulated stimulations to the TCP Tagging 
// after the rendering has completed.
static gboolean flush_callback(gpointer pUserData)
{
	((CBoxAlgorithmP300SpellerVisualisation2*)pUserData)->flushQueue();
	
	return false;	// Only run once
}

bool CBoxAlgorithmP300SpellerVisualisation2::initialize(void)
{
	const IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_pMainWidgetInterface=NULL;
	m_pToolbarWidgetInterface=NULL;
	m_pFlashFontDescription=NULL;
	m_pNoFlashFontDescription=NULL;
	m_pTargetFontDescription=NULL;
	m_pSelectedFontDescription=NULL;

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	m_sInterfaceFilename         =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64RowStimulationBase     =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64ColumnStimulationBase  =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	m_oFlashBackgroundColor      =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 3);
	m_oFlashForegroundColor      =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 4);
	m_ui64FlashFontSize          =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
	m_oNoFlashBackgroundColor    =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 6);
	m_oNoFlashForegroundColor    =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 7);
	m_ui64NoFlashFontSize        =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
	m_oTargetBackgroundColor     =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 9);
	m_oTargetForegroundColor     =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 10);
	m_ui64TargetFontSize         =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);
	m_oSelectedBackgroundColor   =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 12);
	m_oSelectedForegroundColor   =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 13);
	m_ui64SelectedFontSize       =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 14);

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	m_oFlashGroupDecoder.initialize(*this, 0);
	m_oTimelineDecoder.initialize(*this, 1);
	m_oSelectionDecoder.initialize(*this, 2);

	m_visualizationContext = NULL;
	m_pStimulusSender = NULL;

	m_uiIdleFuncTag = 0;
	m_vStimuliQueue.clear();

	m_pMainWidgetInterface=gtk_builder_new(); // glade_xml_new(m_sInterfaceFilename.toASCIIString(), "p300-speller-main", NULL);
	if(!gtk_builder_add_from_file(m_pMainWidgetInterface, m_sInterfaceFilename.toASCIIString(), NULL))
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Could not load interface file [" << m_sInterfaceFilename << "]\n";
		this->getLogManager() << LogLevel_ImportantWarning << "The file may be missing. However, the interface files now use gtk-builder instead of glade. Did you update your files ?\n";
		return false;
	}

	m_pToolbarWidgetInterface=gtk_builder_new(); // glade_xml_new(m_sInterfaceFilename.toASCIIString(), "p300-speller-toolbar", NULL);
	gtk_builder_add_from_file(m_pToolbarWidgetInterface, m_sInterfaceFilename.toASCIIString(), NULL);

	m_pMainWindow=GTK_WIDGET(gtk_builder_get_object(m_pMainWidgetInterface, "p300-speller-main"));
	m_pToolbarWidget=GTK_WIDGET(gtk_builder_get_object(m_pToolbarWidgetInterface, "p300-speller-toolbar"));
	m_pTable=GTK_TABLE(gtk_builder_get_object(m_pMainWidgetInterface, "p300-speller-table"));
	m_pResult=GTK_LABEL(gtk_builder_get_object(m_pMainWidgetInterface, "label-result"));
	m_pTarget=GTK_LABEL(gtk_builder_get_object(m_pMainWidgetInterface, "label-target"));

	gtk_builder_connect_signals(m_pMainWidgetInterface, NULL);
	gtk_builder_connect_signals(m_pToolbarWidgetInterface, NULL);

	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_target_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-target"));
	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_target_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-target-title"));
	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_result_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-result"));
	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_result_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-result-title"));

	m_visualizationContext = dynamic_cast<OpenViBEVisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationContext));
	m_visualizationContext->setWidget(*this, m_pMainWindow);
	m_visualizationContext->setToolbar(*this, m_pToolbarWidget);

	guint l_uiRowCount=0;
	guint l_uiColumnCount=0;
	g_object_get(m_pTable, "n-rows", &l_uiRowCount, NULL);
	g_object_get(m_pTable, "n-columns", &l_uiColumnCount, NULL);

	m_ui64RowCount=l_uiRowCount;
	m_ui64ColumnCount=l_uiColumnCount;

	::PangoFontDescription* l_pMaxFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pFlashFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pNoFlashFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pTargetFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pSelectedFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));

	uint64 l_ui64MaxSize=0;
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64FlashFontSize);
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64NoFlashFontSize);
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64TargetFontSize);
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64SelectedFontSize);

	pango_font_description_set_size(l_pMaxFontDescription, (gint)(l_ui64MaxSize * PANGO_SCALE));
	pango_font_description_set_size(m_pFlashFontDescription, (gint)(m_ui64FlashFontSize * PANGO_SCALE));
	pango_font_description_set_size(m_pNoFlashFontDescription, (gint)(m_ui64NoFlashFontSize * PANGO_SCALE));
	pango_font_description_set_size(m_pTargetFontDescription, (gint)(m_ui64TargetFontSize * PANGO_SCALE));
	pango_font_description_set_size(m_pSelectedFontDescription, (gint)(m_ui64SelectedFontSize * PANGO_SCALE));

	this->_cache_build_from_table_(m_pTable);
	this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_background_cb_, &m_oNoFlashBackgroundColor);
	this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_foreground_cb_, &m_oNoFlashForegroundColor);
	this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_font_cb_, l_pMaxFontDescription);

	pango_font_description_free(l_pMaxFontDescription);

	m_iLastSelectedRow = -1;
	m_iLastSelectedRow = -1;
	m_iSelectedRow = -1;
	m_iSelectedColumn = -1;
	m_iTargetRow = -1;
	m_iTargetColumn = -1;

	m_sTargetText = "";
	m_sSpelledText = "";

	m_pStimulusSender = TCPTagging::createStimulusSender();

	if(!m_pStimulusSender->connect("localhost", "15361"))
	{
		this->getLogManager() << LogLevel_Warning << "Unable to connect to AS TCP Tagging, stimuli wont be forwarded.\n";
	}

	m_bTableInitialized = false;
	m_bTargetSettingMode = false;

	return true;
}

bool CBoxAlgorithmP300SpellerVisualisation2::uninitialize(void)
{
	if(m_uiIdleFuncTag)
	{
		m_vStimuliQueue.clear();
		g_source_remove(m_uiIdleFuncTag);
		m_uiIdleFuncTag = 0;
	}

	if(m_pStimulusSender)
	{
		delete m_pStimulusSender;
		m_pStimulusSender = NULL;
	}

	if(m_pSelectedFontDescription)
	{
		pango_font_description_free(m_pSelectedFontDescription);
		m_pSelectedFontDescription=NULL;
	}

	if(m_pTargetFontDescription)
	{
		pango_font_description_free(m_pTargetFontDescription);
		m_pTargetFontDescription=NULL;
	}

	if(m_pNoFlashFontDescription)
	{
		pango_font_description_free(m_pNoFlashFontDescription);
		m_pNoFlashFontDescription=NULL;
	}

	if(m_pFlashFontDescription)
	{
		pango_font_description_free(m_pFlashFontDescription);
		m_pFlashFontDescription=NULL;
	}

	if(m_pToolbarWidgetInterface)
	{
		g_object_unref(m_pToolbarWidgetInterface);
		m_pToolbarWidgetInterface=NULL;
	}

	if(m_pMainWidgetInterface)
	{
		g_object_unref(m_pMainWidgetInterface);
		m_pMainWidgetInterface=NULL;
	}

	m_oFlashGroupDecoder.uninitialize();
	m_oTimelineDecoder.uninitialize();
	m_oSelectionDecoder.uninitialize();

	if (m_visualizationContext)
	{
		this->releasePluginObject(m_visualizationContext);
	}

	while (m_vFlashGroup.size() > 0)
	{
		CMatrix* tmp = m_vFlashGroup.front();
		m_vFlashGroup.pop();
		delete tmp;
	}

	return true;
}

uint64_t CBoxAlgorithmP300SpellerVisualisation2::getClockFrequency(void)
{
	return 128LL << 32; // the box clock frequency
}

// This box attempts to run more smoothly by being called also when no current input
bool CBoxAlgorithmP300SpellerVisualisation2::processClock(IMessageClock& rMessageClock)
{
	OV_ERROR_UNLESS_KRF(getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(),
		"Failed to mark clock algorithm as ready to process",
		ErrorType::Internal);
	return true;
}

bool CBoxAlgorithmP300SpellerVisualisation2::processInput(uint32 ui32Index)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	if(!m_bTableInitialized)
	{
		this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_background_cb_, &m_oNoFlashBackgroundColor);
		this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_foreground_cb_, &m_oNoFlashForegroundColor);
		this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_font_cb_, m_pNoFlashFontDescription);
		m_bTableInitialized=true;
	}

	return true;
}

bool CBoxAlgorithmP300SpellerVisualisation2::flashNow(const CMatrix& oFlashGroup)
{
	// loop over the group here
	for (uint64 l_iRow = 0; l_iRow < m_ui64RowCount; l_iRow++)
	{
		for (uint64 l_iColumn = 0; l_iColumn < m_ui64ColumnCount; l_iColumn++)
		{
			std::pair< int, int > cacheIdx((int)l_iRow, (int)l_iColumn);
			SWidgetStyle& thisWidget = m_vCache[cacheIdx]; // thisWidget needs to be a ref for changes to have an effect

			const uint64 vectorIdx = l_iRow*m_ui64ColumnCount + l_iColumn; // The vector will have rows-first indexing, left to right
			const float64* l_pBuffer = oFlashGroup.getBuffer();
			if (l_pBuffer[vectorIdx] != 0)
			{
				// Flash this
				this->_cache_change_background_cb_(thisWidget, &m_oFlashBackgroundColor);
				this->_cache_change_foreground_cb_(thisWidget, &m_oFlashForegroundColor);
				this->_cache_change_font_cb_(thisWidget, m_pFlashFontDescription);
			}
			else
			{
				// Don't flash
				this->_cache_change_background_cb_(thisWidget, &m_oNoFlashBackgroundColor);
				this->_cache_change_foreground_cb_(thisWidget, &m_oNoFlashForegroundColor);
				this->_cache_change_font_cb_(thisWidget, m_pNoFlashFontDescription);
			}
		}
	}

	return true;
}

bool CBoxAlgorithmP300SpellerVisualisation2::setSelection(uint64 ui64StimulationIdentifier, bool isMultiCharacter)
{
	if (ui64StimulationIdentifier == OVTK_StimulationId_Target && isMultiCharacter)
	{
		// Beginning of text specification
		m_sSpelledText = "";
		m_iSelectedRow = -1;
		m_iSelectedColumn = -1;
	}
	else if (ui64StimulationIdentifier == OVTK_StimulationId_NonTarget || !isMultiCharacter)
	{
		// this->getLogManager() << LogLevel_Info << "Setting " << m_sSpelledText.c_str() << "\n";

		// End of text specification
		gtk_label_set_markup(m_pResult, m_sSpelledText.c_str());
		// Highlight last letter
		const std::pair< int, int > cacheIdx((int)m_iLastSelectedRow, (int)m_iLastSelectedColumn);
		// no need to test, we've returned false already if the above indexes are outside table

		SWidgetStyle& selectedWidget = m_vCache[cacheIdx];

		// Highlight the last letter
		_cache_change_background_cb_(selectedWidget, &m_oSelectedBackgroundColor);
		_cache_change_foreground_cb_(selectedWidget, &m_oSelectedForegroundColor);
		_cache_change_font_cb_(selectedWidget, m_pSelectedFontDescription);

		m_iSelectedRow = -1;
		m_iSelectedColumn = -1;
	}
	else if (ui64StimulationIdentifier >= m_ui64RowStimulationBase && ui64StimulationIdentifier < m_ui64RowStimulationBase + m_ui64RowCount)
	{
		m_iSelectedRow = (int)(ui64StimulationIdentifier - m_ui64RowStimulationBase);
		this->getLogManager() << LogLevel_Debug << "Received Selected Row " << ui64StimulationIdentifier 
			<< "->" << m_iSelectedRow << "\n";

	}
	else if (ui64StimulationIdentifier >= m_ui64ColumnStimulationBase && ui64StimulationIdentifier < m_ui64ColumnStimulationBase + m_ui64ColumnCount)
	{
		m_iSelectedColumn = (int)(ui64StimulationIdentifier - m_ui64ColumnStimulationBase);
		this->getLogManager() << LogLevel_Debug << "Received Selected Column " << ui64StimulationIdentifier
			<< "->" << m_iSelectedColumn << "\n";
	}
	else
	{
		this->getLogManager() << LogLevel_Warning << "Unrecognized " << ui64StimulationIdentifier << "\n";
	}

	// did we get both?
	if (m_iSelectedRow >= 0 && m_iSelectedColumn >= 0)
	{
		this->getLogManager() << LogLevel_Debug << "Displays Selected Cell\n";

		const std::pair< int, int > cacheIdx((int)m_iSelectedRow, (int)m_iSelectedColumn);

		if (m_vCache.count(cacheIdx) == 0)
		{
			this->getLogManager() << LogLevel_Error << "Indexes " << m_iSelectedRow << "," <<
				m_iSelectedColumn << " is outside the keyboard\n";
			return false;
		}

		SWidgetStyle& selectedWidget = m_vCache[cacheIdx];

		// Write the letter out
		std::string l_sLetter = gtk_label_get_text(GTK_LABEL(selectedWidget.pChildWidget));
		bool l_bCorrect = true;
		if (l_bCorrect)
		{
			m_sSpelledText += "<span color=\"darkgreen\">" + l_sLetter + "</span>";
		}

		m_iLastSelectedRow = m_iSelectedRow;
		m_iLastSelectedColumn = m_iSelectedColumn;
		m_iSelectedRow = -1;
		m_iSelectedColumn = -1;
	}

	return true;
}

bool CBoxAlgorithmP300SpellerVisualisation2::process(void)
{
	IBoxIO& l_rDynamicBoxContext = this->getDynamicBoxContext();

	const uint64 l_ui64CurrentTime = getPlayerContext().getCurrentTime();

	// --- Read flash group
	for (uint32 i = 0; i < l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oFlashGroupDecoder.decode(i);

		if (m_oFlashGroupDecoder.isBufferReceived())
		{
			CMatrix* l_oMatrix = new CMatrix();
			OpenViBEToolkit::Tools::Matrix::copy(*l_oMatrix, *m_oFlashGroupDecoder.getOutputMatrix());
			m_vFlashGroup.push(l_oMatrix);
		}
	}

	// --- Read flash trigger
	for (uint32 i = 0; i < l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		m_oTimelineDecoder.decode(i);

		if (m_oTimelineDecoder.isBufferReceived())
		{
			const IStimulationSet* oSet = m_oTimelineDecoder.getOutputStimulationSet();
			for (uint32 j = 0; j < oSet->getStimulationCount(); j++)
			{
				const uint64 l_ui64StimulationIdentifier = oSet->getStimulationIdentifier(j);
				if (l_ui64StimulationIdentifier == OVTK_StimulationId_VisualStimulationStart)
				{
					this->getLogManager() << LogLevel_Debug << "Received OVTK_StimulationId_VisualStimulationStart - flashes\n";
					m_vNextFlashTime.push(oSet->getStimulationDate(j));
				}
				else if (l_ui64StimulationIdentifier == OVTK_StimulationId_VisualStimulationStop)
				{
					this->getLogManager() << LogLevel_Debug << "Received OVTK_StimulationId_VisualStimulationStop - resets grid\n";
					m_vNextFlashStopTime.push(oSet->getStimulationDate(j));
				}
				else if (l_ui64StimulationIdentifier == OVTK_StimulationId_Reset)
				{
					this->getLogManager() << LogLevel_Debug << "Received OVTK_StimulationId_Reset - clears text\n";
					m_sTargetText = "";
					m_sSpelledText = "";
					gtk_label_set_text(m_pTarget, "");
					gtk_label_set_text(m_pResult, "");
					m_vStimuliQueue.push_back(l_ui64StimulationIdentifier); // delay until rendered
				}
				else if (l_ui64StimulationIdentifier == OVTK_StimulationId_Target)
				{
					m_bTargetSettingMode = true;
					m_sTargetText = "";
					m_pStimulusSender->sendStimulation(l_ui64StimulationIdentifier); // pass immediately
				}
				else if (l_ui64StimulationIdentifier == OVTK_StimulationId_NonTarget)
				{
					m_bTargetSettingMode = false;
					m_pStimulusSender->sendStimulation(l_ui64StimulationIdentifier); // pass immediately
				}
				else if (l_ui64StimulationIdentifier >= m_ui64RowStimulationBase &&  l_ui64StimulationIdentifier < m_ui64RowStimulationBase + m_ui64RowCount)
				{
					if (m_bTargetSettingMode)
					{
						m_iTargetRow = int(l_ui64StimulationIdentifier - m_ui64RowStimulationBase);
					}
					m_pStimulusSender->sendStimulation(l_ui64StimulationIdentifier); // pass immediately
				} 
				else if (l_ui64StimulationIdentifier >= m_ui64ColumnStimulationBase &&  l_ui64StimulationIdentifier < m_ui64ColumnStimulationBase + m_ui64ColumnCount)
				{
					if (m_bTargetSettingMode)
					{
						m_iTargetColumn = int(l_ui64StimulationIdentifier - m_ui64ColumnStimulationBase);
					}
					m_pStimulusSender->sendStimulation(l_ui64StimulationIdentifier); // pass immediately
				}
				else
				{
					m_pStimulusSender->sendStimulation(l_ui64StimulationIdentifier); // pass immediately
				}
				
				// Set a target letter
				if (m_iTargetColumn != -1 && m_iTargetRow != -1)
				{
					std::pair<int, int> idx(m_iTargetRow, m_iTargetColumn);

					m_sTargetText += gtk_label_get_text(GTK_LABEL(m_vCache[idx].pChildWidget));

					gchar *escaped = g_markup_escape_text(m_sTargetText.c_str(),-1);
					gtk_label_set_text(m_pTarget, escaped);
					g_free(escaped);

				//	this->getLogManager() << LogLevel_Info << "Target so far" << m_sTargetText.c_str() << "\n";

					m_iTargetColumn = -1;
					m_iTargetRow = -1;
				}
			}
		}
	}

	// --- Selection stimulations
	for (uint32 i = 0; i < l_rDynamicBoxContext.getInputChunkCount(2); i++)
	{
		m_oSelectionDecoder.decode(i);

		if (m_oSelectionDecoder.isBufferReceived())
		{
			IStimulationSet* oSet = m_oSelectionDecoder.getOutputStimulationSet();
			const bool l_bIsMultiCharacter = (oSet->getStimulationCount()!=2);  // If its a single character, its rows+cols. Otherwise its Target+[row,col,row,col,...]+NonTarget
			for (uint32 j = 0; j < oSet->getStimulationCount(); j++)
			{
				uint64 l_ui64StimulationIdentifier = oSet->getStimulationIdentifier(j);
				if (!setSelection(l_ui64StimulationIdentifier,
					l_bIsMultiCharacter)) {
					return false;
				}
			}
		}
	}

	// If we have everything we need, lets flash
	if (m_vNextFlashTime.size() > 0 && m_vNextFlashTime.front() <= l_ui64CurrentTime)
	{
		if (m_vFlashGroup.size() > 0)
		{
			// this->getLogManager() << LogLevel_Info << "Doing flash\n";
			CMatrix* l_pFlashGroup = m_vFlashGroup.front();
			flashNow(*l_pFlashGroup);
			m_vNextFlashTime.pop();
			m_vFlashGroup.pop();
			delete l_pFlashGroup;
			m_vStimuliQueue.push_back(OVTK_StimulationId_VisualStimulationStart); // delay until rendered
		}
		else
		{
			getLogManager() << LogLevel_Warning << "Should flash but no group received yet\n";
		}
	}
	
	// Should we clear?
	if (m_vNextFlashStopTime.size() > 0 && m_vNextFlashStopTime.front() <= l_ui64CurrentTime)
	{
		// this->getLogManager() << LogLevel_Info << "Doing clear\n";

		this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_background_cb_, &m_oNoFlashBackgroundColor);
		this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_foreground_cb_, &m_oNoFlashForegroundColor);
		this->_cache_for_each_(&CBoxAlgorithmP300SpellerVisualisation2::_cache_change_font_cb_, m_pNoFlashFontDescription);

		m_vNextFlashStopTime.pop();
		m_vStimuliQueue.push_back(OVTK_StimulationId_VisualStimulationStop); // delay until rendered
	}

	// After any possible rendering, we flush the accumulated stimuli. The default idle func is low priority, so it should be run after rendering by gtk.
	if (m_uiIdleFuncTag == 0)
	{
		m_uiIdleFuncTag = g_idle_add(flush_callback, this);
	}

	return true;
}

// _________________________________________________________________________________________________________________________________________________________
//

void CBoxAlgorithmP300SpellerVisualisation2::_cache_build_from_table_(::GtkTable* pTable)
{
	m_vCache.clear();

	if(pTable)
	{
		::GdkColor l_oWhite;
		l_oWhite.red=65535;
		l_oWhite.green=65535;
		l_oWhite.blue=65535;
		l_oWhite.pixel=65535;

		::GtkTableChild* l_pTableChild=NULL;
		::GList* l_pList=NULL;
		for(l_pList=pTable->children; l_pList; l_pList=l_pList->next)
		{
			l_pTableChild=(::GtkTableChild*)l_pList->data;

			for(unsigned long i=l_pTableChild->top_attach; i<l_pTableChild->bottom_attach; i++)
			{
				for(unsigned long j=l_pTableChild->left_attach; j<l_pTableChild->right_attach; j++)
				{
					// nb this indexing starts from 0
					std::pair<int, int> idx(i, j);

					// this->getLogManager() << LogLevel_Info << "Inserted " << uint64(i) << "," << uint64(j) << "\n";
					
					CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle l_oWidgetStyle;
					l_oWidgetStyle.pWidget=l_pTableChild->widget;
					l_oWidgetStyle.pChildWidget=gtk_bin_get_child(GTK_BIN(l_pTableChild->widget));
					l_oWidgetStyle.oBackgroundColor=l_oWhite;
					l_oWidgetStyle.oForegroundColor=l_oWhite;
					l_oWidgetStyle.pFontDescription=NULL;

					m_vCache[idx] = l_oWidgetStyle;
				}
			}
		}
	}
}

void CBoxAlgorithmP300SpellerVisualisation2::_cache_for_each_(_cache_callback_ fpCallback, void* pUserData)
{
	for(auto i=m_vCache.begin(); i!=m_vCache.end(); i++)
	{
		(this->*fpCallback)(i->second,pUserData);
	}
}

void CBoxAlgorithmP300SpellerVisualisation2::_cache_for_each_if_(int iLine, int iColumn, _cache_callback_ fpIfCallback, _cache_callback_ fpElseCallback, void* pIfUserData, void* pElseUserData)
{
	for(auto i=m_vCache.begin(); i!=m_vCache.end(); i++)
	{
		std::pair<int, int> idx = i->first;
	
		if(idx.first==iLine && idx.second==iColumn)
		{
			(this->*fpIfCallback)(i->second, pIfUserData);
		}
		else
		{
			(this->*fpElseCallback)(i->second, pElseUserData);
		}
	}
}

void CBoxAlgorithmP300SpellerVisualisation2::_cache_change_null_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData)
{
}

void CBoxAlgorithmP300SpellerVisualisation2::_cache_change_background_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData)
{
	::GdkColor oColor=*(::GdkColor*)pUserData;
	if(!System::Memory::compare(&rWidgetStyle.oBackgroundColor, &oColor, sizeof(::GdkColor)))
	{
		gtk_widget_modify_bg(rWidgetStyle.pWidget, GTK_STATE_NORMAL, &oColor);
		rWidgetStyle.oBackgroundColor=oColor;
	}
}

void CBoxAlgorithmP300SpellerVisualisation2::_cache_change_foreground_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData)
{
	::GdkColor oColor=*(::GdkColor*)pUserData;
	if(!System::Memory::compare(&rWidgetStyle.oForegroundColor, &oColor, sizeof(::GdkColor)))
	{
		gtk_widget_modify_fg(rWidgetStyle.pChildWidget, GTK_STATE_NORMAL, &oColor);
		rWidgetStyle.oForegroundColor=oColor;
	}
}

void CBoxAlgorithmP300SpellerVisualisation2::_cache_change_font_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData)
{
	::PangoFontDescription* pFontDescription=(::PangoFontDescription*)pUserData;
	if(rWidgetStyle.pFontDescription!=pFontDescription)
	{
		gtk_widget_modify_font(rWidgetStyle.pChildWidget, pFontDescription);
		rWidgetStyle.pFontDescription=pFontDescription;
	}
}

// Note that we don't need concurrency control here as gtk callbacks run in the main thread
void CBoxAlgorithmP300SpellerVisualisation2::flushQueue(void)
{
	for(size_t i=0;i<m_vStimuliQueue.size();i++)
	{
		m_pStimulusSender->sendStimulation(m_vStimuliQueue[i]);
	}
	m_vStimuliQueue.clear();

	// This function will be automatically removed after completion, so set to 0
	m_uiIdleFuncTag = 0;
}
