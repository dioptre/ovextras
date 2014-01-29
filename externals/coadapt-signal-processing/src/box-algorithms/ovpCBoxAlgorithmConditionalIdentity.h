#ifndef __SamplePlugin_CConditionalIdentity_H__
#define __SamplePlugin_CConditionalIdentity_H__

#include "../ovp_defines.h"
#include <toolkit/ovtk_all.h>
#include <cstdio>

namespace OpenViBEPlugins
{
	namespace SignalProcessingCoAdapt
	{
		class CConditionalIdentity : public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			virtual OpenViBE::boolean initialize(void);
			virtual void release(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_ConditionalIdentity)
			
		protected:
			OpenViBEToolkit::TStimulationDecoder < CConditionalIdentity >  m_pStimulationDecoder;
			OpenViBE::uint64 m_ui64StimulationIdentifier;
			OpenViBE::boolean m_bStopSending;
		};

		class CIdentityListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			OpenViBE::boolean check(OpenViBE::Kernel::IBox& rBox)
			{
				char l_sName[1024];
				OpenViBE::uint32 i;
				for(i=0; i<rBox.getInputCount(); i++)
				{
					sprintf(l_sName, "Input stream %u", i+1);
					rBox.setInputName(i, l_sName);
				}
				for(i=0; i<rBox.getOutputCount(); i++)
				{
					sprintf(l_sName, "Output stream %u", i+1);
					rBox.setOutputName(i, l_sName);
				}
				return true;
			}

			virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.addOutput("", OV_UndefinedIdentifier);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onInputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.removeOutput(ui32Index);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getInputType(ui32Index, l_oTypeIdentifier);
				rBox.setOutputType(ui32Index, l_oTypeIdentifier);
				return true;
			}

			virtual OpenViBE::boolean onOutputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.addInput("", OV_UndefinedIdentifier);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onOutputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.removeInput(ui32Index);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getOutputType(ui32Index, l_oTypeIdentifier);
				rBox.setInputType(ui32Index, l_oTypeIdentifier);
				return true;
			};

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};

		class CConditionalIdentityDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Conditional Identity"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard/Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Duplicates input to output as long as the specified stimulus is not received"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Samples"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-copy"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_ConditionalIdentity; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessingCoAdapt::CConditionalIdentity(); }
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CIdentityListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addInput  ("Input stream", OV_TypeId_Stimulations);
				rPrototype.addOutput("Output stream", OV_TypeId_Stimulations);
				rPrototype.addSetting("Condition", OV_TypeId_Stimulation, "199");
				rPrototype.addFlag  (OpenViBE::Kernel::BoxFlag_CanAddOutput);
				rPrototype.addFlag  (OpenViBE::Kernel::BoxFlag_CanModifyOutput);
				rPrototype.addFlag  (OpenViBE::Kernel::BoxFlag_CanAddInput);
				rPrototype.addFlag  (OpenViBE::Kernel::BoxFlag_CanModifyInput);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_ConditionalIdentityDesc)
		};
	};
};

#endif // __SamplePlugin_CIdentity_H__
