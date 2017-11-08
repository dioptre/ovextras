#if defined TARGET_HAS_ThirdPartyMatlab

#include "ovpCBoxAlgorithmMatlabScripting.h"

#include <system/ovCMemory.h>
#include <system/ovCMath.h>

#include <iostream>
#include <stdio.h>
#include <sstream>
#include <string>
#include <ctime>
#include <algorithm>

#include <mex.h>
#include <engine.h>

#include <fs/Files.h>

#if defined TARGET_OS_Windows
	#include <windows.h>
	#include <direct.h>
	#define getCurrentDir _getcwd
#else
	#include <unistd.h>
	#define getCurrentDir getcwd
#endif


// Size of the internal buffer storing matlab messages, in char
#define MATLAB_BUFFER 2048

#define m_pMatlabEngine ((Engine*)m_pMatlabEngineHandle)

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace OpenViBEToolkit;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Matlab;


using namespace std;

// Sanitizes a path so that it only has / or \ characters and has a / or \ in the end.
// @fixme should move to plugins-FS possibly
void CBoxAlgorithmMatlabScripting::sanitizePath(OpenViBE::CString &sPathToModify) const {

	std::string l_oTmpPath(sPathToModify);
	// Append / to end of path if its not there already
	if(l_oTmpPath.length()>0) 
	{
		char l_cLastChar = l_oTmpPath.at(l_oTmpPath.length()-1);
		if(l_cLastChar != '\\' && l_cLastChar != '/')
		{
			l_oTmpPath += "/";
		}
	}

#if defined TARGET_OS_Windows
	// Convert '/' to '\'
	std::replace(l_oTmpPath.begin(), l_oTmpPath.end(), '/', '\\');
#endif
	sPathToModify = OpenViBE::CString(l_oTmpPath.c_str());
}

// The checkFailureRoutine() verifies the result of a matlab call (via engine or helper functions) given as argument.
// If the result is false (the matlab call failed), the message msg is printed in the Error Log Level, and the macro returns false.
// The Matlab output buffer is then printed. If an error message is detected in the buffer, the same error message is printed and
// the macro returns false. 
bool CBoxAlgorithmMatlabScripting::checkFailureRoutine(bool bResult, const OpenViBE::CString &msg) 
{
	OV_ERROR_UNLESS_KRF(
		bResult,
		msg,
		OpenViBE::Kernel::ErrorType::BadProcessing);

	m_bErrorDetected = false;
	this->printOutputBufferWithFormat();
	OV_ERROR_UNLESS_KRF(
		!m_bErrorDetected,
		msg,
		OpenViBE::Kernel::ErrorType::BadProcessing);

	return true;
}

bool CBoxAlgorithmMatlabScripting::OpenMatlabEngineSafely(void)
{
	this->getLogManager() << LogLevel_Trace << "Trying to open the Matlab engine\n";
#if defined TARGET_OS_Linux
	m_pMatlabEngineHandle=::engOpen(m_sMatlabPath.toASCIIString());
	OV_ERROR_UNLESS_KRF(
		m_pMatlabEngine,
		"Could not open the Matlab engine.\n" <<
			"The configured path to the matlab executable was expanded as '" << m_sMatlabPath << "'.",
		OpenViBE::Kernel::ErrorType::BadProcessing);
	}
#elif defined TARGET_OS_Windows
	__try
	{
		m_pMatlabEngineHandle=::engOpen(NULL);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		this->getLogManager() << LogLevel_Error << "First call to the MATLAB engine failed.\n"
			<< "\tTo use this box you must have MATLAB (32 bits version) installed on your computer.\n";
		m_pMatlabEngineHandle = NULL;
	}
	if(!m_pMatlabEngine)
	{
		this->getLogManager() << LogLevel_Error << "Could not open the Matlab engine.\n" << 
			"The matlab binary path was reasoned to be '" << m_sMatlabPath << "'.\n";
		return false;
	}
#else
	OV_ERROR_KRF("Only Linux and Windows are supported", OpenViBE::Kernel::ErrorType::BadVersion);
#endif
	return true;
}

