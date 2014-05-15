
// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#ifndef __OpenViBEPlugins_Algorithm_ClassifierAdaptiveGradientDescentLDA_H__
#define __OpenViBEPlugins_Algorithm_ClassifierAdaptiveGradientDescentLDA_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IWriter.h>
#include <xml/IReader.h>

#include <stack>
#include <vector>

#if defined TARGET_HAS_ThirdPartyITPP

#include <itpp/itbase.h>

#define OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDA			OpenViBE::CIdentifier(0x7D13B924, 0xF194DA09)
#define OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDADesc		OpenViBE::CIdentifier(0x2397E74F, 0xCAC8F95C)

#define OVP_Algorithm_ClassifierGradientDescentLDA_InputParameterId_Lambda	OpenViBE::CIdentifier(0xD6E1AC79, 0xBE50C28F)
#define OVP_Algorithm_ClassifierGradientDescentLDA_InputParameterId_Eta		OpenViBE::CIdentifier(0x5E8D9B97, 0xF82F92D7)

namespace OpenViBEPlugins
{
	namespace Classification
	{
	
		class CAlgorithmClassifierAdaptiveGradientDescentLDA : public OpenViBEToolkit::CAlgorithmClassifier, public XML::IWriterCallback, public XML::IReaderCallback
		{
		public:
			
			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual OpenViBE::boolean saveConfiguration(OpenViBE::IMemoryBuffer& rMemoryBuffer);
			virtual OpenViBE::boolean loadConfiguration(const OpenViBE::IMemoryBuffer& rMemoryBuffer);

			_IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDA);

		protected:

			virtual void write(const char* sString); // XML IWriterCallback

			virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
			virtual void processChildData(const char* sData); // XML IReaderCallback
			virtual void closeChild(void); // XML ReaderCallback

			std::stack<OpenViBE::CString> m_vNode;

			OpenViBE::float64 m_f64Class1;
			OpenViBE::float64 m_f64Class2;
			OpenViBE::uint64 m_ui64TmpLabel;
			OpenViBE::CMemoryBuffer m_oConfiguration;
			itpp::vec m_oCoefficientsClass1;
			itpp::vec m_oCoefficientsClass2;
			
			std::vector<itpp::vec> m_vSampleBuffer;
			std::vector<OpenViBE::uint64> m_vLabelBuffer;
		};

		class CAlgorithmClassifierAdaptiveGradientDescentLDADesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Adaptive Gradient Descent LDA"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Dieter Devlaminck, Eoin Thomas, Emmanuel Dauce"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/INRIA/?"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Adatpvie Gradient Descent LDA as proposed by Emmanuel Dauce and Eoin Thomas"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierAdaptiveGradientDescentLDA; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
				
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierGradientDescentLDA_InputParameterId_Lambda,"Lambda",OpenViBE::Kernel::ParameterType_Float);	
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierGradientDescentLDA_InputParameterId_Eta,"Eta",OpenViBE::Kernel::ParameterType_Float);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDADesc);
		};
	};
};

#endif // TARGET_HAS_ThirdPartyITPP

#endif // __OpenViBEPlugins_Algorithm_ClassifierLDA_H__
