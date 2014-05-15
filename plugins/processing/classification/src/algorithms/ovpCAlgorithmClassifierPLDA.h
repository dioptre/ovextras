
// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#ifndef __OpenViBEPlugins_Algorithm_ClassifierPLDA_H__
#define __OpenViBEPlugins_Algorithm_ClassifierPLDA_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IWriter.h>
#include <xml/IReader.h>

#include <stack>
#include <vector>

#if defined TARGET_HAS_ThirdPartyITPP

#include <itpp/itbase.h>


#define OVP_ClassId_Algorithm_ClassifierPLDA                                         OpenViBE::CIdentifier(0x2F9ECA0B, 0x8D3CA7BD)
#define OVP_ClassId_Algorithm_ClassifierPLDADesc                                   OpenViBE::CIdentifier(0x1BD67420, 0x587600E6)

namespace OpenViBEPlugins
{
	namespace Local
	{	
		class CAlgorithmClassifierPLDA : public OpenViBEToolkit::CAlgorithmClassifier, public XML::IWriterCallback, public XML::IReaderCallback
		{
		public:
			
			CAlgorithmClassifierPLDA() : m_ui32BufferPointer(0), m_bBufferFilled(false)
			{	
			}

			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual OpenViBE::boolean saveConfiguration(OpenViBE::IMemoryBuffer& rMemoryBuffer);
			virtual OpenViBE::boolean loadConfiguration(const OpenViBE::IMemoryBuffer& rMemoryBuffer);

			_IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierPLDA);

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
			
			std::vector<itpp::vec> m_vCircularSampleBuffer;
			std::vector<OpenViBE::uint64> m_vCircularLabelBuffer;
			OpenViBE::uint32 m_ui32BufferPointer;		
			OpenViBE::boolean m_bBufferFilled;
			OpenViBE::uint32 m_ui32BufferEnd;
		};

		class CAlgorithmClassifierPLDADesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Probabilistic classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard / Fabien Lotte / Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA / INSA/IRISA / INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("LDA classifier with probabilistic outputs"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierPLDA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Local::CAlgorithmClassifierPLDA; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
				
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Shrinkage,"Shrinkage",OpenViBE::Kernel::ParameterType_Enumeration, OVP_TypeId_ShrinkageType);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Lambda,"Lambda",OpenViBE::Kernel::ParameterType_Float);			
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierPLDADesc);
		};
	};
};

#endif // TARGET_HAS_ThirdPartyITPP

#endif // __OpenViBEPlugins_Algorithm_ClassifierLDA_H__
