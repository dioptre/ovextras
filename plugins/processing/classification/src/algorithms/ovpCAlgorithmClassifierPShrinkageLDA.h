#if defined TARGET_HAS_ThirdPartyEIGEN

#ifndef __OpenViBEPlugins_Algorithm_ClassifierPShrinkageLDA_H__
#define __OpenViBEPlugins_Algorithm_ClassifierPShrinkageLDA_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>

#include <stack>

#include <Eigen/Dense>

#define OVP_ClassId_Algorithm_ClassifierPShrinkageLDA                                        OpenViBE::CIdentifier(0xFCC44AE1, 0x6225529A)
#define OVP_ClassId_Algorithm_ClassifierPShrinkageLDADesc                                    OpenViBE::CIdentifier(0x928D97ED, 0xB4EA25EE)

#define OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_Shrinkage                   OpenViBE::CIdentifier(0x01357534, 0x028312A1)
#define OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_DiagonalCov                 OpenViBE::CIdentifier(0x067E45C5, 0x15285CC7)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		OpenViBE::int32 getPShrinkageLDABestClassification(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue);

		class CAlgorithmClassifierPShrinkageLDA : public OpenViBEToolkit::CAlgorithmClassifier
		{
		typedef Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

		public:

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);

			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual XML::IXMLNode* saveConfiguration(void);
			virtual OpenViBE::boolean loadConfiguration(XML::IXMLNode *pConfigurationNode);

			_IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierPShrinkageLDA);

		protected:
			// Debug method. Prints the matrix to the logManager. May be disabled in implementation.
			void dumpMatrix(OpenViBE::Kernel::ILogManager& pMgr, const MatrixXdRowMajor& mat, const OpenViBE::CString& desc);

			OpenViBE::float64 m_f64Class1;
			OpenViBE::float64 m_f64Class2;

			Eigen::MatrixXd m_oCoefficients;
			Eigen::MatrixXd m_oCoefficientsClass1;
			Eigen::MatrixXd m_oCoefficientsClass2;
			OpenViBE::uint32 m_ui32NumCols;

			XML::IXMLNode *m_pConfigurationNode;

			OpenViBE::Kernel::IAlgorithmProxy* m_pCovarianceAlgorithm;

		private:
			void loadClassesFromNode(XML::IXMLNode *pNode);
			void loadCoefficientsFromNode(XML::IXMLNode *pNode);

			void generateConfigurationNode(void);
		};

		class CAlgorithmClassifierPShrinkageLDADesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Probabilistic Shrinkage LDA Classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Estimates LDA using regularized covariances"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierPShrinkageLDA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierPShrinkageLDA; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_Shrinkage, "sLDA: Shrinkage (-1 == auto)",OpenViBE::Kernel::ParameterType_Float);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_DiagonalCov,"sLDA: Force diagonal cov (DDA)",OpenViBE::Kernel::ParameterType_Boolean);

				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierPShrinkageLDADesc);
		};
	};
};

#endif // __OpenViBEPlugins_Algorithm_ClassifierShrinkageLDA_H__

#endif // TARGET_HAS_ThirdPartyEIGEN


