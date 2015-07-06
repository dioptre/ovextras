#ifndef __OpenViBEPlugins_Algorithm_ClassifierMLP_H__
#define __OpenViBEPlugins_Algorithm_ClassifierMLP_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

//#if defined TARGET_HAS_ThirdPartyEIGEN

#define OVP_ClassId_Algorithm_ClassifierMLP                                          OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401260)
#define OVP_ClassId_Algorithm_ClassifierMLP_DecisionAvailable                        OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401261)
#define OVP_ClassId_Algorithm_ClassifierMLPDesc                                      OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401262)

#define OVP_Algorithm_ClassifierMLP_InputParameterId_TransfertFunction               OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401263)
#define OVP_Algorithm_ClassifierMLP_InputParameterId_EvaluationFunction              OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401264)
#define OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount               OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401265)

#define OVP_TypeId_Enumeration_TransfertFunction                                     OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401267)
#define OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Identity           OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401268)
#define OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Softmax            OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401269)
#define OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Sigmoid            OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC40126A)

#define OVP_TypeId_Enumeration_EvaluationFunction                                    OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC40126B)
#define OVP_Algorithm_ClassifierMLP_Enumeration_EvaluationFunction_Quadratic         OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC40126C)
#define OVP_Algorithm_ClassifierMLP_Enumeration_EvaluationFunction_MisClassification OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC40126D)




#include <xml/IXMLNode.h>

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CAlgorithmClassifierMLP : public OpenViBEToolkit::CAlgorithmClassifier
		{

		public:

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);

			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector
											   , OpenViBE::float64& rf64Class
											   , OpenViBEToolkit::IVector& rDistanceValue
											   , OpenViBEToolkit::IVector& rProbabilityValue);

			virtual XML::IXMLNode* saveConfiguration(void);
			virtual OpenViBE::boolean loadConfiguration(XML::IXMLNode *pConfigurationNode);

			_IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierMLP)

		protected:

		private:

			OpenViBE::int64 m_i64HiddenNeuronCount;
			OpenViBE::CIdentifier m_oEvaluationFunctionIdentifier;
			OpenViBE::CIdentifier m_oTransfertFunctionIdentifier;
		};

		class CAlgorithmClassifierMLPDesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("MLP Classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Guillaume Serrière"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria / Loria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Multi-layer perceptron algorithm"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierMLP; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierMLP; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{

				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);

				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_TransfertFunction,c_sMLPTransfertFunctionName,
													  OpenViBE::Kernel::ParameterType_Enumeration, OVP_TypeId_Enumeration_TransfertFunction);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_EvaluationFunction,c_sMLPEvaluationFunctionName,
													  OpenViBE::Kernel::ParameterType_Enumeration, OVP_TypeId_Enumeration_EvaluationFunction);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount, "Amount of neuron in hidden layer",
													  OpenViBE::Kernel::ParameterType_Integer);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierMLPDesc)
		};
	}
}


//#endif // TARGET_HAS_ThirdPartyEIGEN

#endif // __OpenViBEPlugins_Algorithm_ClassifierMLP_H__


