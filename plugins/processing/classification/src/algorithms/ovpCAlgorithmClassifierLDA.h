
// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#ifndef __OpenViBEPlugins_Algorithm_ClassifierLDA_H__
#define __OpenViBEPlugins_Algorithm_ClassifierLDA_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>
#define TARGET_HAS_ThirdPartyITPP
#if defined TARGET_HAS_ThirdPartyITPP

#include <itpp/itbase.h>

#define OVP_ClassId_Algorithm_ClassifierLDA                                        OpenViBE::CIdentifier(0xD7183FC7, 0xBD74F298)
#define OVP_ClassId_Algorithm_ClassifierLDADesc                                    OpenViBE::CIdentifier(0xD42D544A, 0x7A28DDB1)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CAlgorithmClassifierLDA : public OpenViBEToolkit::CAlgorithmClassifier
		{
		public:

			virtual OpenViBE::boolean initialize(void);

			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual XML::IXMLNode* saveConfiguration(void);
			virtual OpenViBE::boolean loadConfiguration(XML::IXMLNode* pConfiguratioNode);

			virtual OpenViBE::uint32 getBestClassification(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierLDA);

		protected:

			OpenViBE::float64 m_f64Class1;
			OpenViBE::float64 m_f64Class2;

			itpp::vec m_oCoefficients;

			XML::IXMLNode *m_pConfigurationNode;

		private:
			void loadClassesFromNode(XML::IXMLNode *pNode);
			void loadCoefficientsFromNode(XML::IXMLNode *pNode);

			void generateConfigurationNode(void);
		};

		class CAlgorithmClassifierLDADesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("LDA classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard / Fabien Lotte"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA / INSA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierLDA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierLDA; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierLDADesc);
		};
	};
};

#endif // TARGET_HAS_ThirdPartyITPP

#endif // __OpenViBEPlugins_Algorithm_ClassifierLDA_H__