bool CBoxAlgorithmMatlabScripting::initialize(void)
{
	m_pMatlabEngineHandle = NULL;
	m_sMatlabBuffer = NULL;
	m_oMatlabHelper = new CMatlabHelper(this->getLogManager(), this->getErrorManager());

	m_ui64ClockFrequency = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
		
	for(uint32_t i=0; i<getStaticBoxContext().getInputCount(); i++)
	{
		CIdentifier l_oInputType;
		getStaticBoxContext().getInputType(i, l_oInputType);
		TDecoder<CBoxAlgorithmMatlabScripting> * l_pDecoder = NULL;
		if(l_oInputType==OV_TypeId_StreamedMatrix)
		{
			l_pDecoder= new TStreamedMatrixDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oInputType==OV_TypeId_ChannelLocalisation)
		{
			l_pDecoder= new TChannelLocalisationDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oInputType==OV_TypeId_FeatureVector)
		{
			l_pDecoder= new TFeatureVectorDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oInputType==OV_TypeId_Spectrum)
		{
			l_pDecoder= new TSpectrumDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oInputType==OV_TypeId_Signal)
		{
			l_pDecoder= new TSignalDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oInputType==OV_TypeId_Stimulations)
		{
			l_pDecoder= new TStimulationDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oInputType==OV_TypeId_ExperimentInformation)
		{
			l_pDecoder= new TExperimentInformationDecoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else
		{
			this->getLogManager() << LogLevel_Warning << "Undefined type on input ["<<i<<"].\n";
		}
		if(l_pDecoder != NULL)
		{
			m_mDecoders.insert(make_pair(i,l_pDecoder));
			m_NbInputHeaderSent = 0;
		}
	}

	for(uint32_t i=0; i<getStaticBoxContext().getOutputCount(); i++)
	{
		CIdentifier l_oOutputType;
		getStaticBoxContext().getOutputType(i, l_oOutputType);
		TEncoder<CBoxAlgorithmMatlabScripting> * l_pEncoder = NULL;
		if(l_oOutputType==OV_TypeId_StreamedMatrix)
		{
			l_pEncoder= new TStreamedMatrixEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oOutputType==OV_TypeId_ChannelLocalisation)
		{
			l_pEncoder= new TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oOutputType==OV_TypeId_FeatureVector)
		{
			l_pEncoder= new TFeatureVectorEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oOutputType==OV_TypeId_Spectrum)
		{
			l_pEncoder= new TSpectrumEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oOutputType==OV_TypeId_Signal)
		{
			l_pEncoder= new TSignalEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oOutputType==OV_TypeId_Stimulations)
		{
			l_pEncoder= new TStimulationEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else if(l_oOutputType==OV_TypeId_ExperimentInformation)
		{
			l_pEncoder= new TExperimentInformationEncoder<CBoxAlgorithmMatlabScripting>(*this,i);
		}
		else
		{
			this->getLogManager() << LogLevel_Warning << "Undefined type on input ["<<i<<"].\n";
		}
		if(l_pEncoder != NULL)
		{
			m_mEncoders.insert(make_pair(i,l_pEncoder));
			m_mOutputHeaderState.insert(make_pair(i,false));
		}
	}

	m_sMatlabPath = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

#if defined TARGET_OS_Windows
	if(!FS::Files::directoryExists(m_sMatlabPath) && FS::Files::fileExists(m_sMatlabPath)) 
	{
		// The path might be pointing to the executable, try to extract the directory
		char l_sParentPath[MAX_PATH];
		FS::Files::getParentPath(m_sMatlabPath, l_sParentPath);
		m_sMatlabPath = OpenViBE::CString(l_sParentPath);
	}

	sanitizePath(m_sMatlabPath);

	this->getLogManager() << LogLevel_Trace << "Interpreting Matlab path as '" << m_sMatlabPath << "'\n";

	OV_ERROR_UNLESS_KRF(
		FS::Files::directoryExists(m_sMatlabPath),
		"Configured Matlab path '" << m_sMatlabPath << "' does not seem to be a directory",
		OpenViBE::Kernel::ErrorType::BadConfig);

	char * l_sPath = getenv("PATH");
	OV_ERROR_UNLESS_KRF(
		l_sPath,
		"Could not access the environment variable PATH to add Matlab path to it.",
		OpenViBE::Kernel::ErrorType::BadSetting);

	string l_sStrPath = string(l_sPath);
	size_t l_pFound = l_sStrPath.find(m_sMatlabPath.toASCIIString());
	if( l_pFound == string::npos )
	{
		CString l_sCommandForPathModification = l_sPath + CString(";") + m_sMatlabPath;
		OV_ERROR_UNLESS_KRF(
			_putenv_s("PATH",l_sCommandForPathModification) == 0,
			"Failed to modify the environment PATH with the Matlab path.",
			OpenViBE::Kernel::ErrorType::BadProcessing);

		this->getLogManager() << LogLevel_Trace << "Matlab Path '" << m_sMatlabPath << "' added to Windows PATH environment variable.\n";
	}
	else
	{
		this->getLogManager() << LogLevel_Trace << "No need to add matlab to PATH\n";
	}
#endif

	if(!OpenMatlabEngineSafely()) return false;
	
	m_sMatlabBuffer = new char[MATLAB_BUFFER+1];
	m_sMatlabBuffer[MATLAB_BUFFER]='\0';
	::engOutputBuffer(m_pMatlabEngine, m_sMatlabBuffer, MATLAB_BUFFER);
	m_bErrorDetected = false;

	// add the openvibe toolbox to matlab path
	char l_sCurrentDir[FILENAME_MAX];
	OV_ERROR_UNLESS_KRF(
		getCurrentDir(l_sCurrentDir,FILENAME_MAX),
		"Failed to get the execution directory.",
		OpenViBE::Kernel::ErrorType::BadProcessing);

	std::string curDirString = l_sCurrentDir;
	std::replace(curDirString.begin(), curDirString.end(), '\\', '/');

	this->getConfigurationManager().createConfigurationToken(CString("Path_Bin_Abs"),CString(curDirString.c_str()));
	
	CString l_sCommand = CString("addpath('") + OpenViBE::Directories::getDataDir() + "/plugins/matlab');";
	::engEvalString(m_pMatlabEngine, (const char * )l_sCommand);
	//if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, (const char * )l_sOpenvibeToolboxPath) == 0, "An error occurred while adding the path to openvibe toolbox\n")) return false;
	// If there is more than 1 Matlab box in the scenario, the path is set repeatedly
	// resulting in warning messages in the buffer. We don't print them.
	// this->printOutputBufferWithFormat(); 

	CString l_sWorkingDir = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	sanitizePath(l_sWorkingDir);

	// Try to find an unused box instance variable name
	while (true)
	{
		char l_sBuffer[1024];
		sprintf(l_sBuffer, "OV_BOX_0x%08X_0x%08X", System::Math::randomUInteger32(), System::Math::randomUInteger32());
		m_sBoxInstanceVariableName = CString(l_sBuffer);
		this->getLogManager() << LogLevel_Trace << "Checking if variable " << m_sBoxInstanceVariableName << " is in use...\n";
		mxArray * l_pTmp = engGetVariable(m_pMatlabEngine, m_sBoxInstanceVariableName.toASCIIString());
		if (!l_pTmp) {
			// Found unused name
			break;
		}
		mxDestroyArray(l_pTmp);
	}
	this->getLogManager() << LogLevel_Trace << "Selected variable name " << m_sBoxInstanceVariableName << "\n";

	this->getLogManager() << LogLevel_Trace << "Setting working directory to " << l_sWorkingDir << "\n";
	l_sCommand = CString("cd '") + l_sWorkingDir + CString("'");
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while changing the working directory\n")) return false;

	// executes the pre-run routine that defines the global identifiers for streams and stimulations codes
	l_sCommand = CString("run '") + OpenViBE::Directories::getDataDir() + "/plugins/matlab/OV_define.m'";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while calling OV_define.m")) return false;
	
	l_sCommand = CString("run '") + OpenViBE::Directories::getDataDir() + "/plugins/matlab/OV_stimulations.m'";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while calling OV_stimulations.m")) return false;

	// executes the pre-run routine that construct the ov_box object
	char l_sInputCount[32]; sprintf(l_sInputCount, "%i", this->getStaticBoxContext().getInputCount());
	char l_sOutputCount[32]; sprintf(l_sOutputCount, "%i", this->getStaticBoxContext().getOutputCount());
	l_sCommand = m_sBoxInstanceVariableName + " = OV_createBoxInstance(" + l_sInputCount + "," + l_sOutputCount + ");";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while calling OV_createBoxInstance.m")) return false;
	
	//First call to a function of the openvibe toolbox
	// if it fails, the toolbox may be not installed
	mxArray * l_pBox = engGetVariable(m_pMatlabEngine,(const char*)m_sBoxInstanceVariableName);
	OV_ERROR_UNLESS_KRF(
		l_pBox,
		"Failed to create the box instance with OV_createBoxInstance function.",
		OpenViBE::Kernel::ErrorType::BadProcessing);
	
	m_sInitializeFunction = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	m_sProcessFunction = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_sUninitializeFunction = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

	// Check that the files actually exist
	CString l_sFilename;
	l_sFilename = l_sWorkingDir + m_sInitializeFunction + ".m";
	OV_ERROR_UNLESS_KRF(
		FS::Files::fileExists(l_sFilename.toASCIIString()),
		"Cannot open '" << l_sFilename << "'",
		OpenViBE::Kernel::ErrorType::BadFileRead);
	l_sFilename = l_sWorkingDir + m_sProcessFunction + ".m";
	OV_ERROR_UNLESS_KRF(
		FS::Files::fileExists( l_sFilename.toASCIIString()),
		"Cannot open '" << l_sFilename << "'",
		OpenViBE::Kernel::ErrorType::BadFileRead);
	l_sFilename = l_sWorkingDir + m_sUninitializeFunction + ".m";
	OV_ERROR_UNLESS_KRF(
		FS::Files::fileExists( l_sFilename.toASCIIString()),
		"Cannot open '" << l_sFilename << "'",
		OpenViBE::Kernel::ErrorType::BadFileRead);

	CString l_sSettingNames  = "{";
	CString l_sSettingTypes  = "{";
	CString l_sSettingValues = "{";
	CString l_sTemp;
	CIdentifier l_oType;

	for(uint32_t i = 6; i < getStaticBoxContext().getSettingCount(); i++)
	{
		// get the setting name
		getStaticBoxContext().getSettingName(i,l_sTemp);
		l_sSettingNames = l_sSettingNames + "'" + l_sTemp + "' ";
		
		//setting type
		getStaticBoxContext().getSettingType(i,l_oType);
		stringstream ss;
		ss << l_oType.toUInteger();
		l_sSettingTypes = l_sSettingTypes + "uint64("+CString(ss.str().c_str()) + ") ";
		
		//setting value, typed
		
		l_sTemp = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
		if(l_oType == OV_TypeId_Stimulation)
		{
			uint64 l_oStimCode  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),i);
			stringstream ss1;
			ss1 << l_oStimCode;
			l_sSettingValues = l_sSettingValues +  CString(ss1.str().c_str()) + CString(" ");
			// we keep the stimulation codes as doubles, to be able to put them in arrays with other doubles (such as timings). 
			// they are still comparable with uint64 matlab values.
		}
		else
		{
			if(l_oType == OV_TypeId_Stimulation || l_oType == OV_TypeId_Boolean || l_oType == OV_TypeId_Integer || l_oType == OV_TypeId_Float)
			{
				l_sSettingValues = l_sSettingValues + l_sTemp + " "; // we store the value, these types are readable by matlab directly
			}
			else
			{
				l_sSettingValues = l_sSettingValues + "'" + l_sTemp + "' "; // we store them as matlab strings using '
			}
		}
		

	}
	l_sSettingNames  +=  "}";
	l_sSettingTypes += "}";
	l_sSettingValues += "}";

	// On Windows, Matlab doesn't sometimes notice .m files have been changed, esp. if you have matlab box running while you change them
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, "clear functions;") == 0, "An error occurred while calling matlab 'clear functions;'\n")) return false;

	l_sCommand = m_sBoxInstanceVariableName + " = OV_setSettings("+m_sBoxInstanceVariableName+"," + l_sSettingNames + "," + l_sSettingTypes +"," + l_sSettingValues +");";
	//this->getLogManager() << LogLevel_Error << l_sCommand << "\n";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, (const char *)l_sCommand)==0,"Error calling [OV_setSettings]\n")) return false;

	// we set the box clock frequency in the box structure, so it's accessible in the user scripts if needed
	l_sTemp = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	l_sCommand = m_sBoxInstanceVariableName + ".clock_frequency = " + l_sTemp + ";";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while setting the clock frequency\n")) return false;

	l_sCommand = m_sBoxInstanceVariableName + " = " + m_sInitializeFunction + "(" + m_sBoxInstanceVariableName + ");";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while calling the initialize function\n")) return false;

	m_oMatlabHelper->setMatlabEngine(m_pMatlabEngine);
	m_oMatlabHelper->setBoxInstanceVariableName(m_sBoxInstanceVariableName);

	return true;
}

