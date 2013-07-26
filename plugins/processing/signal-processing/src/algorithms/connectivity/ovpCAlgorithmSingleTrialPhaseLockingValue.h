#if defined(TARGET_HAS_ThirdPartyEIGEN)

#ifndef __OpenViBEPlugins_Algorithm_SingleTrialPhaseLockingValue_H__
#define __OpenViBEPlugins_Algorithm_SingleTrialPhaseLockingValue_H__

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>

#include "../basic/ovpCHilbertTransform.h"

/*
#define OVP_TypeId_Algorithm_SingleTrialPhaseLockingValue									OpenViBE::CIdentifier(0x344B79DE, 0x89EAAABB)
#define OVP_TypeId_Algorithm_SingleTrialPhaseLockingValueDesc								OpenViBE::CIdentifier(0x8CAB236A, 0xA789800D)
*/

namespace OpenViBEPlugins
{
	namespace SignalProcessing
	{
		class CAlgorithmSingleTrialPhaseLockingValue : public OpenViBEToolkit::CConnectivityAlgorithm //OpenViBEToolkit::TAlgorithm <OpenViBE::Plugins::IAlgorithm>
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean process(void);


			_IsDerivedFromClass_Final_(OpenViBEToolkit::CConnectivityAlgorithm, OVP_TypeId_Algorithm_SingleTrialPhaseLockingValue);

		protected:

			OpenViBE::Kernel::IAlgorithmProxy* m_pHilbertTransform;

			OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> ip_pSignal1;
			OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> ip_pSignal2;

			OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> ip_pChannelPairs;
			OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pMatrix;

			OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> ip_pHilbertInput;
			OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pInstantaneousPhase;


		private:

			void HilbertPhase (Eigen::VectorXd InputVector, Eigen::VectorXd OutputVector);
		};

		class CAlgorithmSingleTrialPhaseLockingValueDesc : public OpenViBEToolkit::CConnectivityAlgorithmDesc //OpenViBE::Plugins::IAlgorithmDesc
		{
		public:
			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Phase Locking algorithm"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Alison Cellard"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Computes the Phase-Locking Value algorithm"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Signal processing/Connectivity"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-execute"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_TypeId_Algorithm_SingleTrialPhaseLockingValue; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessing::CAlgorithmSingleTrialPhaseLockingValue; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
					OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{

				rAlgorithmPrototype.addInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_InputMatrix1,     "Signal 1", OpenViBE::Kernel::ParameterType_Matrix);
				rAlgorithmPrototype.addInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_InputMatrix2,     "Signal 2", OpenViBE::Kernel::ParameterType_Matrix);
				rAlgorithmPrototype.addInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_LookupMatrix, "Pairs of channel", OpenViBE::Kernel::ParameterType_Matrix);

				rAlgorithmPrototype.addOutputParameter(OVTK_Algorithm_Connectivity_OutputParameterId_OutputMatrix,    "Matrix", OpenViBE::Kernel::ParameterType_Matrix);

				rAlgorithmPrototype.addInputTrigger   (OVTK_Algorithm_Connectivity_InputTriggerId_Initialize,   "Initialize");
				rAlgorithmPrototype.addInputTrigger   (OVTK_Algorithm_Connectivity_InputTriggerId_Process,      "Process");
				rAlgorithmPrototype.addOutputTrigger  (OVTK_Algorithm_Connectivity_OutputTriggerId_ProcessDone, "Process done");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::CConnectivityAlgorithmDesc, OVP_TypeId_Algorithm_SingleTrialPhaseLockingValueDesc);
		};
	};  // namespace SignalProcessing
}; // namespace OpenViBEPlugins

#endif //__OpenViBEPlugins_Algorithm_SingleTrialPhaseLockingValue_H__
#endif //TARGET_HAS_ThirdPartyEIGEN
