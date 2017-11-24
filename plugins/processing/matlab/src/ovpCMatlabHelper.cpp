#if defined TARGET_HAS_ThirdPartyMatlab

#include "ovpCMatlabHelper.h"


#include <system/ovCMemory.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <mex.h>
//#include <engine.h>
#include <string>

#if defined TARGET_OS_Windows
	#include <windows.h>
#endif

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace OpenViBEToolkit;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Matlab;

using namespace std;
//---------------------------------------------------------------------------------------------------------------

static std::string escapeMatlabString(const char* sStringToEscape)
{
	   std::string str = std::string(sStringToEscape);
	   std::string::iterator it = str.begin();
	   while(it != str.end())
	   {
			   if( *it =='\'')
			   {
					   str.insert(it,'\'');
					   it++;
			   }
			   it++;
	   }
	   return str;
}

static std::string genLabelsList(IMatrix* pMatrix, int axis = 0)
{
	   std::string labelsList = "";
	   for(uint32_t i = 0; i<pMatrix->getDimensionSize(axis); i++)
	   {
			   labelsList += std::string("'") + escapeMatlabString(pMatrix->getDimensionLabel(axis,i)) + "' ";
	   }
	   return labelsList;
}


static uint64_t convertFromMArray(mxArray* array, int index)
{
	const double* ptr = ::mxGetPr(array);
	const float64 timeValue = static_cast<float64>(ptr[index]);
	return ITimeArithmetics::secondsToTime(timeValue);
}
static uint64_t castFromMArray(mxArray* array, int index)
{
	const double* ptr = ::mxGetPr(array);
	const float64 value = static_cast<float64>(ptr[index]);
	return static_cast<uint64_t>(value);
}

static CString getNameFromCell(mxArray* l_pNames, int cellIndex)
{
	   mxArray * l_pCell = mxGetCell(l_pNames, cellIndex);
	   if(!l_pCell)
	   {
			   return CString("");
	   }
	   const mwSize * l_pCellSizes  = mxGetDimensions(l_pCell);
	   char * l_sName = new char[l_pCellSizes[1]+1];
	   l_sName[l_pCellSizes[1]] = '\0';
	   for(uint32_t cellsize = 0; cellsize < (uint32_t)l_pCellSizes[1]; cellsize++)
	   {
			   l_sName[cellsize] = static_cast<char*>(::mxGetData(l_pCell))[cellsize*2];
	   }
	   CString res = l_sName;
	   delete l_sName;
	   return res;
}

std::vector<CString> CMatlabHelper::getNamelist(const char* name)
{
	std::vector<CString> res;

	mxArray* marray = ::engGetVariable(m_pMatlabEngine, name);
	if (!marray)
	{
		this->getLogManager() << LogLevel_Error << "Nonexisting variable [" << name << "]\n";
		return res;
	}

	mwSize nbCells = mxGetNumberOfElements(marray);
	for(uint32_t cell = 0; cell < nbCells; cell++)
	{
		res.push_back(getNameFromCell(marray, cell));
	}

	mxDestroyArray(marray);
	return res;
}

uint32_t CMatlabHelper::getUint32FromEnv(const char* name)
{
	   mxArray* marray = ::engGetVariable(m_pMatlabEngine, name);
	   if (!marray)
	   {
		   this->getLogManager() << LogLevel_Error << "Nonexisting variable [" << name << "]\n";
		   return 0;
	   }
	   uint32_t res = static_cast<uint32_t> (*mxGetPr(marray));
	   mxDestroyArray(marray);
	   return res;
}

uint64_t CMatlabHelper::getUint64FromEnv(const char* name)
{
	   mxArray* marray = ::engGetVariable(m_pMatlabEngine, name);
	   if (!marray)
	   {
		   this->getLogManager() << LogLevel_Error << "Nonexisting variable [" << name << "]\n";
		   return 0;
	   }
	   uint64_t res = static_cast<uint64_t> (*mxGetPr(marray));
	   mxDestroyArray(marray);
	   return res;
}

