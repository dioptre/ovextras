#include "ovp_defines.h"

/*
#include <openvibe/ov_all.h>
#include <openvibe-toolkit/ovtk_all.h>
*/

#include "ovpCBoxAlgorithmSharedMemoryWriter.h"
#include "ovpCBoxAlgorithmStimulationBasedFileReader.h"

OVP_Declare_Begin();
	OVP_Declare_New(OpenViBEPlugins::FileReadingAndWriting::CBoxAlgorithmSharedMemoryWriterDesc);
	OVP_Declare_New(OpenViBEPlugins::FileReadingAndWriting::CBoxAlgorithmStimulationBasedFileReaderDesc);
OVP_Declare_End();
