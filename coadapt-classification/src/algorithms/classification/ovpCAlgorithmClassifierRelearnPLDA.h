
// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#ifndef __OpenViBEPlugins_Algorithm_ClassifierRelearnPLDA_H__
#define __OpenViBEPlugins_Algorithm_ClassifierRelearnPLDA_H__

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IWriter.h>
#include <xml/IReader.h>

#include <stack>
#include <vector>

#if defined TARGET_HAS_ThirdPartyITPP

#include <itpp/itbase.h>

namespace OpenViBEPlugins
{
	namespace Classification
	{
	
		class CAlgorithmClassifierRelearnPLDA : public OpenViBEToolkit::CAlgorithmClassifier, public XML::IWriterCallback, public XML::IReaderCallback
		{
		public:
			
			CAlgorithmClassifierRelearnPLDA() : m_ui32BufferPointer(0), m_bBufferFilled(false)
			{	
				if (m_ui32BufferPointer!=0 || m_bBufferFilled)
					this->getLogManager() << OpenViBE::Kernel::LogLevel_Warning << "m_ui32BufferPointer not initialised to zero or m_bBufferFilled initialised to true\n";
			}

			virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual OpenViBE::boolean saveConfiguration(OpenViBE::IMemoryBuffer& rMemoryBuffer);
			virtual OpenViBE::boolean loadConfiguration(const OpenViBE::IMemoryBuffer& rMemoryBuffer);

			_IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierRelearnPLDA);

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
			OpenViBE::uint32 m_ui32BufferSize;
		};

		class CAlgorithmClassifierRelearnPLDADesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Probabilistic classifier with relearning"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard / Fabien Lotte / Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA / INSA/IRISA / INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("LDA classifier with probabilistic outputs"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierRelearnPLDA; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmClassifierRelearnPLDA; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
				
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Shrinkage,"Shrinkage",OpenViBE::Kernel::ParameterType_Enumeration, OVP_TypeId_ShrinkageType);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Lambda,"Lambda",OpenViBE::Kernel::ParameterType_Float);	
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_BufferSize,"Buffer size",OpenViBE::Kernel::ParameterType_Integer);
				return true;
			}

			_IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierRelearnPLDADesc);
		};
	};
};

#endif // TARGET_HAS_ThirdPartyITPP

#endif // __OpenViBEPlugins_Algorithm_ClassifierLDA_H__