bool CBoxAlgorithmMatlabScripting::CloseMatlabEngineSafely(void)
{
	if(m_pMatlabEngine == NULL)
	{
		return true;
	}
	this->getLogManager() << LogLevel_Trace << "Trying to close Matlab engine\n";
#if defined TARGET_OS_Windows
	__try
	{
#endif
		if(m_pMatlabEngine)
		{
			if(::engClose(m_pMatlabEngine)!=0)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Could not close Matlab engine.\n";
			}
		}
#if defined TARGET_OS_Windows
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		this->getLogManager() << LogLevel_Error << "Closing MATLAB engine failed.\n"
			<< "\tTo use this box you must have MATLAB (32 bits version) installed on your computer.\n";

		return false;
	}
#endif

	return true;
}

bool CBoxAlgorithmMatlabScripting::uninitialize(void)
{
	delete m_oMatlabHelper;
	if(m_pMatlabEngine != NULL)
	{
		CString l_sCommand = m_sBoxInstanceVariableName + " = " + m_sUninitializeFunction + "(" + m_sBoxInstanceVariableName + ");";
		if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while calling the uninitialize function\n")) 
		{ 
			// NOP, we still want to deallocate below
		} 
		// Remove the box from the matlab workspace
		l_sCommand = "clear " + m_sBoxInstanceVariableName + ";";
		if (!checkFailureRoutine(::engEvalString(m_pMatlabEngine, l_sCommand) == 0, "An error occurred while clearing the box\n"))
		{
			// NOP, we still want to deallocate below
		}
	}

	CloseMatlabEngineSafely();
	if(m_sMatlabBuffer) 
	{
		delete[] m_sMatlabBuffer;
		m_sMatlabBuffer = NULL;
	}

	for(uint32_t i = 0; i< m_mDecoders.size(); i++)
	{
		m_mDecoders[i]->uninitialize();
		delete m_mDecoders[i];
	}
	for(uint32_t i = 0; i< m_mEncoders.size(); i++)
	{
		m_mEncoders[i]->uninitialize();
		delete m_mEncoders[i];
	}

	return true;
}

