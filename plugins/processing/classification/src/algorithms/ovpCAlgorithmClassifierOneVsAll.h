#ifndef __OpenViBEPlugins_Algorithm_OneVsAll_H__
#define __OpenViBEPlugins_Algorithm_OneVsAll_H__

// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <system/Memory.h>

#include <xml/IWriter.h>
#include <xml/IReader.h>
#include <xml/IXMLNode.h>

#include <stack>
#include <vector>
#include <iostream>

#define OVP_ClassId_Algorithm_ClassifierOneVsAll                                        OpenViBE::CIdentifier(0xD7183FC6, 0xBD74F297)
#define OVP_ClassId_Algorithm_ClassifierOneVsAllDesc                                    OpenViBE::CIdentifier(0xD42D5449, 0x7A28DDB0)

#define OVP_Algorithm_ClassifierOneVsAll_SubClassifierId                                OpenViBE::CIdentifier(0x2ABAA20F, 0xB5C375EF)

namespace OpenViBEPlugins
{
	namespace Local
	{
		class CAlgorithmClassifierOneVsAll : public OpenViBEToolkit::CAlgorithmPairingStrategy, public XML::IWriterCallback, public XML::IReaderCallback
		{
		public:

			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);
			virtual OpenViBE::boolean designArchitecture(OpenViBE::CIdentifier &rId, OpenViBE::uint64& rClassAmount);

			virtual XML::IXMLNode* saveConfiguration(void);
			virtual OpenViBE::boolean loadConfiguration(XML::IXMLNode *pConfigurationNode);
			virtual OpenViBE::uint32 getBestClassification(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::CAlgorithmPairingStrategy, OVP_ClassId_Algorithm_ClassifierOneVsAll)

		protected:

			virtual void write(const char* sString); // XML IWriterCallback

			XML::IXMLNode* getClassifierConfiguration(OpenViBE::Kernel::IAlgorithmProxy* classifier);
			virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
			virtual void processChildData(const char* sData); // XML IReaderCallback
			virtual void closeChild(void); // XML ReaderCallback


			std::stack<OpenViBE::CString> m_vNode;

			std::vector<OpenViBE::Kernel::IAlgorithmProxy*> m_oSubClassifierList;
			OpenViBE::uint64 m_iClassCounter; //This variable is use during configuration loading

			OpenViBE::CMemoryBuffer m_oConfiguration;
			XML::IXMLNode *m_pConfigurationNode;

		private:
			void addNewClassifierAtBack(void);
			void removeClassifierAtBack(void);

			void loadSubClassifierConfiguration(XML::IXMLNode *pSubClassifiersNode);
		};

		class CAlgorithmClassifierOneVsAllDesc : public OpenViBEToolkit::CAlgorithmPairingStrategyDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("OneVsAll pairing classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Guillaume Serriere"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA / INSA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierOneVsAll; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Local::CAlgorithmClassifierOneVsAll; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
					OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmPairingStrategyDesc::getAlgorithmPrototype(rAlgorithmPrototype);

				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierOneVsAll_SubClassifierId,"Sub Classifier type",OpenViBE::Kernel::ParameterType_Enumeration, OVTK_TypeId_ClassificationAlgorithm);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmPairingStrategyDesc, OVP_ClassId_Algorithm_ClassifierOneVsAllDesc);
		};
	};
};

#endif // __OpenViBEPlugins_Algorithm_OneVsAll_H__
