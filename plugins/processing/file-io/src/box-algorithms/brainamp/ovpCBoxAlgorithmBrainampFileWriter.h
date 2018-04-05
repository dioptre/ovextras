#ifndef __OpenViBEPlugins_BoxAlgorithm_BrainampFileWriter_H__
#define __OpenViBEPlugins_BoxAlgorithm_BrainampFileWriter_H__

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <fstream>
#include <map>

namespace OpenViBEPlugins
{
	namespace FileIO
	{
		class CBoxAlgorithmBrainampFileWriter : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_BrainampFileWriter)

		protected:

			OpenViBE::Kernel::IAlgorithmProxy* m_pSignalStreamDecoder;
			OpenViBE::Kernel::IAlgorithmProxy* m_pStimulationStreamDecoder;

			OpenViBE::Kernel::TParameterHandler<const OpenViBE::IMemoryBuffer*> ip_pSignalMemoryBuffer;
			OpenViBE::Kernel::TParameterHandler<const OpenViBE::IMemoryBuffer*> ip_pStimulationsMemoryBuffer;
			OpenViBE::Kernel::TParameterHandler<OpenViBE::IMatrix*> op_pMatrix;
			OpenViBE::Kernel::TParameterHandler<OpenViBE::IStimulationSet*> op_pStimulationSet;
			OpenViBE::Kernel::TParameterHandler<OpenViBE::uint64> op_ui64SamplingRate;

			OpenViBE::CString m_sOutputFileFullPath;
			OpenViBE::CString m_sDictionaryFileName;
			OpenViBE::boolean m_bTransformStimulations;
			bool m_bShouldWriteFullFileNames;

		private:
			std::ofstream m_oHeaderFileStream;
			std::ofstream m_oEEGFileStream;
			std::ofstream m_oMarkerFileStream;

			std::map<OpenViBE::uint64, std::string> m_mStimulationToMarker;

			OpenViBE::uint64 m_ui64MarkersWritten;
			bool m_bWasMarkerHeaderWritten;
		};

		class CBoxAlgorithmBrainampFileWriterDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("BrainVision Format File Writer"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jozef Legeny"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Mensia Technologies"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Writes its input into a BrainVision format file"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("This box allows to write the input signal under BrainVision file format."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("File reading and writing/BrainVision Format"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.1"); }			
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-save"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_BrainampFileWriter; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::FileIO::CBoxAlgorithmBrainampFileWriter; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				// Adds box outputs
				rBoxAlgorithmPrototype.addInput("EEG stream", OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addInput("Stimulations", OV_TypeId_Stimulations);

				// Adds settings
				rBoxAlgorithmPrototype.addSetting("Filename (header in vhdr format)", OV_TypeId_Filename, "record-[$core{date}-$core{time}].vhdr");
				rBoxAlgorithmPrototype.addSetting("Marker to OV Stimulation dictionary", OV_TypeId_Filename, "");
				rBoxAlgorithmPrototype.addSetting("Convert OpenViBE Stimulations to markers", OV_TypeId_Boolean, "true");
				rBoxAlgorithmPrototype.addSetting("Use full data and marker file names in header", OV_TypeId_Boolean, "false");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_BrainampFileWriterDesc)
		};
	}
}

#endif // __OpenViBEPlugins_BoxAlgorithm_BrainampFileWriter_H__
