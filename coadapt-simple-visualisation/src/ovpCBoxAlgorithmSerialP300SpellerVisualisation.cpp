#include "ovpCBoxAlgorithmSerialP300SpellerVisualisation.h"

#include <system/Memory.h>

#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;


using namespace std;

namespace
{
	class _AutoCast_
	{
	public:
		_AutoCast_(IBox& rBox, IConfigurationManager& rConfigurationManager, const uint32 ui32Index) : m_rConfigurationManager(rConfigurationManager) { rBox.getSettingValue(ui32Index, m_sSettingValue); }
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

boolean CBoxAlgorithmSerialP300SpellerVisualisation::initialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_pMainWidgetInterface=NULL;
	m_pToolbarWidgetInterface=NULL;
	m_pFlashFontDescription=NULL;
	m_pTargetFontDescription=NULL;
	m_pCorrectSelectedFontDescription=NULL;
	m_pWrongSelectedFontDescription=NULL;

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	m_sInterfaceFilename         =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64StimulationBase     =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_oFlashBackgroundColor      =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 2);
	m_oFlashForegroundColor      =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 3);
	m_ui64FlashFontSize          =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_oTargetBackgroundColor     =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 5);
	m_oTargetForegroundColor     =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 6);
	m_ui64TargetFontSize         =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
	m_oCorrectSelectedBackgroundColor   =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 8);
	m_oCorrectSelectedForegroundColor   =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 9);
	m_ui64CorrectSelectedFontSize       =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10);
	m_oWrongSelectedBackgroundColor   =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 11);
	m_oWrongSelectedForegroundColor   =_AutoCast_(l_rStaticBoxContext, this->getConfigurationManager(), 12);
	m_ui64WrongSelectedFontSize       =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 13);
	OpenViBE::uint64 l_ui64ResultFontSize       =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 14);

	CString l_sLettersConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 15));
	uint64 l_ui32NumberOfGradientSteps = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 16);
	float64 l_fFeedbackSignificanceLevel = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 17);
	float64 l_fFeedbackRate = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 18);	
	

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	m_pSequenceStimulationDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pSequenceStimulationDecoder->initialize();

	m_pTargetStimulationDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pTargetStimulationDecoder->initialize();

	m_pTargetFlaggingStimulationEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	m_pTargetFlaggingStimulationEncoder->initialize();

	m_pSelectionStimulationDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pSelectionStimulationDecoder->initialize();

	ip_pSequenceMemoryBuffer.initialize(m_pSequenceStimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pSequenceStimulationSet.initialize(m_pSequenceStimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

	ip_pTargetMemoryBuffer.initialize(m_pTargetStimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pTargetStimulationSet.initialize(m_pTargetStimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

	ip_pTargetFlaggingStimulationSet.initialize(m_pTargetFlaggingStimulationEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
	op_pTargetFlaggingMemoryBuffer.initialize(m_pTargetFlaggingStimulationEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

	m_oAlgo1_StreamedMatrixDecoder.initialize(*this);

	m_ui64LastTime=0;

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
	m_pTable=GTK_TABLE(gtk_builder_get_object(m_pMainWidgetInterface, "p300-serial-speller-table"));
	m_pResult=GTK_LABEL(gtk_builder_get_object(m_pMainWidgetInterface, "label-result"));
	m_pTarget=GTK_LABEL(gtk_builder_get_object(m_pMainWidgetInterface, "label-target"));
	m_pCenterLabel=GTK_LABEL(gtk_builder_get_object(m_pMainWidgetInterface, "center-label"));
	//std::cout << gtk_label_get_text(m_pCenterLabel) << "\n";

	gtk_builder_connect_signals(m_pMainWidgetInterface, NULL);
	gtk_builder_connect_signals(m_pToolbarWidgetInterface, NULL);

	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_target_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-target"));
	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_target_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-target-title"));
	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_result_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-result"));
	g_signal_connect(gtk_builder_get_object(m_pToolbarWidgetInterface, "toolbutton-show_result_text"),             "toggled", G_CALLBACK(toggle_button_show_hide_cb), gtk_builder_get_object(m_pMainWidgetInterface, "label-result-title"));

	getVisualisationContext().setWidget(m_pMainWindow);
	getVisualisationContext().setToolbar(m_pToolbarWidget);

	::PangoFontDescription* l_pMaxFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pFlashFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pTargetFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pCorrectSelectedFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	m_pWrongSelectedFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));

	uint64 l_ui64MaxSize=0;
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64FlashFontSize);
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64TargetFontSize);
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64CorrectSelectedFontSize);
	l_ui64MaxSize=std::max(l_ui64MaxSize, m_ui64WrongSelectedFontSize);

	pango_font_description_set_size(l_pMaxFontDescription, l_ui64MaxSize * PANGO_SCALE);
	pango_font_description_set_size(m_pFlashFontDescription, m_ui64FlashFontSize * PANGO_SCALE);
	pango_font_description_set_size(m_pTargetFontDescription, m_ui64TargetFontSize * PANGO_SCALE);
	pango_font_description_set_size(m_pCorrectSelectedFontDescription, m_ui64CorrectSelectedFontSize * PANGO_SCALE);
	pango_font_description_set_size(m_pWrongSelectedFontDescription, m_ui64WrongSelectedFontSize * PANGO_SCALE);

	::PangoFontDescription* l_pResultFontDescription=pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_pMainWindow)));
	pango_font_description_set_size(l_pResultFontDescription, l_ui64ResultFontSize * PANGO_SCALE);
	gtk_widget_modify_font((::GtkWidget*)m_pResult, l_pResultFontDescription);
	gtk_widget_modify_font((::GtkWidget*)m_pTarget, l_pResultFontDescription);

	/*this->_cache_build_from_table_(m_pTable);
	this->_cache_for_each_(&CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_background_cb_, &m_oNoFlashBackgroundColor);
	this->_cache_for_each_(&CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_foreground_cb_, &m_oNoFlashForegroundColor);
	this->_cache_for_each_(&CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_font_cb_, l_pMaxFontDescription);*/

	//gtk_widget_modify_font((::GtkWidget*)m_pCenterLabel, m_pFlashFontDescription);
	this->_cache_change_font_cb_(m_pFlashFontDescription);
	//gtk_widget_modify_fg((::GtkWidget*)m_pCenterLabel, GTK_STATE_NORMAL, &m_oFlashForegroundColor);
	this->_cache_change_foreground_cb_(&m_oFlashForegroundColor);
	m_pCenterEventBox=GTK_EVENT_BOX(gtk_builder_get_object(m_pMainWidgetInterface, "center-event-box"));
	this->_cache_change_background_cb_(&m_oFlashBackgroundColor);
	//gtk_widget_modify_bg((::GtkWidget*)m_pCenterEventBox, GTK_STATE_NORMAL, &m_oFlashBackgroundColor);

	pango_font_description_free(l_pMaxFontDescription);

	m_iLastTarget=-1;
	m_iTarget=-1;
	m_iSelection=-1;

	m_bTableInitialized=false;

	initializeP300SymbolList(l_sLettersConfigurationFilename);

	m_pLikeliness = new CMatrix();
	m_bReceivedLikeliness = false;
	initializeColorMap(&m_oFlashForegroundColor, &m_oCorrectSelectedBackgroundColor, l_ui32NumberOfGradientSteps, l_fFeedbackSignificanceLevel, l_fFeedbackRate);

	return true;
}

