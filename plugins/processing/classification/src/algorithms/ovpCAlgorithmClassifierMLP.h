#ifndef __OpenViBEPlugins_Algorithm_ClassifierMLP_H__
#define __OpenViBEPlugins_Algorithm_ClassifierMLP_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

//#if defined TARGET_HAS_ThirdPartyEIGEN

#define OVP_ClassId_Algorithm_ClassifierLDA                                        OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401260)
#define OVP_ClassId_Algorithm_ClassifierLDA_DecisionAvailable                      OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401261)
#define OVP_ClassId_Algorithm_ClassifierLDADesc                                    OpenViBE::CIdentifier(0xF3FAB4BE, 0xDC401262)


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
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierMLPDesc)
		};
	}
}


//#endif // TARGET_HAS_ThirdPartyEIGEN

#endif // __OpenViBEPlugins_Algorithm_ClassifierMLP_H__


