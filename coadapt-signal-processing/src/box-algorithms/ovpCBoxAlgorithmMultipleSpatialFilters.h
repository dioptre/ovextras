#ifndef __OpenViBEPlugins_BoxAlgorithm_MultipleSpatialFilter_H__
#define __OpenViBEPlugins_BoxAlgorithm_MultipleSpatialFilter_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IReader.h>

#include <vector>
#include <stack>

#if defined TARGET_HAS_ThirdPartyITPP

#include <itpp/itbase.h>

namespace OpenViBEPlugins
{
	namespace SignalProcessingCoAdapt
	{
		class CBoxAlgorithmMultipleSpatialFilters : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, public XML::IReaderCallback
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);
			
			virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
			virtual void processChildData(const char* sData); // XML IReaderCallback
			virtual void closeChild(void); // XML ReaderCallback			

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_MultipleSpatialFilters);

		protected:

			OpenViBE::Kernel::IAlgorithmProxy* m_pStreamDecoder;
			OpenViBE::Kernel::TParameterHandler < const OpenViBE::IMemoryBuffer* > ip_pMemoryBuffer;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > op_pMatrix;

			OpenViBE::Kernel::IAlgorithmProxy* m_pStreamEncoder;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pMatrix;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMemoryBuffer* > op_pMemoryBuffer;
			
			//OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmSpatialFilter> m_oStimulationDecoder;
			//OpenViBEToolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter> m_oStreamedMatrixDecoder;

			std::stack<OpenViBE::CString> m_vNode;
			
			std::vector< itpp::Mat<OpenViBE::float64> > m_vCoefficients;
			OpenViBE::uint32 m_ui32NumberOfFilters;
			OpenViBE::uint32 m_ui32OutputChannelCount;
			OpenViBE::uint32 m_ui32InputChannelCount;

		private:
			// Loads the m_vCoefficient vector (representing a matrix) from the given string. c1 and c2 are separator characters between floats.
			//OpenViBE::uint32 loadCoefficients(const OpenViBE::CString &rCoefficients, const char c1, const char c2);

		};

		class CBoxAlgorithmMultipleSpatialFiltersListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			virtual OpenViBE::boolean isValidTypeIdentifier(const OpenViBE::CIdentifier& rTypeIdenfitier)
			{
				OpenViBE::boolean l_bValid=false;
				l_bValid|=(rTypeIdenfitier==OV_TypeId_StreamedMatrix);
				l_bValid|=(rTypeIdenfitier==OV_TypeId_Spectrum);
				l_bValid|=(rTypeIdenfitier==OV_TypeId_Signal);
				return l_bValid;
			}

			virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getInputType(0, l_oTypeIdentifier);
				if(this->isValidTypeIdentifier(l_oTypeIdentifier))
				{
					rBox.setOutputType(0, l_oTypeIdentifier);
				}
				return true;
			}

			virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getOutputType(0, l_oTypeIdentifier);
				if(this->isValidTypeIdentifier(l_oTypeIdentifier))
				{
					rBox.setInputType(0, l_oTypeIdentifier);
				}
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};

		class CBoxAlgorithmMultipleSpatialFiltersDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Multiple Spatial Filters"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard/Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Signal processing/Filtering"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-missing-image"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_MultipleSpatialFilters; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessingCoAdapt::CBoxAlgorithmMultipleSpatialFilters; }
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmMultipleSpatialFiltersListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput  ("Input Signal",                OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addOutput ("Output Signal",               OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addSetting("Filename to load spatial filters from", OV_TypeId_Filename,    "");
				rBoxAlgorithmPrototype.addFlag   (OpenViBE::Kernel::BoxFlag_CanModifyInput);
				rBoxAlgorithmPrototype.addFlag   (OpenViBE::Kernel::BoxFlag_CanModifyOutput);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_MultipleSpatialFiltersDesc);
		};
	};
};

#endif

#endif // __OpenViBEPlugins_BoxAlgorithm_SpatialFilter_H__
