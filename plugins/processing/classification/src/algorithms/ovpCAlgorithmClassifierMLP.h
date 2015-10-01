#ifndef __OpenViBEPlugins_Algorithm_ClassifierMLP_H__
#define __OpenViBEPlugins_Algorithm_ClassifierMLP_H__

#if defined TARGET_HAS_ThirdPartyEIGEN

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define OVP_ClassId_Algorithm_ClassifierMLP                                          OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401260)
#define OVP_ClassId_Algorithm_ClassifierMLP_DecisionAvailable                        OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401261)
#define OVP_ClassId_Algorithm_ClassifierMLPDesc                                      OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401262)

#define OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount               OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401263)
#define OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon                         OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401264)
#define OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha                           OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401265)


#include <Eigen/Dense>

#include <xml/IXMLNode.h>
#include <vector>

namespace OpenViBEPlugins
{
	namespace Classification
	{
		OpenViBE::int32 MLPClassificationCompare(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue);

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

			virtual OpenViBE::uint32 getOutputProbabilityVectorLength();
			virtual OpenViBE::uint32 getOutputDistanceVectorLength();

			_IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierMLP)

		protected:

		private:
			//Helpers for load or sotre data in XMLNode
			void dumpData (XML::IXMLNode *pNode, Eigen::MatrixXd &rMatrix);
			void dumpData (XML::IXMLNode *pNode, Eigen::VectorXd &rVector);
			void dumpData (XML::IXMLNode *pNode, OpenViBE::int64 i64Value);
			void dumpData (XML::IXMLNode *pNode, OpenViBE::float64 f64Value);

			void loadData (XML::IXMLNode *pNode, Eigen::MatrixXd &rMatrix, OpenViBE::int64 i64RowCount, OpenViBE::int64 i64ColCount);
			void loadData (XML::IXMLNode *pNode, Eigen::VectorXd &rVector);
			void loadData (XML::IXMLNode *pNode, OpenViBE::int64 &i64Value);
			void loadData (XML::IXMLNode *pNode, OpenViBE::float64 &f64Value);

			std::vector < OpenViBE::float64 > m_oLabelList;

			Eigen::MatrixXd m_oInputWeight;
			Eigen::VectorXd m_oInputBias;

			Eigen::MatrixXd m_oHiddenWeight;
			Eigen::VectorXd m_oHiddenBias;

			OpenViBE::float64 m_f64Min;
			OpenViBE::float64 m_f64Max;
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

				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount, "Number of neurons in hidden layer",
													  OpenViBE::Kernel::ParameterType_Integer);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon, "Learning stop condition",
													  OpenViBE::Kernel::ParameterType_Float);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha, "Learning coefficient",
													  OpenViBE::Kernel::ParameterType_Float);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierMLPDesc)
		};
	}
}
#endif // TARGET_HAS_ThirdPartyEIGEN

#endif // __OpenViBEPlugins_Algorithm_ClassifierMLP_H__


