#ifndef __OpenViBEPlugins_SimpleVisualisation_CDisplayCueImage_H__
#define __OpenViBEPlugins_SimpleVisualisation_CDisplayCueImage_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <map>
#include <deque>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/version.hpp>

#define OVP_ClassId_DisplayCueImage                                            OpenViBE::CIdentifier(0x005789A4, 0x3AB78A36)
#define OVP_ClassId_DisplayCueImageDesc                                        OpenViBE::CIdentifier(0x086185A4, 0x796A854C)

namespace TCPTagging
{
	class IStimulusSender; // fwd declare
};

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{

		class CDisplayCueImage :
				public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			CDisplayCueImage(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize();
			virtual OpenViBE::boolean uninitialize();
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::uint64 getClockFrequency(void){ return (128LL<<32); }				// 128hz
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			virtual OpenViBE::boolean process();
			virtual void redraw(void);
			virtual void resize(OpenViBE::uint32 ui32Width, OpenViBE::uint32 ui32Height);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_DisplayCueImage)

		public:

			void flushQueue(void);					// Sends all accumulated stimuli to the TCP Tagging

			static const OpenViBE::uint32 m_ui32NonCueSettingsCount = 3; // fullscreen + scale + clear

		protected:

			virtual void drawCuePicture(OpenViBE::uint32 uint32CueID);

			//The Builder handler used to create the interface
			::GtkBuilder* m_pBuilderInterface;
			::GtkWidget*  m_pMainWindow;
			::GtkWidget*  m_pDrawingArea;

			OpenViBEToolkit::TStimulationDecoder<CDisplayCueImage> m_oStimulationDecoder;
			OpenViBEToolkit::TStimulationEncoder<CDisplayCueImage> m_oStimulationEncoder;

			// For the display of the images:
			OpenViBE::boolean m_bImageRequested;        //when true: a new image must be drawn
			OpenViBE::int32   m_int32RequestedImageID;  //ID of the requested image. -1 => clear the screen

			OpenViBE::boolean m_bImageDrawn;            //when true: the new image has been drawn
			OpenViBE::int32   m_int32DrawnImageID;      //ID of the drawn image. -1 => clear the screen

			// Data corresponding to each cue image. Could be refactored to a vector of structs.
			std::vector< GdkPixbuf* > m_vOriginalPicture;
			std::vector< GdkPixbuf* > m_vScaledPicture;
			std::vector<OpenViBE::uint64> m_vStimulationsId;
			std::vector<OpenViBE::CString> m_vImageNames;

			::GdkColor m_oBackgroundColor;
			::GdkColor m_oForegroundColor;

			//Settings
			OpenViBE::uint32   m_ui32NumberOfCues;
			OpenViBE::uint64   m_ui64ClearScreenStimulation;
			OpenViBE::boolean  m_bFullScreen;
			OpenViBE::boolean  m_bScaleImages;

			//Start and end time of the last buffer
			OpenViBE::uint64 m_ui64StartTime;
			OpenViBE::uint64 m_ui64EndTime;
			OpenViBE::uint64 m_ui64LastOutputChunkDate;

			//We save the received stimulations
			OpenViBE::CStimulationSet m_oPendingStimulationSet;

			// For queuing stimulations to the TCP Tagging
			std::vector< OpenViBE::uint64 > m_vStimuliQueue;
			guint m_uiIdleFuncTag;								// This is not really used, its for debugging purposes
			boost::mutex m_oIdleFuncMutex;
			TCPTagging::IStimulusSender* m_pStimulusSender;
		};

		class CDisplayCueImageListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			virtual OpenViBE::boolean onSettingAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				const OpenViBE::uint32 l_ui32PreviousCues = ((ui32Index - 1) - CDisplayCueImage::m_ui32NonCueSettingsCount) / 2 + 1;
				const OpenViBE::uint32 l_ui32CueNumber = l_ui32PreviousCues + 1;

				char l_sValue[1024];
				sprintf(l_sValue, "${Path_Data}/plugins/simple-visualisation/p300-magic-card/%02d.png", l_ui32CueNumber);

				rBox.setSettingDefaultValue(ui32Index, l_sValue);
				rBox.setSettingValue(ui32Index, l_sValue);

				sprintf(l_sValue, "OVTK_StimulationId_Label_%02X", l_ui32CueNumber);
				rBox.addSetting("", OV_TypeId_Stimulation, l_sValue);
				rBox.setSettingDefaultValue(ui32Index+1, l_sValue);
				rBox.setSettingValue(ui32Index+1, l_sValue);

				checkSettingNames(rBox);

				return true;
			}

			virtual OpenViBE::boolean onSettingRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				// Remove also the associated setting in the other slot
				const OpenViBE::uint32 l_ui32IndexNumber = (ui32Index - CDisplayCueImage::m_ui32NonCueSettingsCount);

				if (l_ui32IndexNumber % 2 == 0)
				{
					// This was the 'cue image' setting, remove 'stimulation setting'
					// when onSettingRemoved is called, ui32Index has already been removed, so using it will effectively mean 'remove next setting'.
					rBox.removeSetting(ui32Index);
				}
				else
				{
					// This was the 'stimulation setting'. Remove the 'cue image' setting.
					rBox.removeSetting(ui32Index - 1);
				}

				checkSettingNames(rBox);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);

		private:

			// This function is used to make sure the setting names and types are correct
			OpenViBE::boolean checkSettingNames(OpenViBE::Kernel::IBox& rBox)
			{
				char l_sName[1024];
				for (OpenViBE::uint32 i = CDisplayCueImage::m_ui32NonCueSettingsCount; i < rBox.getSettingCount() - 1; i += 2)
				{
					sprintf(l_sName, "Cue Image %i", i / 2);
					rBox.setSettingName(i, l_sName);
					rBox.setSettingType(i, OV_TypeId_Filename);
					sprintf(l_sName, "Stimulation %i", i / 2);
					rBox.setSettingName(i + 1, l_sName);
					rBox.setSettingType(i + 1, OV_TypeId_Stimulation);
				}
				return true;
			}
		};

		/**
		 * Plugin's description
		 */
		class CDisplayCueImageDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Display cue image"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Joan Fruitet, Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria Sophia, Inria Rennes"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Display cue images when receiving stimulations"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Display cue images when receiving specified stimulations. Forwards the stimulations to the AS using TCP Tagging."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Presentation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.2"); }
			virtual void release(void)                                   { }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_DisplayCueImage; }

			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-fullscreen"); }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CDisplayCueImage(); }
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CDisplayCueImageListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePF) const
			{
				return ePF == OpenViBE::Kernel::PluginFunctionality_Visualization;
			}

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addInput  ("Stimulations", OV_TypeId_Stimulations);
				rPrototype.addOutput ("Stimulations (deprecated)", OV_TypeId_Stimulations);
				rPrototype.addSetting("Display images in full screen", OV_TypeId_Boolean, "false");
				rPrototype.addSetting("Scale images to fit", OV_TypeId_Boolean, "false");
				rPrototype.addSetting("Clear screen Stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_VisualStimulationStop");
				rPrototype.addSetting("Cue Image 1", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualisation/p300-magic-card/01.png");
				rPrototype.addSetting("Stimulation 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rPrototype.addFlag   (OpenViBE::Kernel::BoxFlag_CanAddSetting);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_DisplayCueImageDesc)
		};
	};
};

#endif
