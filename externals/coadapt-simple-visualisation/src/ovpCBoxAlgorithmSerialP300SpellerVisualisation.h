#ifndef __OpenViBEPlugins_BoxAlgorithm_SerialP300SpellerVisualisation_H__
#define __OpenViBEPlugins_BoxAlgorithm_SerialP300SpellerVisualisation_H__

#include "ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>
#include <map>
#include <list>
#include <iostream>
#include <cstring>
#include <stack>
#include <vector>
#include <map>

#include <xml/IReader.h>

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		class CBoxAlgorithmSerialP300SpellerVisualisation : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, public XML::IReaderCallback
		{

		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32Index);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_SerialP300SpellerVisualisation);

		private:

			/*typedef struct
			{
				::GtkWidget* pWidget;
				::GtkWidget* pChildWidget;
				::GdkColor oBackgroundColor;
				::GdkColor oForegroundColor;
				::PangoFontDescription* pFontDescription;
			} SWidgetStyle;*/

			//typedef void (CBoxAlgorithmSerialP300SpellerVisualisation::*_cache_callback_)(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData);

			void initializeP300SymbolList(OpenViBE::CString filename);
			void initializeColorMap(const ::GdkColor* startColor, const ::GdkColor* endColor, OpenViBE::uint32 nSteps, OpenViBE::float32 sigLevel, OpenViBE::float32 logRate);

			/*void _cache_build_from_table_(::GtkTable* pTable);
			void _cache_for_each_(_cache_callback_ fpCallback, void* pUserData);
			void _cache_for_each_if_(int iLine, int iColumn, _cache_callback_ fpIfCallback, _cache_callback_ fpElseCallback, void* pIfUserData, void* pElseUserData);*/
			//void _cache_change_null_cb_(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData);
			void _cache_change_background_cb_(::GdkColor * bgColor);
			void _cache_change_foreground_cb_(::GdkColor * fgColor);
			void _cache_change_font_cb_(::PangoFontDescription * fontDesc);
			/*void _cache_collect_widget_cb_(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData);
			void _cache_collect_child_widget_cb_(CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle& rWidgetStyle, void* pUserData);*/
			void _cache_change_symbol(int symbolIndex);
			void clear_screen();

		protected:

			virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
			virtual void processChildData(const char* sData); // XML IReaderCallback
			virtual void closeChild(void); // XML ReaderCallback

			std::stack<OpenViBE::CString> m_vNode;

			OpenViBE::CString m_sInterfaceFilename;
			OpenViBE::uint64 m_ui64StimulationBase;

			::GdkColor m_oFlashBackgroundColor;
			::GdkColor m_oFlashForegroundColor;
			OpenViBE::uint64 m_ui64FlashFontSize;
			::PangoFontDescription* m_pFlashFontDescription;
			::GdkColor m_oTargetBackgroundColor;
			::GdkColor m_oTargetForegroundColor;
			OpenViBE::uint64 m_ui64TargetFontSize;
			::PangoFontDescription* m_pTargetFontDescription;
			::GdkColor m_oCorrectSelectedBackgroundColor;
			::GdkColor m_oCorrectSelectedForegroundColor;
			OpenViBE::uint64 m_ui64CorrectSelectedFontSize;
			::PangoFontDescription* m_pCorrectSelectedFontDescription;
			::GdkColor m_oWrongSelectedBackgroundColor;
			::GdkColor m_oWrongSelectedForegroundColor;
			OpenViBE::uint64 m_ui64WrongSelectedFontSize;
			::PangoFontDescription* m_pWrongSelectedFontDescription;

		private:

			OpenViBE::Kernel::IAlgorithmProxy* m_pSequenceStimulationDecoder;
			OpenViBE::Kernel::IAlgorithmProxy* m_pTargetStimulationDecoder;
			OpenViBE::Kernel::IAlgorithmProxy* m_pTargetFlaggingStimulationEncoder;
			OpenViBE::Kernel::IAlgorithmProxy* m_pSelectionStimulationDecoder;
			OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmSerialP300SpellerVisualisation > m_oAlgo1_StreamedMatrixDecoder;
			OpenViBE::Kernel::TParameterHandler<const OpenViBE::IMemoryBuffer*> ip_pSequenceMemoryBuffer;
			OpenViBE::Kernel::TParameterHandler<const OpenViBE::IMemoryBuffer*> ip_pTargetMemoryBuffer;
			OpenViBE::Kernel::TParameterHandler<const OpenViBE::IStimulationSet*> ip_pTargetFlaggingStimulationSet;
			OpenViBE::Kernel::TParameterHandler<OpenViBE::IStimulationSet*> op_pSequenceStimulationSet;
			OpenViBE::Kernel::TParameterHandler<OpenViBE::IStimulationSet*> op_pTargetStimulationSet;
			OpenViBE::Kernel::TParameterHandler<OpenViBE::IMemoryBuffer*> op_pTargetFlaggingMemoryBuffer;
			OpenViBE::uint64 m_ui64LastTime;

			::GtkBuilder* m_pMainWidgetInterface;
			::GtkBuilder* m_pToolbarWidgetInterface;
			::GtkWidget* m_pMainWindow;
			::GtkWidget* m_pToolbarWidget;
			::GtkTable* m_pTable;
			::GtkLabel* m_pResult;
			::GtkLabel* m_pTarget;
			::GtkLabel* m_pCenterLabel;
			::GtkEventBox* m_pCenterEventBox;

			int m_iLastTarget;
			int m_iTarget;
			int m_iSelection;

			OpenViBE::boolean m_bTableInitialized;

			//std::map < unsigned long, std::map < unsigned long, CBoxAlgorithmSerialP300SpellerVisualisation::SWidgetStyle > > m_vCache;
			std::list < int > m_vTargetHistory;
			std::vector <OpenViBE::CString> m_vSymbolList;

			OpenViBE::IMatrix* m_pLikeliness;
			OpenViBE::boolean m_bReceivedLikeliness;

			struct ColorTriple
			{
			    guint16 color[3];
			};
			std::map<OpenViBE::float32, ColorTriple> m_mColorMap;
		};

		class CBoxAlgorithmSerialP300SpellerVisualisationDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("P300 Serial Speller Visualisation"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-select-font"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_SerialP300SpellerVisualisation; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CBoxAlgorithmSerialP300SpellerVisualisation; }

			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePF) const { return ePF == OpenViBE::Kernel::PluginFunctionality_Visualization; }
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput ("Sequence stimulations",            OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput ("Target stimulations",              OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput ("Selection stimulations",       OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput ("Character likeliness",       OV_TypeId_StreamedMatrix);

				rBoxAlgorithmPrototype.addOutput("Target / Non target flagging",     OV_TypeId_Stimulations);

				rBoxAlgorithmPrototype.addSetting("Interface filename",              OV_TypeId_Filename,    "../share/coadapt/openvibe-plugins/simple-visualisation/p300-serial-speller.ui");
				rBoxAlgorithmPrototype.addSetting("Stimulation base",            OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");

				rBoxAlgorithmPrototype.addSetting("Flash background color",          OV_TypeId_Color,       "10,10,10");
				rBoxAlgorithmPrototype.addSetting("Flash foreground color",          OV_TypeId_Color,       "100,100,100");
				rBoxAlgorithmPrototype.addSetting("Flash font size",                 OV_TypeId_Integer,     "100");

				rBoxAlgorithmPrototype.addSetting("Target background color",         OV_TypeId_Color,       "10,40,10");
				rBoxAlgorithmPrototype.addSetting("Target foreground color",         OV_TypeId_Color,       "60,100,60");
				rBoxAlgorithmPrototype.addSetting("Target font size",                OV_TypeId_Integer,     "100");

				rBoxAlgorithmPrototype.addSetting("Correct selected background color",       OV_TypeId_Color,       "70,20,20");
				rBoxAlgorithmPrototype.addSetting("Correct selected foreground color",       OV_TypeId_Color,       "30,10,10");
				rBoxAlgorithmPrototype.addSetting("Correct selected font size",              OV_TypeId_Integer,     "100");

				rBoxAlgorithmPrototype.addSetting("Wrong selected background color",       OV_TypeId_Color,       "70,20,20");
				rBoxAlgorithmPrototype.addSetting("Wrong selected foreground color",       OV_TypeId_Color,       "30,10,10");
				rBoxAlgorithmPrototype.addSetting("Wrong selected font size",              OV_TypeId_Integer,     "100");

				rBoxAlgorithmPrototype.addSetting("Result font size",              OV_TypeId_Integer,     "20");

				rBoxAlgorithmPrototype.addSetting("Letters group filename",              OV_TypeId_Filename,    "../share/coadapt/openvibe-plugins/simple-visualisation/p300-serial-symbols.xml");
				
				rBoxAlgorithmPrototype.addSetting("Number of feedback color steps",                OV_TypeId_Integer,     "100");
				rBoxAlgorithmPrototype.addSetting("Feedback color significance level",                OV_TypeId_Float,     "0.1");
				rBoxAlgorithmPrototype.addSetting("Feedback color rate",                OV_TypeId_Float,     "2");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_SerialP300SpellerVisualisationDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_SerialP300SpellerVisualisation_H__
