#ifndef __OpenViBEPlugins_BoxAlgorithm_MessageSender_H__
#define __OpenViBEPlugins_BoxAlgorithm_MessageSender_H__

//You may have to change this path to match your folder organisation
#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>
#include <cstdio>

//#include <openvibe/ov_all.h>
//#include <openvibe-toolkit/ovtk_all.h>
#include <map>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OVP_ClassId_BoxAlgorithm_MessageSender OpenViBE::CIdentifier(0x8C881F74, 0x920B95A3)
#define OVP_ClassId_BoxAlgorithm_MessageSenderDesc OpenViBE::CIdentifier(0x7135DCFF, 0x643C7DB8)

namespace OpenViBEPlugins
{
	namespace Examples
	{
		/**
		 * \class CBoxAlgorithmMessageSender
		 * \author Loic Mahe (Inria)
		 * \date Thu Aug  8 11:17:24 2013
		 * \brief The class CBoxAlgorithmMessageSender describes the box Message Sender.
		 *
		 */
		class CBoxAlgorithmMessageSender : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			//Here is the different process callbacks possible
			// - On clock ticks :
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			// - On new input received (the most common behaviour for signal processing) :
			//virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			// If you want to use processClock, you must provide the clock frequency.
			virtual OpenViBE::uint64 getClockFrequency(void);
			
			virtual OpenViBE::boolean process(void);

			// As we do with any class in openvibe, we use the macro below 
			// to associate this box to an unique identifier. 
			// The inheritance information is also made available, 
			// as we provide the superclass OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_MessageSender);

		protected:
			// No codec algorithms were specified in the skeleton-generator.
			OpenViBE::uint64 m_ui64BoxFrequency;
			OpenViBE::boolean m_bAppendTestMatrix;

			std::map<OpenViBE::CString, OpenViBE::uint64> m_oIntegers;
			std::map<OpenViBE::CString, OpenViBE::float64> m_oFloats;
			std::map<OpenViBE::CString, OpenViBE::CString> m_oStrings;

		};


		// If you need to implement a box Listener, here is a sekeleton for you.
		// Use only the callbacks you need.
		// For example, if your box has a variable number of input, but all of them must be stimulation inputs.
		// The following listener callback will ensure that any newly added input is stimulations :
		/*		
		virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
		{
			rBox.setInputType(ui32Index, OV_TypeId_Stimulations);
		};
		*/
		
		//*
		// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
		// Please uncomment below the callbacks you want to use.
		class CBoxAlgorithmMessageSenderListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			//virtual OpenViBE::boolean onInitialized(OpenViBE::Kernel::IBox& rBox) { return true; };
			//virtual OpenViBE::boolean onNameChanged(OpenViBE::Kernel::IBox& rBox) { return true; };
			//virtual OpenViBE::boolean onInputConnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputDisconnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputConnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputDisconnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			virtual OpenViBE::boolean onSettingAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				char l_sName[1024];
				sprintf(l_sName, "Setting %u", ui32Index);
				OpenViBE::CString l_sSettingName(&l_sName[0]);
				rBox.setSettingName(ui32Index, l_sSettingName);

				rBox.setSettingDefaultValue(ui32Index, OpenViBE::CString("0") );
				rBox.setSettingValue(ui32Index, OpenViBE::CString("0") );
				return true;
			};
			//virtual OpenViBE::boolean onSettingRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			virtual OpenViBE::boolean onSettingTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier rTypeIdentifier;
				rBox.getSettingType(ui32Index, rTypeIdentifier);

				OpenViBE::boolean l_bIsInteger = (rTypeIdentifier==OV_TypeId_Integer);
				OpenViBE::boolean l_bIsFloat = (rTypeIdentifier==OV_TypeId_Float);
				OpenViBE::boolean l_bIsString = (rTypeIdentifier==OV_TypeId_String);

				if ((!l_bIsInteger)&&(!l_bIsFloat)&&(!l_bIsString))
				{
					rBox.removeSetting(ui32Index);
				}
				return true;
			};
			//virtual OpenViBE::boolean onSettingNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingDefaultValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};
		//*/

		/**
		 * \class CBoxAlgorithmMessageSenderDesc
		 * \author Loic Mahe (Inria)
		 * \date Thu Aug  8 11:17:24 2013
		 * \brief Descriptor of the box Message Sender.
		 *
		 */
		class CBoxAlgorithmMessageSenderDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Message Sender"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Loic Mahe"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Code example on how to construct messages"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("This box can be configured to send out fixed messages periodically."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Examples/Messaging"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(""); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_MessageSender; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Examples::CBoxAlgorithmMessageSender; }
			
			//*
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmMessageSenderListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			//*/
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{	
				rBoxAlgorithmPrototype.addSetting("Frequency",OV_TypeId_Integer,"1");
				rBoxAlgorithmPrototype.addSetting("Append test matrix",OV_TypeId_Boolean,"true");

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);
				
				rBoxAlgorithmPrototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

				rBoxAlgorithmPrototype.addMessageOutput(OpenViBE::CString("Message output"));
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddMessageOutput);

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyMessageOutput);

				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_MessageSenderDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_MessageSender_H__
