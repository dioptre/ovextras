#ifndef __OpenViBEPlugins_Algorithm_OneVsAll_H__
#define __OpenViBEPlugins_Algorithm_OneVsAll_H__

// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <toolkit/algorithms/classification/ovtkCAlgorithmPairingStrategy.h>

#include <xml/IWriter.h>
#include <xml/IReader.h>

#include <stack>

#define OVP_ClassId_Algorithm_ClassifierOneVsAll                                        OpenViBE::CIdentifier(0xD7183FC6, 0xBD74F297)
#define OVP_ClassId_Algorithm_ClassifierOneVsAllDesc                                    OpenViBE::CIdentifier(0xD42D5449, 0x7A28DDB0)

namespace OpenViBEPlugins
{
    namespace Local
    {
        class CAlgorithmClassifierOneVsAll : public OpenViBEToolkit::CAlgorithmPairingStrategy
        {
        public:

            virtual OpenViBE::boolean train(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
            virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);
            virtual OpenViBE::boolean designArchitecture(OpenViBE::CIdentifier &rId, OpenViBEToolkit::IVector& rClassesList);

            virtual OpenViBE::boolean saveConfiguration(OpenViBE::IMemoryBuffer& rMemoryBuffer);
            virtual OpenViBE::boolean loadConfiguration(const OpenViBE::IMemoryBuffer& rMemoryBuffer);

            _IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierOneVsAll);

        protected:

            virtual void write(const char* sString); // XML IWriterCallback

            virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
            virtual void processChildData(const char* sData); // XML IReaderCallback
            virtual void closeChild(void); // XML ReaderCallback

            std::stack<OpenViBE::CString> m_vNode;

            OpenViBE::float64 m_f64Class1;
            OpenViBE::float64 m_f64Class2;

            OpenViBE::CMemoryBuffer m_oConfiguration;
        };

        class CAlgorithmClassifierOneVsAllDesc : public OpenViBEToolkit::CAlgorithmClassifierDesc
        {
        public:

            virtual void release(void) { }

            virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("OneVsAll pairing classifier"); }
            virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard / Fabien Lotte"); }
            virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA / INSA/IRISA"); }
            virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
            virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
            virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
            virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

            virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_ClassifierOneVsAll; }
            virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Local::CAlgorithmClassifierOneVsAll; }

            virtual OpenViBE::boolean getAlgorithmPrototype(
                OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
            {
                CAlgorithmClassifierDesc::getAlgorithmPrototype(rAlgorithmPrototype);
                return true;
            }

            _IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierOneVsAllDesc);
        };
    };
};

#endif // __OpenViBEPlugins_Algorithm_OneVsAll_H__
