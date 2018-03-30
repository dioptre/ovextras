#include "ovp_defines.h"
#include "box-algorithms/ovpCGDFFileReader.h"
#include "box-algorithms/ovpCGDFFileWriter.h"
#include "box-algorithms/ovpCBCICompetitionIIIbReader.h"

#include "algorithms/brainamp/ovpCAlgorithmBrainampFileReader.h"

#include "box-algorithms/brainamp/ovpCBoxAlgorithmBrainampFileReader.h"
#include "box-algorithms/brainamp/ovpCBoxAlgorithmBrainampFileWriter.h"

#include "box-algorithms/csv/ovpCBoxAlgorithmCSVFileWriter.h"
#include "box-algorithms/csv/ovpCBoxAlgorithmCSVFileReader.h"

#include "box-algorithms/bci2000reader/ovpCBoxAlgorithmBCI2000Reader.h"

#include "box-algorithms/ovpCBoxAlgorithmSignalConcatenation.h"

OVP_Declare_Begin()
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.toUInteger());
	
	OVP_Declare_New(OpenViBEPlugins::FileIO::CGDFFileReaderDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CGDFFileWriterDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBCICompetitionIIIbReaderDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CAlgorithmBrainampFileReaderDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmBrainampFileReaderDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmBrainampFileWriterDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmCSVFileWriterDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmCSVFileReaderDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmBCI2000ReaderDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmSignalConcatenationDesc)

OVP_Declare_End()