uint64 CBoxAlgorithmMatlabScripting::getClockFrequency(void)
{
	return m_ui64ClockFrequency<<32;
}

bool CBoxAlgorithmMatlabScripting::processClock(IMessageClock& rMessageClock)
{
	if(!m_pMatlabEngine)
	{
		return true;
	}

	std::string l_sCommand = std::string(m_sBoxInstanceVariableName)
			+ ".clock = " + std::to_string((this->getPlayerContext().getCurrentTime()>>16)/65536.) + ";";
	OV_ERROR_UNLESS_KRF(
		::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0,
		"An error occurred while updating the box clock.",
		OpenViBE::Kernel::ErrorType::BadProcessing);

	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

bool CBoxAlgorithmMatlabScripting::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	
	for(uint32_t i = 0; i < getStaticBoxContext().getInputCount(); i++)
	{
		for(uint32_t j = 0; j < l_rDynamicBoxContext.getInputChunkCount(i); j++)
		{
			m_mDecoders[i]->decode(j);
			
			CIdentifier l_oType;
			getStaticBoxContext().getInputType(i,l_oType);
			bool l_bUnknownStream = true;
			bool l_bReceivedSomething = false;

			if(m_mDecoders[i]->isHeaderReceived())
			{
				l_bReceivedSomething = true;
				// this->getLogManager() << LogLevel_Debug << "Received header\n";

				if(l_oType == OV_TypeId_StreamedMatrix)
				{
					IMatrix * l_pMatrix = ((TStreamedMatrixDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputMatrix();
					if(!checkFailureRoutine(m_oMatlabHelper->setStreamedMatrixInputHeader(i,l_pMatrix),"Error calling [OV_setStreamMatrixInputHeader]\n")) return false ;
					
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_Signal)
				{
					IMatrix * l_pMatrix = ((TSignalDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputMatrix();
					uint64 l_ui64SamplingRate = ((TSignalDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputSamplingRate();
					if(!checkFailureRoutine(m_oMatlabHelper->setSignalInputHeader(i,l_pMatrix,l_ui64SamplingRate),"Error calling [OV_setSignalInputHeader]\n")) return false;
					
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_FeatureVector)
				{
					IMatrix * l_pMatrix = ((TFeatureVectorDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputMatrix();
					if(!checkFailureRoutine(m_oMatlabHelper->setFeatureVectorInputHeader(i,l_pMatrix),"Error calling [OV_setFeatureVectorInputHeader]\n")) return false;
					
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_Spectrum)
				{
					IMatrix * l_pMatrix    = ((TSpectrumDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputMatrix();
					IMatrix * l_pFreqAbscissa = ((TSpectrumDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputFrequencyAbscissa();
					uint64_t l_pSamplingRate = ((TSpectrumDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputSamplingRate();

					if(!checkFailureRoutine(m_oMatlabHelper->setSpectrumInputHeader(i, l_pMatrix, l_pFreqAbscissa, l_pSamplingRate),"Error calling [OV_setSpectrumInputHeader]\n")) return false;
					
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_ChannelLocalisation)
				{
					IMatrix * l_pMatrix = ((TChannelLocalisationDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputMatrix();
					bool l_bDynamic  = ((TChannelLocalisationDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputDynamic();
					if(!checkFailureRoutine(m_oMatlabHelper->setChannelLocalisationInputHeader(i,l_pMatrix,l_bDynamic),"Error calling [OV_setChannelLocalizationInputHeader]\n")) return false;
					
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_ExperimentInformation)
				{
					OV_WARNING_K("The Experiment Information Stream is not implemented with the Matlab Scripting Box. If this is relevant for your usage, please contact the official development Team.");
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_Stimulations)
				{
					if(!checkFailureRoutine(m_oMatlabHelper->setStimulationsInputHeader(i),"Error calling [OV_setStimulationsInputHeader]\n")) return false;
					
					m_NbInputHeaderSent++;
					l_bUnknownStream = false;
				}
			} 
			
			if(m_mDecoders[i]->isBufferReceived())
			{
				// this->getLogManager() << LogLevel_Debug << "Received buffer\n";
				l_bReceivedSomething = true;
				// 
				if(l_oType == OV_TypeId_StreamedMatrix || this->getTypeManager().isDerivedFromStream(l_oType, OV_TypeId_StreamedMatrix))
				{
					IMatrix * l_pMatrix = ((TSignalDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputMatrix();
					uint64 l_ui64StartTime = l_rDynamicBoxContext.getInputChunkStartTime(i,j);
					uint64 l_ui64EndTime = l_rDynamicBoxContext.getInputChunkEndTime(i,j);
					if(!checkFailureRoutine(m_oMatlabHelper->addStreamedMatrixInputBuffer(i,l_pMatrix,l_ui64StartTime,l_ui64EndTime),"Error calling [OV_addInputBuffer (Streamed Matrix or child stream)]\n")) return false;
					l_bUnknownStream = false;
				}

				if(l_oType == OV_TypeId_Stimulations)
				{
					IStimulationSet * l_pStimSet = ((TStimulationDecoder<CBoxAlgorithmMatlabScripting> *) m_mDecoders[i])->getOutputStimulationSet();
					uint64 l_ui64StartTime = l_rDynamicBoxContext.getInputChunkStartTime(i,j);
					uint64 l_ui64EndTime = l_rDynamicBoxContext.getInputChunkEndTime(i,j);
					if(l_pStimSet->getStimulationCount() > 0) {
						this->getLogManager() << LogLevel_Trace << "Inserting stimulation set with size " << l_pStimSet->getStimulationCount() << "\n";
					}
					if(!checkFailureRoutine(m_oMatlabHelper->addStimulationsInputBuffer(i,l_pStimSet,l_ui64StartTime,l_ui64EndTime),"Error calling [OV_addInputBuffer (Stimulations)]\n")) return false;
					l_bUnknownStream = false;
				}
				
			} 
			
			if(m_mDecoders[i]->isEndReceived()) 
			{
				this->getLogManager() << LogLevel_Info << "Received end\n";
				l_bReceivedSomething = true;
				l_bUnknownStream = false;
				// @FIXME should something additional be done here?
			}

			OV_ERROR_UNLESS_KRF(
				!l_bReceivedSomething || !l_bUnknownStream,
				"Unknown Stream Type " << l_oType.toString() << " on input ["<<i<<"].",
				OpenViBE::Kernel::ErrorType::BadProcessing);
		}
	}

	if(m_NbInputHeaderSent < getStaticBoxContext().getInputCount())
	{
		// not ready to process 
		return true;
	}


	// CALL TO PROCESS FUNCTION
	std::string command = std::string(m_sBoxInstanceVariableName) + " = " + (const char*)m_sProcessFunction + "(" + (const char*) m_sBoxInstanceVariableName + ");";
	if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, command.c_str()) == 0,"Error calling the Process function.\n")) return false;

	// Go through every output in the matlab box and copy the data to the C++ side
	for(uint32_t i = 0; i < getStaticBoxContext().getOutputCount(); i++)
	{
		// now we check for pending output chunk to be sent (output type independent call)
		command = std::string("OV_PENDING_COUNT_TMP = OV_getNbPendingOutputChunk(") + (const char*) m_sBoxInstanceVariableName + ", " + std::to_string(i + 1) + ");";
		if(!checkFailureRoutine(::engEvalString(m_pMatlabEngine, command.c_str()) == 0,"Error calling [OV_getNbPendingOutputChunk].\n")) return false;

		mxArray * l_pPending = ::engGetVariable(m_pMatlabEngine,"OV_PENDING_COUNT_TMP");
		double   l_dPending  = *mxGetPr(l_pPending);
		
		CIdentifier l_oType;
		getStaticBoxContext().getOutputType(i,l_oType);

		for(uint32_t c = 0; c < (uint32_t)l_dPending; c++)
		{
			// If no header were ever sent, we need to extract header information in the matlab box
			// This header must have been set prior to sending the very first buffer. 
			// @FIXME the practice used below of assigning to getters is nasty, it should be refactored to e.g. using getter/setter pairs
			if(!m_mOutputHeaderState[i])
			{
				bool l_bUnknownType = true;
				if(l_oType == OV_TypeId_StreamedMatrix)
				{
					IMatrix * l_pMatrixToSend = ((TStreamedMatrixEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputMatrix();
					if(!checkFailureRoutine(m_oMatlabHelper->getStreamedMatrixOutputHeader(i,l_pMatrixToSend),"Error calling [OV_getStreamedMatrixOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")) return false;

					l_bUnknownType = false;
				}
				else if(l_oType == OV_TypeId_Signal)
				{
					IMatrix * l_pMatrixToSend = ((TSignalEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputMatrix();
					uint64 l_ui64SamplingRate = ((TSignalEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputSamplingRate();
					if(!checkFailureRoutine(m_oMatlabHelper->getSignalOutputHeader(i,l_pMatrixToSend,l_ui64SamplingRate),"Error calling [OV_getSignalOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")) return false;

					// Set the new sampling rate
					((TSignalEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputSamplingRate() = l_ui64SamplingRate;
					
					l_bUnknownType = false;
				}
				else if(l_oType == OV_TypeId_FeatureVector)
				{
					IMatrix * l_pMatrixToSend = ((TFeatureVectorEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputMatrix();
					if(!checkFailureRoutine(m_oMatlabHelper->getFeatureVectorOutputHeader(i,l_pMatrixToSend),"Error calling [OV_getFeatureVectorOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")) return false;

					l_bUnknownType = false;
				}
				else if(l_oType == OV_TypeId_Spectrum)
				{
					IMatrix * l_pMatrixToSend = ((TSpectrumEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputMatrix();
					IMatrix * l_pFreqAbscissa = ((TSpectrumEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputFrequencyAbscissa();
					uint64_t l_SamplingRate = ((TSpectrumEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputSamplingRate();

					if(!checkFailureRoutine(m_oMatlabHelper->getSpectrumOutputHeader(i, l_pMatrixToSend, l_pFreqAbscissa, l_SamplingRate),"Error calling [OV_getSpectrumOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")) return false;

					l_bUnknownType = false;
				}
				else if(l_oType == OV_TypeId_ChannelLocalisation)
				{
					IMatrix * l_pMatrixToSend = ((TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputMatrix();
					bool   l_bDynamic      = ((TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputDynamic();
					if(!checkFailureRoutine(m_oMatlabHelper->getChannelLocalisationOutputHeader(i,l_pMatrixToSend,l_bDynamic),"Error calling [OV_getChannelLocalizationOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")) return false;

					// Set the new channel localisation
					((TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputDynamic() = l_bDynamic;

					l_bUnknownType = false;
				}
				else if(l_oType == OV_TypeId_ExperimentInformation)
				{
					OV_WARNING_K("The Experiment Information Stream is not implemented with the Matlab Scripting Box. If this is relevant for your usage, please contact the official development Team.");
					l_bUnknownType = false;
				}
				else if(l_oType == OV_TypeId_Stimulations)
				{
					IStimulationSet * l_pStimSet = ((TStimulationEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputStimulationSet();
					if(!checkFailureRoutine(m_oMatlabHelper->getStimulationsOutputHeader(i, l_pStimSet),"Error calling [OV_getStimulationsOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")) return false;

					l_bUnknownType = false;
				}

				OV_ERROR_UNLESS_KRF(
					!l_bUnknownType,
					"Unknown Stream Type on output ["<<i<<"].",
					OpenViBE::Kernel::ErrorType::BadProcessing);

				m_mEncoders[i]->encodeHeader();
				l_rDynamicBoxContext.markOutputAsReadyToSend(i,0,0);


				m_mOutputHeaderState[i] = true;
			}

			
			bool l_bUnknownType = true;
			uint64 l_ui64StartTime = 0;
			uint64 l_ui64EndTime   = 0;

			if(l_oType == OV_TypeId_StreamedMatrix || this->getTypeManager().isDerivedFromStream(l_oType, OV_TypeId_StreamedMatrix))
			{
				IMatrix * l_pMatrixToSend = ((TStreamedMatrixEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputMatrix();
				if(!checkFailureRoutine(m_oMatlabHelper->popStreamedMatrixOutputBuffer(i,l_pMatrixToSend,l_ui64StartTime,l_ui64EndTime),"Error calling [OV_popOutputBufferReshape] for Streamed Matrix stream or child stream.\n")) return false;

				l_bUnknownType = false;
			}

			if(l_oType == OV_TypeId_Stimulations)
			{
				IStimulationSet * l_pStimSet = ((TStimulationEncoder<CBoxAlgorithmMatlabScripting> *) m_mEncoders[i])->getInputStimulationSet();
				l_pStimSet->clear();
				if(!checkFailureRoutine(m_oMatlabHelper->popStimulationsOutputBuffer(i,l_pStimSet,l_ui64StartTime,l_ui64EndTime),"Error calling [OV_popOutputBuffer] for Stimulation stream.\n")) return false;
				l_bUnknownType = false;
			}

			OV_ERROR_UNLESS_KRF(
				!l_bUnknownType,
				"Unknown Stream Type on output ["<<i<<"].",
				OpenViBE::Kernel::ErrorType::BadProcessing);

			m_mEncoders[i]->encodeBuffer();
			l_rDynamicBoxContext.markOutputAsReadyToSend(i,l_ui64StartTime,l_ui64EndTime);

		}
		mxDestroyArray(l_pPending);
		
	}

	return true;
}

bool CBoxAlgorithmMatlabScripting::processInput(unsigned int inputIndex)
{
	OV_ERROR_UNLESS_KRF(getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(),
		"Error while marking algorithm as ready to process",
		ErrorType::Internal);
	return true;
}

bool CBoxAlgorithmMatlabScripting::printOutputBufferWithFormat()
{
	// the buffer for the console
	std::stringstream l_ssMatlabBuffer;
	l_ssMatlabBuffer<<m_sMatlabBuffer;
	if(l_ssMatlabBuffer.str().size()>0)
	{
		const size_t l_oErrorIndex1 = l_ssMatlabBuffer.str().find("??? ");
		const size_t l_oErrorIndex2 = l_ssMatlabBuffer.str().find("Error: ");
		const size_t l_oErrorIndex3 = l_ssMatlabBuffer.str().find("Error ");

		// Find the earliest error message
		size_t l_oErrorIndex = std::string::npos;
		if (l_oErrorIndex == std::string::npos || (l_oErrorIndex1 != std::string::npos && l_oErrorIndex1 < l_oErrorIndex))
		{
			l_oErrorIndex = l_oErrorIndex1;
		}
		if (l_oErrorIndex == std::string::npos || (l_oErrorIndex2 != std::string::npos && l_oErrorIndex2 < l_oErrorIndex))
		{
			l_oErrorIndex = l_oErrorIndex2;
		}
		if (l_oErrorIndex == std::string::npos || (l_oErrorIndex3 != std::string::npos && l_oErrorIndex3 < l_oErrorIndex))
		{
			l_oErrorIndex = l_oErrorIndex3;
		}

		size_t l_oWarningIndex=l_ssMatlabBuffer.str().find("Warning: ");
		if(l_oErrorIndex==std::string::npos && l_oWarningIndex==std::string::npos)
		{
			this->getLogManager()<<LogLevel_Info<< "\n---- MATLAB BUFFER - INFO ----\n"<<l_ssMatlabBuffer.str().substr(0,(l_oWarningIndex<l_oErrorIndex)?l_oWarningIndex:l_oErrorIndex).c_str()<<"\n";
			//this->getLogManager()<<LogLevel_Info<< "-------- END OF BUFFER --------\n";
		}
		if(l_oWarningIndex!=std::string::npos)
		{
			this->getLogManager()<<LogLevel_Warning<< "\n---- MATLAB BUFFER - WARNING ----\n"<<l_ssMatlabBuffer.str().substr(l_oWarningIndex,l_oErrorIndex).c_str()<<"\n";
			//this->getLogManager()<<LogLevel_Warning<< "-------- END OF BUFFER --------\n";
		}
		if(l_oErrorIndex!=std::string::npos)
		{
			this->getLogManager()<<LogLevel_Error<< "\n---- MATLAB BUFFER - ERROR ! ----\n"<< l_ssMatlabBuffer.str().substr(l_oErrorIndex).c_str()<<"\n";
			//this->getLogManager()<<LogLevel_Error<< "-------- END OF BUFFER --------\n";
			m_bErrorDetected = true;
		}
	}
	return true;
}

#endif // TARGET_HAS_ThirdPartyMatlab