void CBoxAlgorithmSerialP300SpellerVisualisation::initializeP300SymbolList(OpenViBE::CString filename)
{

	CMemoryBuffer l_pLettersConfigurationFile;
	ifstream l_oLettersFile(filename.toASCIIString(), ios::binary);

	if(l_oLettersFile.is_open())
	{
		this->getLogManager() << LogLevel_Warning << "letter conf file "<<filename << "is open\n";
		size_t l_iFileLen;
		l_oLettersFile.seekg(0, ios::end);
		l_iFileLen=l_oLettersFile.tellg();
		l_oLettersFile.seekg(0, ios::beg);

		l_pLettersConfigurationFile.setSize(l_iFileLen, true);//set size and discard true
		l_oLettersFile.read((char*)l_pLettersConfigurationFile.getDirectPointer(), l_iFileLen);
		l_oLettersFile.close();

		//
		XML::IReader* l_pReader=XML::createReader(*this);
		l_pReader->processData(l_pLettersConfigurationFile.getDirectPointer(), l_pLettersConfigurationFile.getSize());
		l_pReader->release();
		l_pReader=NULL;
		//

		this->getLogManager() << LogLevel_Warning << "loading letters conf\n";
	}			
	else
	{
		this->getLogManager() << LogLevel_Error << "Could not load letters configuration from file [" << filename << "]\n";
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::initializeColorMap(const ::GdkColor* startColor, const ::GdkColor* endColor, uint32 nSteps, float32 sigLevel, float32 logRate)
{
	int l_fRedBegin = (int)startColor->red; int l_fGreenBegin = (int)startColor->green; int l_fBlueBegin = (int)startColor->blue;
	int l_fRedEnd = (int)endColor->red; int l_fGreenEnd = (int)endColor->green; int l_fBlueEnd = (int)endColor->blue;
	float startKey = -log(sigLevel)/log(logRate);

	//std::cout << "RedBegin " << l_fRedBegin << ", RedEnd " << l_fRedEnd << ", GreenBegin " << l_fGreenBegin << ", GreenEnd " << l_fGreenEnd << ", BlueBegin " << l_fBlueBegin << ", BlueEnd " << l_fBlueEnd << "\n";

	ColorTriple l_oColorTriple;
	l_oColorTriple.color[0] = l_fRedBegin;
	l_oColorTriple.color[1] = l_fGreenBegin;
	l_oColorTriple.color[2] = l_fBlueBegin;	
	m_mColorMap.insert(std::pair<float32, ColorTriple>(1.01, l_oColorTriple));	
	for (int i=1; i<nSteps; i++)
	{
		ColorTriple l_oColorTriple;
	    	l_oColorTriple.color[0] = (guint16)(l_fRedBegin - ((l_fRedBegin-l_fRedEnd)*i)/(int)nSteps);
	    	l_oColorTriple.color[1] = (guint16)(l_fGreenBegin - ((l_fGreenBegin-l_fGreenEnd)*i)/(int)nSteps);
		//std::cout << (l_fGreenBegin-l_fGreenEnd)*i << " " << ((l_fGreenBegin-l_fGreenEnd)*i)/(int)nSteps << " " << (l_fGreenBegin - ((l_fGreenBegin-l_fGreenEnd)*i)/(int)nSteps) << "\n";
	    	l_oColorTriple.color[2] = (guint16)(l_fBlueBegin - ((l_fBlueBegin-l_fBlueEnd)*i)/(int)nSteps);	
		//std::cout << "Color key = " << pow(logRate,-(startKey+i-1)) << ", color R: " << l_oColorTriple.color[0] << ", G: " << l_oColorTriple.color[1] << ", B:" << l_oColorTriple.color[2] <<"\n";
		m_mColorMap.insert(std::pair<float32, ColorTriple>(pow(logRate,-(startKey+i-1)), l_oColorTriple));
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);

	if (CString(sName)==CString("P300SymbolList"))
	{
		m_vSymbolList.reserve(atoi(sAttributeValue[0]));
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::processChildData(const char* sData)
{
	if(m_vNode.top()==CString("Label"))
	{
		m_vSymbolList.push_back(CString(sData));
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::closeChild(void)
{
	m_vNode.pop();
}

boolean CBoxAlgorithmSerialP300SpellerVisualisation::uninitialize(void)
{
	if(m_pWrongSelectedFontDescription)
	{
		pango_font_description_free(m_pWrongSelectedFontDescription);
		m_pWrongSelectedFontDescription=NULL;
	}

	if(m_pCorrectSelectedFontDescription)
	{
		pango_font_description_free(m_pCorrectSelectedFontDescription);
		m_pCorrectSelectedFontDescription=NULL;
	}

	if(m_pTargetFontDescription)
	{
		pango_font_description_free(m_pTargetFontDescription);
		m_pTargetFontDescription=NULL;
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

	ip_pTargetFlaggingStimulationSet.uninitialize();
	op_pTargetFlaggingMemoryBuffer.uninitialize();

	op_pTargetStimulationSet.uninitialize();
	ip_pTargetMemoryBuffer.uninitialize();

	op_pSequenceStimulationSet.uninitialize();
	ip_pSequenceMemoryBuffer.uninitialize();

	if(m_pSelectionStimulationDecoder)
	{
		m_pSelectionStimulationDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pSelectionStimulationDecoder);
		m_pSelectionStimulationDecoder=NULL;
	}

	if(m_pTargetFlaggingStimulationEncoder)
	{
		m_pTargetFlaggingStimulationEncoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pTargetFlaggingStimulationEncoder);
		m_pTargetFlaggingStimulationEncoder=NULL;
	}

	if(m_pTargetStimulationDecoder)
	{
		m_pTargetStimulationDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pTargetStimulationDecoder);
		m_pTargetStimulationDecoder=NULL;
	}

	if(m_pSequenceStimulationDecoder)
	{
		m_pSequenceStimulationDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pSequenceStimulationDecoder);
		m_pSequenceStimulationDecoder=NULL;
	}

	m_oAlgo1_StreamedMatrixDecoder.uninitialize();

	delete m_pLikeliness;

	return true;
}

boolean CBoxAlgorithmSerialP300SpellerVisualisation::processInput(uint32 ui32Index)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	if(!m_bTableInitialized)
	{
		/*this->_cache_for_each_(&CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_background_cb_, &m_oNoFlashBackgroundColor);
		this->_cache_for_each_(&CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_foreground_cb_, &m_oNoFlashForegroundColor);
		this->_cache_for_each_(&CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_font_cb_, m_pNoFlashFontDescription);*/
		m_bTableInitialized=true;
	}

	return true;
}

boolean CBoxAlgorithmSerialP300SpellerVisualisation::process(void)
{
	// IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	uint32 i, j, k;

	// --- Sequence stimulations

	for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		CStimulationSet l_oFlaggingStimulationSet;

		ip_pSequenceMemoryBuffer=l_rDynamicBoxContext.getInputChunk(0, i);
		ip_pTargetFlaggingStimulationSet=&l_oFlaggingStimulationSet;
		op_pTargetFlaggingMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(0);

		m_pSequenceStimulationDecoder->process();

		m_ui64LastTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);

		if(m_pSequenceStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			m_pTargetFlaggingStimulationEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeHeader);
		}

		if(m_pSequenceStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			IStimulationSet* l_pStimulationSet=op_pSequenceStimulationSet;
			for(j=0; j<l_pStimulationSet->getStimulationCount(); j++)
			{
				uint64 l_ui64StimulationIdentifier=l_pStimulationSet->getStimulationIdentifier(j);
				boolean l_bFlash=false;
				int l_iSelectedSymbolIndex=-1;
				if(l_ui64StimulationIdentifier >= m_ui64StimulationBase)
				{
					l_iSelectedSymbolIndex=l_ui64StimulationIdentifier-m_ui64StimulationBase;
					l_bFlash=true;
					if(l_iSelectedSymbolIndex==m_iLastTarget)
					{
						l_oFlaggingStimulationSet.appendStimulation(OVTK_StimulationId_Target, l_pStimulationSet->getStimulationDate(j), 0);
					}
					else
					{
						l_oFlaggingStimulationSet.appendStimulation(OVTK_StimulationId_NonTarget, l_pStimulationSet->getStimulationDate(j), 0);
					}
				}
				if(l_ui64StimulationIdentifier == OVTK_StimulationId_VisualStimulationStop)
				{
					this->clear_screen();
				}
				if(l_ui64StimulationIdentifier == OVTK_StimulationId_Reset)
				{
					gtk_label_set_text(m_pTarget, "");
					gtk_label_set_text(m_pResult, "");
				}

				if(l_bFlash)
				{
					this->_cache_change_symbol(l_iSelectedSymbolIndex);
					if (m_bReceivedLikeliness)
					{
						map<float32, ColorTriple>::iterator l_oColorIterator = m_mColorMap.begin();
						while((1-(*(m_pLikeliness->getBuffer()+l_iSelectedSymbolIndex))) > (*l_oColorIterator).first) 
						{ 
							l_oColorIterator++; 
						}
						::GdkColor l_oGradientForegroundColor;
						l_oGradientForegroundColor.pixel = 0;
						l_oGradientForegroundColor.red = (*l_oColorIterator).second.color[0]; l_oGradientForegroundColor.green = (*l_oColorIterator).second.color[1]; l_oGradientForegroundColor.blue = (*l_oColorIterator).second.color[2];
						this->_cache_change_foreground_cb_(&l_oGradientForegroundColor);
				}
					else
						this->_cache_change_foreground_cb_(&m_oFlashForegroundColor);
					this->_cache_change_background_cb_(&m_oFlashBackgroundColor);
					this->_cache_change_font_cb_(m_pFlashFontDescription);
				}
			}
			m_pTargetFlaggingStimulationEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);
		}

		if(m_pSequenceStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			m_pTargetFlaggingStimulationEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeEnd);
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
		l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
	}

	// --- Target stimulations

	for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		if(m_ui64LastTime>=l_rDynamicBoxContext.getInputChunkStartTime(1, i))
		{
			ip_pTargetMemoryBuffer=l_rDynamicBoxContext.getInputChunk(1, i);
			m_pTargetStimulationDecoder->process();

			if(m_pTargetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
			{
			}

			if(m_pTargetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
			{
				IStimulationSet* l_pStimulationSet=op_pTargetStimulationSet;
				for(j=0; j<l_pStimulationSet->getStimulationCount(); j++)
				{
					uint64 l_ui64StimulationIdentifier=l_pStimulationSet->getStimulationIdentifier(j);
					boolean l_bTarget=false;
					if(l_ui64StimulationIdentifier >= m_ui64StimulationBase)
					{
						this->getLogManager() << LogLevel_Debug << "Received Target Row " << l_ui64StimulationIdentifier << "\n";
						m_iTarget=l_ui64StimulationIdentifier-m_ui64StimulationBase;
						l_bTarget=true;
					}

					if(l_bTarget && m_iTarget!=-1)
					{
						this->getLogManager() << LogLevel_Debug << "Displays Target Character\n";

						this->_cache_change_symbol(m_iTarget);
						this->_cache_change_foreground_cb_(&m_oTargetForegroundColor);
						this->_cache_change_background_cb_(&m_oTargetBackgroundColor);
						this->_cache_change_font_cb_(m_pTargetFontDescription);

						std::string l_sString=gtk_label_get_text(m_pTarget);
						std::string l_sTargetChar = m_vSymbolList[m_iTarget].toASCIIString();
						if(l_sTargetChar.compare("SPC")==0)
						{
							l_sTargetChar= std::string("_");
						}
						l_sString+=l_sTargetChar;
						gtk_label_set_text(m_pTarget, l_sString.c_str());

						m_vTargetHistory.push_back(m_iTarget);
						m_iLastTarget=m_iTarget;
						m_iTarget=-1;
					}
				}
			}

			if(m_pTargetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
			{
			}

			l_rDynamicBoxContext.markInputAsDeprecated(1, i);
		}
	}

	// --- Selection stimulations

	for(k=2; k<3; k++)
	{
		TParameterHandler < const IMemoryBuffer* > ip_pSelectionMemoryBuffer(m_pSelectionStimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
		TParameterHandler < IStimulationSet* > op_pSelectionStimulationSet(m_pSelectionStimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

		for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(k); i++)
		{
			if(m_ui64LastTime>=l_rDynamicBoxContext.getInputChunkStartTime(k, i))
			{
				ip_pSelectionMemoryBuffer=l_rDynamicBoxContext.getInputChunk(k, i);
				m_pSelectionStimulationDecoder->process();

				if(m_pSelectionStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
				{
				}

				if(m_pSelectionStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
				{
					IStimulationSet* l_pStimulationSet=op_pSelectionStimulationSet;
					for(j=0; j<l_pStimulationSet->getStimulationCount(); j++)
					{
						uint64 l_ui64StimulationIdentifier=l_pStimulationSet->getStimulationIdentifier(j);
						boolean l_bSelected=false;
						if(l_ui64StimulationIdentifier == OVTK_StimulationId_VisualStimulationStop)
						{
						}
						if(l_ui64StimulationIdentifier >= m_ui64StimulationBase)
						{
							this->getLogManager() << LogLevel_Debug << "Received Selection " << l_ui64StimulationIdentifier << "\n";
							m_iSelection=l_ui64StimulationIdentifier-m_ui64StimulationBase;
							l_bSelected=true;
						}
						if(l_ui64StimulationIdentifier == OVTK_StimulationId_Label_00)
						{
							m_iSelection=-2;
							l_bSelected=true;
						}
						if(l_bSelected && m_iSelection!=-1)
						{
							if(m_iSelection>=0)
							{
								boolean l_bCorrect = false;
								if(m_vTargetHistory.size()) {
									std::list < int >::const_iterator it=m_vTargetHistory.begin();
									l_bCorrect=(*it==m_iSelection);
									m_vTargetHistory.pop_front();

									this->getLogManager() << LogLevel_Debug << "Displays Selected Cell\n";
									if (l_bCorrect)
									{
										this->_cache_change_symbol(m_iSelection);
										this->_cache_change_foreground_cb_(&m_oCorrectSelectedForegroundColor);
										this->_cache_change_background_cb_(&m_oCorrectSelectedBackgroundColor);
										this->_cache_change_font_cb_(m_pCorrectSelectedFontDescription);	
									}
									else
									{
										this->_cache_change_symbol(m_iSelection);
										this->_cache_change_foreground_cb_(&m_oWrongSelectedForegroundColor);
										this->_cache_change_background_cb_(&m_oWrongSelectedBackgroundColor);
										this->_cache_change_font_cb_(m_pWrongSelectedFontDescription);				
									}
								}
								else
								{
									this->_cache_change_symbol(m_iSelection);
									this->_cache_change_foreground_cb_(&m_oCorrectSelectedForegroundColor);
									this->_cache_change_background_cb_(&m_oCorrectSelectedBackgroundColor);
									this->_cache_change_font_cb_(m_pCorrectSelectedFontDescription);
								}

								std::string l_sString;
								l_sString=m_vSymbolList[m_iSelection].toASCIIString();
								if(l_sString.compare("SPC")==0)
									l_sString = " ";

								if(m_vTargetHistory.size())
								{
									if(l_bCorrect)
									{
										l_sString="<span color=\"darkgreen\">" + l_sString + "</span>";
									}
								}
								l_sString=std::string(gtk_label_get_label(m_pResult))+l_sString;
								gtk_label_set_markup(m_pResult, l_sString.c_str());
			
							}
							else
							{
								this->getLogManager() << LogLevel_Trace << "Selection Rejected !\n";
								std::string l_sString;
								l_sString=gtk_label_get_text(m_pResult);
								l_sString+="*";
								gtk_label_set_text(m_pResult, l_sString.c_str());
							}

							m_iSelection=-1;
						}
					}
				}

				if(m_pSelectionStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
				{
				}

				l_rDynamicBoxContext.markInputAsDeprecated(k, i);
			}
		}
	}

	for(k=3; k<4; k++)
	{
		for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(k); i++)
		{
			m_oAlgo1_StreamedMatrixDecoder.decode(k,i);

			if (m_oAlgo1_StreamedMatrixDecoder.isHeaderReceived())
			{
				m_pLikeliness->setDimensionCount(1);
				if (m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionCount()==1)
				{
					m_pLikeliness->setDimensionSize(0, m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(0));
				}
				else if (m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionCount()==2)
				{
					this->getLogManager() << LogLevel_Warning << "Input matrix has two dimensions, should have only one, trying to correct...\n";
					if (m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(0)==1 &&
						m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(1)>0)
					{
						m_pLikeliness->setDimensionSize(0, m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(1));
					}
					else if (m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(0)>0 &&
						m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(1)==1)
					{
						m_pLikeliness->setDimensionSize(0, m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(0));
					}
					else
						this->getLogManager() << LogLevel_Warning << "None of the dimension sizes equals one, can not interpret this vector.\n";
				}
				else
					this->getLogManager() << LogLevel_Warning << "Expected a vector as input, not a multidimensional input matrix.\n";
			}			

			if (m_oAlgo1_StreamedMatrixDecoder.isBufferReceived())
			{ 
				OpenViBEToolkit::Tools::Matrix::copy(*m_pLikeliness, *m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix()); // the StreamedMatrix of samples.
				m_bReceivedLikeliness = true;
			}
		}
	}

	return true;
}

// _________________________________________________________________________________________________________________________________________________________
//

/*void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_build_from_table_(::GtkTable* pTable)
{
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
					CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& l_rWidgetStyle=m_vCache[i][j];
					l_rWidgetStyle.pWidget=l_pTableChild->widget;
					l_rWidgetStyle.pChildWidget=gtk_bin_get_child(GTK_BIN(l_pTableChild->widget));
					l_rWidgetStyle.oBackgroundColor=l_oWhite;
					l_rWidgetStyle.oForegroundColor=l_oWhite;
					l_rWidgetStyle.pFontDescription=NULL;
				}
			}
		}
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_for_each_(_cache_callback_ fpCallback, void* pUserData)
{
	std::map < unsigned long, std::map < unsigned long, CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle > >::iterator i;
	std::map < unsigned long, CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle >::iterator j;

	for(i=m_vCache.begin(); i!=m_vCache.end(); i++)
	{
		for(j=i->second.begin(); j!=i->second.end(); j++)
		{
			(this->*fpCallback)(j->second, pUserData);
		}
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_for_each_if_(int iLine, int iColumn, _cache_callback_ fpIfCallback, _cache_callback_ fpElseCallback, void* pIfUserData, void* pElseUserData)
{
	std::map < unsigned long, std::map < unsigned long, CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle > >::iterator i;
	std::map < unsigned long, CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle >::iterator j;

	for(i=m_vCache.begin(); i!=m_vCache.end(); i++)
	{
		for(j=i->second.begin(); j!=i->second.end(); j++)
		{
			boolean l_bLine=(iLine!=-1);
			boolean l_bColumn=(iColumn!=-1);
			boolean l_bInLine=false;
			boolean l_bInColumn=false;
			boolean l_bIf;

			if(l_bLine)
			{
				l_bInLine=true;
			}
			if(l_bColumn)
			{
				l_bInColumn=true;
			}

			if(l_bLine && l_bColumn)
			{
				l_bIf=l_bInLine && l_bInColumn;
			}
			else
			{
				l_bIf=l_bInLine || l_bInColumn;
			}

			if(l_bIf)
			{
				(this->*fpIfCallback)(j->second, pIfUserData);
			}
			else
			{
				(this->*fpElseCallback)(j->second, pElseUserData);
			}
		}
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_null_cb_(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData)
{
}*/

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_symbol(int symbolIndex)
{
	if ((symbolIndex<(int)m_vSymbolList.size()) && (symbolIndex>=0))
		gtk_label_set_text(m_pCenterLabel, m_vSymbolList[symbolIndex].toASCIIString());  
	else
		this->getLogManager() << LogLevel_Warning << "Symbol index out of range: " << symbolIndex << "\n";
}

void CBoxAlgorithmSerialP300SpellerVisualisation::clear_screen()
{
	gtk_label_set_text(m_pCenterLabel, " ");   
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_background_cb_(::GdkColor * bgColor)
{
	gtk_widget_modify_bg((::GtkWidget*)m_pCenterEventBox, GTK_STATE_NORMAL, bgColor);
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_foreground_cb_(::GdkColor * fgColor)
{
	gtk_widget_modify_fg((::GtkWidget*)m_pCenterLabel, GTK_STATE_NORMAL, fgColor);
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_change_font_cb_(::PangoFontDescription * fontDesc)
{
	gtk_widget_modify_font((::GtkWidget*)m_pCenterLabel, fontDesc);
}

/*void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_collect_widget_cb_(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData)
{
	if(pUserData)
	{
		((std::vector < ::GtkWidget* >*)pUserData)->push_back(rWidgetStyle.pWidget);
	}
}

void CBoxAlgorithmSerialP300SpellerVisualisation::_cache_collect_child_widget_cb_(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData)
{
	if(pUserData)
	{
		((std::vector < ::GtkWidget* >*)pUserData)->push_back(rWidgetStyle.pChildWidget);
	}
}*/
