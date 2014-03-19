#ifndef __OpenViBEPlugins_Algorithm_OneVsOne_H__
#define __OpenViBEPlugins_Algorithm_OneVsOne_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>

#include <vector>

#define OVP_ClassId_Algorithm_ClassifierOneVsOne                             OpenViBE::CIdentifier(0x638C2F90, 0xEAE10226)
#define OVP_ClassId_Algorithm_ClassifierOneVsOneDesc                         OpenViBE::CIdentifier(0xE78E7CDB, 0x369AA9EF)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CAlgorithmClassifierOneVsOne : public OpenViBEToolkit::CAlgorithmPairingStrategy
		{
		public:

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);
			virtual OpenViBE::boolean designArchitecture(OpenViBE::CIdentifier &rId, OpenViBE::int64& rClassAmount);

			virtual XML::IXMLNode* saveConfiguration(void);
			virtual OpenViBE::boolean loadConfiguration(XML::IXMLNode *pConfigurationNode);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::CAlgorithmPairingStrategy, OVP_ClassId_Algorithm_ClassifierOneVsOne)

		protected:
			std::vector<OpenViBE::Kernel::IAlgorithmProxy*> m_oSubClassifierList;

			XML::IXMLNode *m_pConfigurationNode;

		private:
			XML::IXMLNode* getClassifierConfiguration(OpenViBE::Kernel::IAlgorithmProxy* classifier);
			OpenViBE::uint32 getClassAmount(void) const;

			void loadSubClassifierConfiguration(XML::IXMLNode *pSubClassifiersNode);
			void generateConfigurationNode(void);
		};

		class CAlgorithmClassifierOneVsOneDesc : public OpenViBEToolkit::CAlgorithmPairingStrategyDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("OneVsOne pairing classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Guillaume Serriere"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/Loria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierOneVsOne; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierOneVsOne; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
					OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmPairingStrategyDesc::getAlgorithmPrototype(rAlgorithmPrototype);

				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmPairingStrategyDesc, OVP_ClassId_Algorithm_ClassifierOneVsOneDesc)
		};
	}
}

#endif // __OpenViBEPlugins_Algorithm_OneVsAll_H__
