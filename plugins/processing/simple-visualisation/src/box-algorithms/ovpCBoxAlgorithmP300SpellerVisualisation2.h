#ifndef __OpenViBEPlugins_BoxAlgorithm_P300SpellerVisualisation2_H__
#define __OpenViBEPlugins_BoxAlgorithm_P300SpellerVisualisation2_H__

// Based on 'P300 Speller Visualization' by Yann Renard

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <visualization-toolkit/ovviz_all.h>

#include <gtk/gtk.h>
#include <map>
#include <list>
#include <queue>

// TODO:
// - please move the identifier definitions in ovp_defines.h
// - please include your desciptor in ovp_main.cpp

#define OVP_ClassId_BoxAlgorithm_P300SpellerVisualisation2     OpenViBE::CIdentifier(0x1B255925, 0x258C3852)
#define OVP_ClassId_BoxAlgorithm_P300SpellerVisualisation2Desc OpenViBE::CIdentifier(0x8412A601, 0x92375D35)

namespace TCPTagging
{
	class IStimulusSender; // fwd declare
};

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{


		class CBoxAlgorithmP300SpellerVisualisation2 : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:

			virtual void release(void) { delete this; }

			virtual bool initialize(void);
			virtual bool uninitialize(void);
			virtual bool processInput(OpenViBE::uint32 ui32Index);
			virtual bool process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_P300SpellerVisualisation2);

		private:

			typedef struct
			{
				::GtkWidget* pWidget;
				::GtkWidget* pChildWidget;
				::GdkColor oBackgroundColor;
				::GdkColor oForegroundColor;
				::PangoFontDescription* pFontDescription;
			} SWidgetStyle;

			typedef void (CBoxAlgorithmP300SpellerVisualisation2::*_cache_callback_)(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData);

			void _cache_build_from_table_(::GtkTable* pTable);
			void _cache_for_each_(_cache_callback_ fpCallback, void* pUserData);
			void _cache_for_each_if_(int iLine, int iColumn, _cache_callback_ fpIfCallback, _cache_callback_ fpElseCallback, void* pIfUserData, void* pElseUserData);
			void _cache_change_null_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData);
			void _cache_change_background_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData);
			void _cache_change_foreground_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData);
			void _cache_change_font_cb_(CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle& rWidgetStyle, void* pUserData);

			bool flashNow(const OpenViBE::CMatrix& oFlashGroup);
			bool setSelection(OpenViBE::uint64 ui64StimulationIdentifier, bool isMultiCharacter);

		public:

			void flushQueue(void);					// Sends all accumulated stimuli to the TCP Tagging

		protected:

			OpenViBE::CString m_sInterfaceFilename;
			OpenViBE::uint64 m_ui64RowStimulationBase;
			OpenViBE::uint64 m_ui64ColumnStimulationBase;

			::GdkColor m_oFlashBackgroundColor;
			::GdkColor m_oFlashForegroundColor;
			OpenViBE::uint64 m_ui64FlashFontSize;
			::PangoFontDescription* m_pFlashFontDescription;
			::GdkColor m_oNoFlashBackgroundColor;
			::GdkColor m_oNoFlashForegroundColor;
			OpenViBE::uint64 m_ui64NoFlashFontSize;
			::PangoFontDescription* m_pNoFlashFontDescription;
			::GdkColor m_oTargetBackgroundColor;
			::GdkColor m_oTargetForegroundColor;
			OpenViBE::uint64 m_ui64TargetFontSize;
			::PangoFontDescription* m_pTargetFontDescription;
			::GdkColor m_oSelectedBackgroundColor;
			::GdkColor m_oSelectedForegroundColor;
			OpenViBE::uint64 m_ui64SelectedFontSize;
			::PangoFontDescription* m_pSelectedFontDescription;

		private:

			OpenViBEToolkit::TFeatureVectorDecoder<CBoxAlgorithmP300SpellerVisualisation2> m_oFlashGroupDecoder;
			OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmP300SpellerVisualisation2> m_oTimelineDecoder;
			OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmP300SpellerVisualisation2> m_oTargetDecoder;
			OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmP300SpellerVisualisation2> m_oSelectionDecoder;

			::GtkBuilder* m_pMainWidgetInterface;
			::GtkBuilder* m_pToolbarWidgetInterface;
			::GtkWidget* m_pMainWindow;
			::GtkWidget* m_pToolbarWidget;
			::GtkTable* m_pTable;
			::GtkLabel* m_pResult;
			::GtkLabel* m_pTarget;
			OpenViBE::uint64 m_ui64RowCount;
			OpenViBE::uint64 m_ui64ColumnCount;

			int m_iSelectedRow;
			int m_iSelectedColumn;

			bool m_bTableInitialized;
			bool m_bTargetSettingMode;

			int m_iTargetRow;
			int m_iTargetColumn;

			std::string m_sTargetText;
			std::string m_sSpelledText;

			std::queue<OpenViBE::CMatrix*> m_vFlashGroup;
			std::queue<OpenViBE::uint64> m_vNextFlashTime;
			std::queue<OpenViBE::uint64> m_vNextFlashStopTime;

			// Maps from keyboard coordinates to the widget
			std::map < std::pair< int, int >, CBoxAlgorithmP300SpellerVisualisation2::SWidgetStyle > m_vCache;
			std::list < std::pair < int, int > > m_vTargetHistory;

			// TCP Tagging
			std::vector< OpenViBE::uint64 > m_vStimuliQueue;
			guint m_uiIdleFuncTag;
			TCPTagging::IStimulusSender* m_pStimulusSender;

			OpenViBEVisualizationToolkit::IVisualizationContext* m_visualizationContext;
		};

		class CBoxAlgorithmP300SpellerVisualisation2Desc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("P300 Speller Visualisation II"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("P300 keyboard with arbitrary flash patterns"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualization/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-select-font"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_P300SpellerVisualisation2; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CBoxAlgorithmP300SpellerVisualisation2; }

			virtual bool hasFunctionality(OpenViBE::CIdentifier functionalityIdentifier) const
			{
				return functionalityIdentifier == OVD_Functionality_Visualization;
			}

			virtual bool getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Flash patterns",                   OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput("Timeline",                         OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Target stimulations",              OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Selection stimulations",           OV_TypeId_Stimulations);

				rBoxAlgorithmPrototype.addSetting("Interface filename",              OV_TypeId_Filename,    "${Path_Data}/plugins/simple-visualisation/p300-speller-hash.ui");
				rBoxAlgorithmPrototype.addSetting("Row stimulation base",            OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rBoxAlgorithmPrototype.addSetting("Column stimulation base",         OV_TypeId_Stimulation, "OVTK_StimulationId_Label_08");

				rBoxAlgorithmPrototype.addSetting("Flash background color",          OV_TypeId_Color,       "10,10,10");
				rBoxAlgorithmPrototype.addSetting("Flash foreground color",          OV_TypeId_Color,       "100,100,100");
				rBoxAlgorithmPrototype.addSetting("Flash font size",                 OV_TypeId_Integer,     "100");

				rBoxAlgorithmPrototype.addSetting("No flash background color",       OV_TypeId_Color,       "0,0,0");
				rBoxAlgorithmPrototype.addSetting("No flash foreground color",       OV_TypeId_Color,       "50,50,50");
				rBoxAlgorithmPrototype.addSetting("No flash font size",              OV_TypeId_Integer,     "75");

				rBoxAlgorithmPrototype.addSetting("Target background color",         OV_TypeId_Color,       "10,40,10");
				rBoxAlgorithmPrototype.addSetting("Target foreground color",         OV_TypeId_Color,       "60,100,60");
				rBoxAlgorithmPrototype.addSetting("Target font size",                OV_TypeId_Integer,     "100");

				rBoxAlgorithmPrototype.addSetting("Selected background color",       OV_TypeId_Color,       "70,20,20");
				rBoxAlgorithmPrototype.addSetting("Selected foreground color",       OV_TypeId_Color,       "30,10,10");
				rBoxAlgorithmPrototype.addSetting("Selected font size",              OV_TypeId_Integer,     "100");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_P300SpellerVisualisation2Desc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_P300SpellerVisualisation2_H__