uint64_t CMatlabHelper::genUint64FromEnvConverted(const char* name)
{
		mxArray* marray = ::engGetVariable(m_pMatlabEngine, name);
		if (!marray)
		{
			this->getLogManager() << LogLevel_Error << "Nonexisting variable [" << name << "]\n";
			return 0;
		}

		const float64 value = static_cast<float64>(*mxGetPr(marray));
		uint64_t res = ITimeArithmetics::secondsToTime(value);
		mxDestroyArray(marray);
		return res;
}
//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::setStreamedMatrixInputHeader(uint32_t ui32InputIndex, IMatrix * pMatrix)
{
	std::string l_sLabelList = "";
	std::string l_sDimensionSizes = "";
	for(uint32_t dim = 0; dim<pMatrix->getDimensionCount(); dim++)
	{
		l_sDimensionSizes += std::to_string(pMatrix->getDimensionSize(dim)) + " ";
		l_sLabelList += std::string("{") + genLabelsList(pMatrix, dim) + "} ";
	}
	//function box_out = OV_setStreamedMatrixInputInputHeader(box_in, input_index, dimension_count, dimension_sizes, dimension_labels)

	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_setStreamedMatrixInputHeader(" + (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + ","
			+ std::to_string(pMatrix->getDimensionCount()) + ","
			+ "[" + l_sDimensionSizes + "],"
			+ "{"+ l_sLabelList + "});";
	
	return engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

bool CMatlabHelper::setFeatureVectorInputHeader(uint32_t ui32InputIndex, IMatrix * pMatrix)
{
	std::string l_sLabelList = genLabelsList(pMatrix, 0);
	CString l_sDimensionSizes = "";
	
	//function box_out = OV_setStreamedMatrixInputInputHeader(box_in, input_index, dimension_count, dimension_sizes, dimension_labels)

	//box_out = OV_setFeatureVectorInputHeader(box_in, input_index, nb_features, labels)
	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_setFeatureVectorInputHeader(" + (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + ","
			+ std::to_string(pMatrix->getDimensionSize(0)) + ","
			+ "{"+ l_sLabelList + "});";

	return engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

bool CMatlabHelper::setSignalInputHeader(uint32_t ui32InputIndex, IMatrix * pMatrix, uint64_t ui64SamplingRate)
{
	std::string l_sLabelList = genLabelsList(pMatrix, 0);
	//function box_out = ov_set_signal_input_header(box_in,  input_index, nb_channel, nb_samples_per_buffer, channel_names, sampling_rate)

	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_setSignalInputHeader(" + (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + ","
			+ std::to_string(pMatrix->getDimensionSize(0)) + ","
			+ std::to_string(pMatrix->getDimensionSize(1)) + ","
			+ "{" + l_sLabelList + "},"
			+ std::to_string(ui64SamplingRate) + ");";

	return engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;

}

bool CMatlabHelper::setChannelLocalisationInputHeader(uint32_t ui32InputIndex, IMatrix * pMatrix, bool bDynamic)
{
	std::string l_sLabelList = genLabelsList(pMatrix, 0);
	//function  box_out = OV_setChannelLocalisationInputHeader(box_in, input_index, nb_channels, channel_names, dynamic)

	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_setChannelLocalisationInputHeader(" + (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + ","
			+ std::to_string(pMatrix->getDimensionSize(0)) + ","
			+ "{"+ l_sLabelList + "},"
			+ (bDynamic?"true":"false") + ");";
	
	return engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

bool CMatlabHelper::setSpectrumInputHeader(uint32_t ui32InputIndex, IMatrix* pMatrix, IMatrix* pFrequencyAbscissa, uint64_t samplingRate)
{
	std::string l_sLabelList = genLabelsList(pMatrix, 0);
	std::string l_sAbscissaList = genLabelsList(pMatrix, 1);

	// @FIXME CERT this is now one dim array
	mxArray * l_pMatlabMatrix=::mxCreateDoubleMatrix(
			pFrequencyAbscissa->getDimensionSize(0),
			1,
			mxREAL);
					
	System::Memory::copy(::mxGetPr(l_pMatlabMatrix), pFrequencyAbscissa->getBuffer(), pFrequencyAbscissa->getBufferElementCount()*sizeof(float64));
	::engPutVariable(m_pMatlabEngine, "OV_MATRIX_TMP", l_pMatlabMatrix);

	mxDestroyArray(l_pMatlabMatrix);

	//box_out = OV_setSpectrumInputHeader(box_in, input_index, nb_channels, channel_names, nb_bands, band_names, bands, sampling_rate)
	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_setSpectrumInputHeader("
			+ (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + "," // input_index
			+ std::to_string(pMatrix->getDimensionSize(0)) + "," // nb_channels
			+ "{" + l_sLabelList + "}," // channel_names
			+ std::to_string(pMatrix->getDimensionSize(1)) + "," // nb_freq abscissa
			+ "{" + l_sAbscissaList + "}," //freq abscissa names
			+ "OV_MATRIX_TMP," //bands
			+ std::to_string(samplingRate) + ");"; //sampling rate

	return engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

bool CMatlabHelper::setStimulationsInputHeader(uint32_t ui32InputIndex)
{
	//box_out = OV_setStimulationInputHeader(box_in, input_index)
	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_setStimulationsInputHeader("
			+ (const char*)m_sBoxInstanceVariableName + "," + std::to_string(ui32InputIndex+1) + ");";
	
	return engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

//bool setExperimentInformationInputHeader(IMatrix * pMatrix);

//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::addStreamedMatrixInputBuffer(uint32_t ui32InputIndex, IMatrix * pMatrix, uint64_t ui64OpenvibeStartTime,uint64_t ui64OpenvibeEndTime)
{
	mwSize* l_pDims = new mwSize[pMatrix->getDimensionCount()];
	uint32_t j = pMatrix->getDimensionCount()-1;
	for(uint32_t i = 0; i < pMatrix->getDimensionCount();i++)
	{
		l_pDims[i] = pMatrix->getDimensionSize(j);
		j--;
	}
	
	mxArray * l_pMatlabMatrix=::mxCreateNumericArray(
		pMatrix->getDimensionCount(),
		l_pDims,
		mxDOUBLE_CLASS,
		mxREAL);

	//test : channel 1 samples to '10'
	//for(uint32_t i = 0; i < 32;i++) pMatrix->getBuffer()[i] = 10;

	System::Memory::copy(::mxGetPr(l_pMatlabMatrix), pMatrix->getBuffer(), pMatrix->getBufferElementCount()*sizeof(float64));
	::engPutVariable(m_pMatlabEngine, "OV_MATRIX_TMP", l_pMatlabMatrix);
	
	mxDestroyArray(l_pMatlabMatrix);

	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_addInputBuffer(" + (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + ","
			+ std::to_string(ITimeArithmetics::timeToSeconds(ui64OpenvibeStartTime)) + ","
			+ std::to_string(ITimeArithmetics::timeToSeconds(ui64OpenvibeEndTime)) + ",OV_MATRIX_TMP');";
	// please note the transpose operator ' to put the matrix with  1 channel per line

	delete[] l_pDims;

	return ::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

bool CMatlabHelper::addStimulationsInputBuffer(uint32_t ui32InputIndex, IStimulationSet * pStimulationSet, uint64_t ui64OpenvibeStartTime,uint64_t ui64OpenvibeEndTime)
{
	if(pStimulationSet->getStimulationCount() == 0)
	{
		if(::engEvalString(m_pMatlabEngine, "OV_MATRIX_TMP = 0") != 0) return false;
	}
	else
	{
		// we create a 3xN matrix for N stims (access is easier in that order)
		mxArray * l_pMatlabMatrix=::mxCreateDoubleMatrix(
			3,
			(uint32_t)pStimulationSet->getStimulationCount(),
			mxREAL);

		for(uint32_t i = 0; i<pStimulationSet->getStimulationCount(); i++)
		{
			::mxGetPr(l_pMatlabMatrix)[i*3]   =  (double)pStimulationSet->getStimulationIdentifier(i);
			::mxGetPr(l_pMatlabMatrix)[i * 3 + 1] = ITimeArithmetics::timeToSeconds(pStimulationSet->getStimulationDate(i));
			::mxGetPr(l_pMatlabMatrix)[i * 3 + 2] = ITimeArithmetics::timeToSeconds(pStimulationSet->getStimulationDuration(i));
		}

		::engPutVariable(m_pMatlabEngine, "OV_MATRIX_TMP", l_pMatlabMatrix);

		mxDestroyArray(l_pMatlabMatrix);
	}

	std::string l_sCommand = std::string(m_sBoxInstanceVariableName) + " = OV_addInputBuffer(" + (const char*)m_sBoxInstanceVariableName + ","
			+ std::to_string(ui32InputIndex + 1) + ","
			+ std::to_string(ITimeArithmetics::timeToSeconds(ui64OpenvibeStartTime)) + ","
			+ std::to_string(ITimeArithmetics::timeToSeconds(ui64OpenvibeEndTime)) + ",OV_MATRIX_TMP');";

	return ::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0;
}

//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::getStreamedMatrixOutputHeader(uint32_t ui32OutputIndex, IMatrix * pMatrix)
{
	std::string l_sCommand = std::string("[OV_ERRNO, OV_NB_DIMENSIONS, OV_DIMENSION_SIZES, OV_DIMENSION_LABELS] = OV_getStreamedMatrixOutputHeader(") + (const char*)m_sBoxInstanceVariableName + "," + std::to_string(ui32OutputIndex + 1).c_str() + ");";
	OV_ERROR_UNLESS_KRF(
		::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0 && getUint32FromEnv("OV_ERRNO") == 0,
		"Could not get Streamed matrix output header",
		OpenViBE::Kernel::ErrorType::BadProcessing);
	
	uint32_t l_ui32NbDimensions = getUint32FromEnv("OV_NB_DIMENSIONS");
	mxArray * l_pDimensionSizes  = ::engGetVariable(m_pMatlabEngine,"OV_DIMENSION_SIZES");
	if (!l_pDimensionSizes)
	{
		this->getLogManager() << LogLevel_Error << "Nonexisting variable [OV_DIMENSION_SIZES]\n";
		return false;
	}

	std::vector<CString> l_pNameList = getNamelist("OV_DIMENSION_LABELS");
	
	pMatrix->setDimensionCount(l_ui32NbDimensions);
	uint32_t l_ui32Index = 0;
	for(uint32_t i = 0; i<l_ui32NbDimensions;i++)
	{
		pMatrix->setDimensionSize(i,(uint32_t) mxGetPr(l_pDimensionSizes)[i]);
		for(uint32_t x = 0; x<pMatrix->getDimensionSize(i) && l_ui32Index < l_pNameList.size() ; x++)
		{
			pMatrix->setDimensionLabel(i,x,escapeMatlabString(l_pNameList[l_ui32Index].toASCIIString()).c_str());
			l_ui32Index++;
		}
	}

	mxDestroyArray(l_pDimensionSizes);

	return true;
}

bool CMatlabHelper::getFeatureVectorOutputHeader(uint32_t ui32OutputIndex, IMatrix * pMatrix)
{
	std::string l_sCommand = std::string("[OV_ERRNO, OV_NB_FEATURES, OV_LABELS] = OV_getFeatureVectorOutputHeader(") + (const char*)m_sBoxInstanceVariableName + "," + std::to_string(ui32OutputIndex + 1).c_str() + ");";
	OV_ERROR_UNLESS_KRF(
		::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0 && getUint32FromEnv("OV_ERRNO") == 0,
		"Could not get Feature Vector output header",
		OpenViBE::Kernel::ErrorType::BadProcessing);

	uint32_t l_ui32NbFeatures = getUint32FromEnv("OV_NB_FEATURES");
	std::vector<CString> l_pNameList = getNamelist("OV_LABELS");
	// Check nb features == nb names ?
						
	pMatrix->setDimensionCount(1);
	pMatrix->setDimensionSize(0,l_ui32NbFeatures);
	for(uint32_t x=0;x<l_ui32NbFeatures;x++)
	{
		pMatrix->setDimensionLabel(0,x,escapeMatlabString(l_pNameList[x].toASCIIString()).c_str());
	}

	return true;
}

bool CMatlabHelper::getSignalOutputHeader(uint32_t ui32OutputIndex, IMatrix * pMatrix, uint64_t& rSamplingFrequency)
{
	std::string l_sCommand = std::string("[OV_ERRNO, OV_NB_CHANNELS, OV_NB_SAMPLES_PER_BUFFER, OV_CHANNEL_NAMES, OV_SAMPLING_RATE] = OV_getSignalOutputHeader(") + (const char*)m_sBoxInstanceVariableName + "," + std::to_string(ui32OutputIndex + 1).c_str() + ");";
	OV_ERROR_UNLESS_KRF(
		::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0 && getUint32FromEnv("OV_ERRNO") == 0,
		"Could not get Signal output header",
		OpenViBE::Kernel::ErrorType::BadProcessing);

	uint32_t l_ui32NbChannels = getUint32FromEnv("OV_NB_CHANNELS");
	uint32_t l_ui32NbSamples  = getUint32FromEnv("OV_NB_SAMPLES_PER_BUFFER");
	std::vector<CString> l_pNameList = getNamelist("OV_CHANNEL_NAMES");
	uint32_t l_ui32Rate = getUint32FromEnv("OV_SAMPLING_RATE");

	if(l_pNameList.size() != l_ui32NbChannels)
	{
		return false;
	}

	pMatrix->setDimensionCount(2);
	pMatrix->setDimensionSize(0,l_ui32NbChannels);
	pMatrix->setDimensionSize(1,l_ui32NbSamples);
	rSamplingFrequency = l_ui32Rate;

	for(uint32_t x=0;x<l_ui32NbChannels;x++)
	{
		pMatrix->setDimensionLabel(0,x,escapeMatlabString(l_pNameList[x].toASCIIString()).c_str());
	}

	return true;

}

bool CMatlabHelper::getChannelLocalisationOutputHeader(uint32_t ui32OutputIndex, IMatrix * pMatrix, bool& bDynamic)
{
	std::string l_sCommand = std::string("[OV_ERRNO, OV_NB_CHANNELS, OV_CHANNEL_NAMES, OV_DYNAMIC] = OV_getChannelLocalisationOutputHeader(") + (const char*)m_sBoxInstanceVariableName + "," + std::to_string(ui32OutputIndex + 1).c_str() + ");";
	OV_ERROR_UNLESS_KRF(
		::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0 && getUint32FromEnv("OV_ERRNO") == 0,
		"Could not get Channel Localisation output header",
		OpenViBE::Kernel::ErrorType::BadProcessing);

	uint32_t l_ui32NbChannels = getUint32FromEnv("OV_NB_CHANNELS");
	std::vector<CString> l_pNameList = getNamelist("OV_CHANNEL_NAMES");
	mxArray * l_pDynamic    = ::engGetVariable(m_pMatlabEngine,"OV_DYNAMIC");
	if (!l_pDynamic)
	{
		this->getLogManager() << LogLevel_Error << "Nonexisting variable [OV_DYNAMIC]\n";
		return false;
	}

	if(l_pNameList.size() != l_ui32NbChannels)
	{
		return false;
	}
				
	pMatrix->setDimensionCount(2);
	pMatrix->setDimensionSize(0,l_ui32NbChannels);
	pMatrix->setDimensionSize(1,3);

	for(uint32_t x=0;x<l_ui32NbChannels;x++)
	{
		pMatrix->setDimensionLabel(0,x,escapeMatlabString(l_pNameList[x].toASCIIString()).c_str());
	}

	mxDestroyArray(l_pDynamic);

	return true;
}

bool CMatlabHelper::getSpectrumOutputHeader(uint32_t ui32OutputIndex, IMatrix * pMatrix, IMatrix * pFrequencyAbscissa, uint64_t& samplingRate)
{
	std::string l_sCommand = std::string("[OV_ERRNO, OV_NB_CHANNELS, OV_CHANNEL_NAMES, OV_NB_BANDS, OV_BANDS_NAME, OV_BANDS_LINEAR, OV_SAMPLING_RATE] = OV_getSpectrumOutputHeader(") + (const char*)m_sBoxInstanceVariableName + "," +std::to_string(ui32OutputIndex + 1).c_str() + ");";
	OV_ERROR_UNLESS_KRF(
		::engEvalString(m_pMatlabEngine, l_sCommand.c_str()) == 0 && getUint32FromEnv("OV_ERRNO") == 0,
		"Could not get Spectrum output header",
		OpenViBE::Kernel::ErrorType::BadProcessing);

	uint32_t l_ui32NbChannels = getUint32FromEnv("OV_NB_CHANNELS");
	std::vector<CString> l_pNameList = getNamelist("OV_CHANNEL_NAMES");
	uint32_t l_ui32NbAbscissa = getUint32FromEnv("OV_NB_ABSCISSAS");
	std::vector<CString>  l_pFreqAbscissaNames = getNamelist("OV_ABSCISSAS_NAME");
	mxArray* l_pFreqAbscissa = ::engGetVariable(m_pMatlabEngine,"OV_ABSCISSAS_LINEAR");
	if (!l_pFreqAbscissa)
	{
		this->getLogManager() << LogLevel_Error << "Nonexisting variable [OV_ABSCISSAS_LINEAR]\n";
		return false;
	}

	samplingRate = getUint64FromEnv("OV_SAMPLING_RATE");

	//The Frequency abscissa list has dimensions nb_bands
	pFrequencyAbscissa->setDimensionCount(1);
	pFrequencyAbscissa->setDimensionSize(0,l_ui32NbAbscissa);
	for(uint32_t x=0;x<l_ui32NbAbscissa;x++)
	{
		pFrequencyAbscissa->setDimensionLabel(0,x,escapeMatlabString(l_pFreqAbscissaNames[x].toASCIIString()).c_str());
	}

	// Adding the bands:
	System::Memory::copy(pFrequencyAbscissa->getBuffer(), ::mxGetPr(l_pFreqAbscissa), (uint64_t)l_ui32NbAbscissa*sizeof(float64));

	pMatrix->setDimensionCount(2);
	pMatrix->setDimensionSize(0,l_ui32NbChannels);
	pMatrix->setDimensionSize(1,l_ui32NbAbscissa);
	for(uint32_t x=0;x<l_ui32NbChannels;x++)
	{
		pMatrix->setDimensionLabel(0,x,escapeMatlabString(l_pNameList[x].toASCIIString()).c_str());
	}
	for(uint32_t x=0;x<l_ui32NbAbscissa;x++)
	{
		pMatrix->setDimensionLabel(1,x,escapeMatlabString(l_pFreqAbscissaNames[x].toASCIIString()).c_str());
	}
	// @FIXME CERT is it me or it never copy data to pMatrix ?

	mxDestroyArray(l_pFreqAbscissa);

	return true;
}

bool CMatlabHelper::getStimulationsOutputHeader(uint32_t ui32OutputIndex, IStimulationSet * pStimulationSet)
{
	// Nothing to do, the stimulation header is empty.
	return true;
}

//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::popStreamedMatrixOutputBuffer(uint32_t ui32OutputIndex, IMatrix * pMatrix, uint64_t& rStartTime, uint64_t& rEndTime)
{
	std::string buf = std::string("[") + (const char*)m_sBoxInstanceVariableName + ", OV_START_TIME, OV_END_TIME, OV_LINEAR_DATA_SIZE, OV_LINEAR_DATA] = OV_popOutputBufferReshape(" + (const char*)m_sBoxInstanceVariableName + ", " + 	std::to_string(ui32OutputIndex + 1).c_str() + ");";
	uint32_t l_ui32Result = ::engEvalString(m_pMatlabEngine, buf.c_str());
	if(l_ui32Result != 0)
	{
		return false;
	}

	rStartTime  = genUint64FromEnvConverted("OV_START_TIME");
	rEndTime = genUint64FromEnvConverted("OV_END_TIME");
	uint64_t l_pSize = getUint64FromEnv("OV_LINEAR_DATA_SIZE");
	mxArray * l_pData      = ::engGetVariable(m_pMatlabEngine,"OV_LINEAR_DATA");
	if (!l_pData)
	{
		this->getLogManager() << LogLevel_Error << "Nonexisting variable [OV_LINEAR_DATA]\n";
		return false;
	}

	// ti be copied direclty in openvibe buffer, the linear matrix must be ordered line by line
	System::Memory::copy(pMatrix->getBuffer(), ::mxGetPr(l_pData), l_pSize*sizeof(float64));
	mxDestroyArray(l_pData);

	return true;
}

bool CMatlabHelper::popStimulationsOutputBuffer(uint32_t ui32OutputIndex, IStimulationSet * pStimulationSet, uint64_t& rStartTime, uint64_t& rEndTime)
{
	std::string buf = std::string("[") + (const char*)m_sBoxInstanceVariableName + ", OV_START_TIME, OV_END_TIME, OV_LINEAR_MATRIX_SIZE, OV_LINEAR_DATA] = OV_popOutputBuffer(" + (const char*)m_sBoxInstanceVariableName + ", " + 	std::to_string(ui32OutputIndex + 1).c_str() + ");";
	uint32_t l_ui32Result = ::engEvalString(m_pMatlabEngine, buf.c_str());
	if(l_ui32Result != 0)
	{
		return false;
	}
	rStartTime  = genUint64FromEnvConverted("OV_START_TIME");
	rEndTime = genUint64FromEnvConverted("OV_END_TIME");
	uint32_t l_pSize = getUint32FromEnv("OV_LINEAR_MATRIX_SIZE");
	mxArray* l_pData = ::engGetVariable(m_pMatlabEngine,"OV_LINEAR_DATA");
	if (!l_pData)
	{
		this->getLogManager() << LogLevel_Error << "Nonexisting variable [OV_LINEAR_DATA]\n";
		return false;
	}

	for(uint32_t i = 0; i < l_pSize; i+=3)
	{
		uint64_t l_ui64Identifier = castFromMArray(l_pData,    i+0);
		uint64_t l_ui64Date       = convertFromMArray(l_pData, i+1);
		uint64_t l_ui64Duration   = convertFromMArray(l_pData, i+2);
		pStimulationSet->appendStimulation(l_ui64Identifier,l_ui64Date, l_ui64Duration);
	}

	mxDestroyArray(l_pData);

	return true;
}

//--------------------------------------------------------------------------------------------------------------
#endif // TARGET_HAS_ThirdPartyMatlab
