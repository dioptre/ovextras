#ifndef __OpenViBEPlugins_SimpleVisualisation_CGrazVisualization_H__
#define __OpenViBEPlugins_SimpleVisualisation_CGrazVisualization_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>

#include <vector>
#include <string>
#include <map>
#include <deque>
#include <memory>

namespace TCPTagging
{
	class IStimulusMultiSender; // fwd declare
};

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{

		enum EArrowDirection
		{
			EArrowDirection_None	= 0,
			EArrowDirection_Left,
			EArrowDirection_Right,
			EArrowDirection_Up,
			EArrowDirection_Down,
		};

		enum EGrazVisualizationState
		{
			EGrazVisualizationState_Idle,
			EGrazVisualizationState_Reference,
			EGrazVisualizationState_Cue,
			EGrazVisualizationState_ContinousFeedback
		};

		/**
		*/
		class CGrazVisualization :
			virtual public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			CGrazVisualization(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize();
			virtual OpenViBE::boolean uninitialize();
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process();

			virtual void redraw();
			virtual void resize(OpenViBE::uint32 ui32Width, OpenViBE::uint32 ui32Height);

		public:

			void flushQueue(void);					// Sends all accumulated stimuli to the TCP Tagging

		protected:

			virtual void setStimulation(const OpenViBE::uint32 ui32StimulationIndex, const OpenViBE::uint64 ui64StimulationIdentifier, const OpenViBE::uint64 ui64StimulationDate);

			virtual void setMatrixBuffer(const OpenViBE::float64* pBuffer);

			virtual void processState();


			virtual void drawReferenceCross();
			virtual void drawArrow(EArrowDirection eDirection);
			virtual void drawBar();
			virtual void drawAccuracy();
			virtual void updateConfusionMatrix(OpenViBE::float64 f64Prediction);
			virtual OpenViBE::float64 aggregatePredictions(bool bIncludeAll);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_GrazVisualization)

		public:
			//! The Builder handler used to create the interface
			::GtkBuilder* m_pBuilderInterface;

			GtkWidget * m_pMainWindow;

			GtkWidget * m_pDrawingArea;

			//ebml
			OpenViBEToolkit::TStimulationDecoder<CGrazVisualization> m_oStimulationDecoder;
			OpenViBEToolkit::TStreamedMatrixDecoder<CGrazVisualization> m_oMatrixDecoder;

			EGrazVisualizationState m_eCurrentState;
			EArrowDirection m_eCurrentDirection;

			OpenViBE::float64 m_f64MaxAmplitude;
			OpenViBE::float64 m_f64BarScale;

			//Start and end time of the last buffer
			OpenViBE::uint64 m_ui64StartTime;
			OpenViBE::uint64 m_ui64EndTime;

			OpenViBE::boolean m_bTwoValueInput;

			GdkPixbuf * m_pOriginalBar;
			GdkPixbuf * m_pLeftBar;
			GdkPixbuf * m_pRightBar;

			GdkPixbuf * m_pOriginalLeftArrow;
			GdkPixbuf * m_pOriginalRightArrow;
			GdkPixbuf * m_pOriginalUpArrow;
			GdkPixbuf * m_pOriginalDownArrow;

			GdkPixbuf * m_pLeftArrow;
			GdkPixbuf * m_pRightArrow;
			GdkPixbuf * m_pUpArrow;
			GdkPixbuf * m_pDownArrow;

			GdkColor m_oBackgroundColor;
			GdkColor m_oForegroundColor;

			std::deque<OpenViBE::float64> m_vAmplitude; // predictions for the current trial

			OpenViBE::boolean m_bShowInstruction;
			OpenViBE::boolean m_bShowFeedback;
			OpenViBE::boolean m_bDelayFeedback;
			OpenViBE::boolean m_bShowAccuracy;
			OpenViBE::boolean m_bPositiveFeedbackOnly;

			OpenViBE::uint64 m_i64PredictionsToIntegrate;

			OpenViBE::CMatrix m_oConfusion;
			
			TCPTagging::IStimulusMultiSender* m_pStimulusMultiSender;

			OpenViBE::uint64 m_ui64LastStimulation;
		};

		/**
		* Plugin's description
		*/
		class CGrazVisualizationDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Graz visualization"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Bruno Renier, Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Visualization plugin for the Graz experiment"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Visualization/Feedback plugin for the Graz experiment"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.2"); }
			virtual void release(void)                                   { }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_GrazVisualization; }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-fullscreen"); }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CGrazVisualization(); }

			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePF) const
			{
				return ePF == OpenViBE::Kernel::PluginFunctionality_Visualization;
			}

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addInput("Stimulations", OV_TypeId_Stimulations);
				rPrototype.addInput("Amplitude", OV_TypeId_StreamedMatrix);

				rPrototype.addSetting("Show instruction", OV_TypeId_Boolean,            "true");
				rPrototype.addSetting("Show feedback", OV_TypeId_Boolean,               "false");
				rPrototype.addSetting("Delay feedback", OV_TypeId_Boolean,              "false");
				rPrototype.addSetting("Show accuracy", OV_TypeId_Boolean,               "false");
				rPrototype.addSetting("Predictions to integrate", OV_TypeId_Integer,    "5");
				rPrototype.addSetting("Positive feedback only", OV_TypeId_Boolean,      "false");
				rPrototype.addSetting("TCP Tagging Host address", OV_TypeId_String, "localhost");
				rPrototype.addSetting("TCP Tagging Host port", OV_TypeId_Integer, "15361");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_GrazVisualizationDesc)
		};

	};
};

#endif
