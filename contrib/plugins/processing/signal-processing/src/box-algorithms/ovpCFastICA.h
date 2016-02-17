
// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#ifndef __OpenViBEPlugins_SignalProcessing_CFastICA_H__
#define __OpenViBEPlugins_SignalProcessing_CFastICA_H__

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <map>
#include <string>
// Jeff
#include <itpp/itstat.h>
#include <itpp/itsignal.h>

#define FICA_NONLIN_POW3   10 // Use x^3 non-linearity. 
#define FICA_NONLIN_TANH   20 // Use tanh(x) non-linearity. 
#define FICA_NONLIN_GAUSS  30 // Use Gaussian non-linearity. 
#define FICA_NONLIN_SKEW   40 // Use skew non-linearity. 

using namespace itpp;

// TODO create a member function to get rid of this
#ifndef  CString2Boolean
	#define CString2Boolean(string) (strcmp(string,"true"))?0:1
#endif

namespace OpenViBEPlugins
{
	namespace SignalProcessing
	{
		/**
		* The FastICA plugin's main class.
		*/
		class CFastICA : virtual public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			CFastICA(void);

			virtual void release(void);

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);

			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);

			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_FastICA)

		protected:

			virtual void computeICA(void);

			OpenViBEToolkit::TSignalDecoder<CFastICA> m_oDecoder;
			OpenViBEToolkit::TSignalEncoder<CFastICA> m_oEncoder;
		
		// Jeff
		protected:

			OpenViBE::float64 *fifo_buffer;
			OpenViBE::float64 *demixer;

			bool m_bTrained;
			bool m_bFileSaved;

			OpenViBE::uint32  m_ui32Buff_Size;
			OpenViBE::uint32  m_ui32Samp_Nb;
			OpenViBE::uint32  m_ui32Nb_ICs;
			OpenViBE::uint32  m_ui32Duration;
			OpenViBE::uint32  m_ui32NbRep_max;
			OpenViBE::uint32  m_ui32NbTune_max;
			OpenViBE::CString m_sSpatialFilterFilename;
			OpenViBE::boolean m_bSaveAsFile;
			OpenViBE::boolean m_bSetFineTune;
			OpenViBE::float64 m_ui64Set_Mu;
			OpenViBE::float64 m_ui64Epsilon;
			OpenViBE::uint32  m_ui32Non_Lin;

		};

		class CFastICADesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Independent Component Analysis (FastICA)"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Guillaume Gibert / Jeff B."); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INSERM / Independent "); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Computes fast independent component analysis"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Signal processing/Independent component analysis"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.2"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_FastICA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessing::CFastICA(); }

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addInput ("Input signal",  OV_TypeId_Signal);
				rPrototype.addOutput("Output signal", OV_TypeId_Signal);

				rPrototype.addSetting("Number of independent components to extract",   OV_TypeId_Integer,  "14");
				rPrototype.addSetting("Set sample size (seconds) for FastICA",         OV_TypeId_Integer,  "120");
				rPrototype.addSetting("Max number of reps for the ICA convergence",    OV_TypeId_Integer,  "100000");
				rPrototype.addSetting("Set Fine tuning",                               OV_TypeId_Boolean,  "true");
				rPrototype.addSetting("Max number of reps for the fine tuning",        OV_TypeId_Integer,  "100");
				rPrototype.addSetting("Non linearity (10: POW3, 20: TANH, 30: GAUSS)", OV_TypeId_Integer,  "20");
				rPrototype.addSetting("Set internal Mu parameter for FastICA",         OV_TypeId_Float,    "1.0");
				rPrototype.addSetting("Set Epsilon parameter for FastICA",             OV_TypeId_Float,    "0.0001");
				rPrototype.addSetting("Spatial filter filename",                       OV_TypeId_Filename, "");
				rPrototype.addSetting("Save the spatial filter/demixing matrix",        OV_TypeId_Boolean, "true");

				rPrototype.addFlag  (OpenViBE::Kernel::BoxFlag_IsUnstable);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_FastICADesc)
		};
	}
}
#endif // TARGET_HAS_ThirdPartyITPP

#endif // __SamplePlugin_CFastICA_H__
