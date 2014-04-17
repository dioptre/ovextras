
// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#ifndef __OpenViBEPlugins_Algorithm_ClassifierPLDA_H__
#define __OpenViBEPlugins_Algorithm_ClassifierPLDA_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>
#if defined TARGET_HAS_ThirdPartyITPP

#include <itpp/itbase.h>

#define OVP_ClassId_Algorithm_ClassifierPLDA                                        OpenViBE::CIdentifier(0xA7A8FB40, 0xD85B90B8)
#define OVP_ClassId_Algorithm_ClassifierPLDADesc                                    OpenViBE::CIdentifier(0x27C51B3C, 0x0907ECEA)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		OpenViBE::int32 getPLDABestClassification(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue);

		class CAlgorithmClassifierPLDA : public OpenViBEToolkit::CAlgorithmClassifier
		{
		public:

			virtual OpenViBE::boolean initialize(void);

			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual XML::IXMLNode* saveConfiguration(void);
			virtual OpenViBE::boolean loadConfiguration(XML::IXMLNode* pConfiguratioNode);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierPLDA)

		protected:

			OpenViBE::float64 m_f64Class1;
			OpenViBE::float64 m_f64Class2;

			itpp::vec m_oCoefficientsClass1;
			itpp::vec m_oCoefficientsClass2;

			XML::IXMLNode *m_pConfigurationNode;

		private:
			void loadClassesFromNode(XML::IXMLNode *pNode);
			void loadCoefficientsFromNode(XML::IXMLNode *pNode);

			void generateConfigurationNode(void);
		};

		class CAlgorithmClassifierPLDADesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Probabilistic LDA classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard / Fabien Lotte"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA / INSA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierPLDA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierPLDA; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierPLDADesc)
		};
	}
}

#endif // TARGET_HAS_ThirdPartyITPP

#endif // __OpenViBEPlugins_Algorithm_ClassifierLDA_H__
