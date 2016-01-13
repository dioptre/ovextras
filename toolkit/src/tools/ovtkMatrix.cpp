#include "ovtkMatrix.h"

#include <system/ovCMemory.h>

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cfloat>
#include <cerrno>

// for save/load
#include <fstream>
#include <iostream>
#include <vector>
#include <locale> // std::isspace
#include <sstream>

#if defined TARGET_OS_Windows
 #define isnan _isnan
 #define isinf !_finite
#elif defined TARGET_OS_MacOS
 #define isnan std::isnan
 #define isinf std::isinf
#endif

using namespace OpenViBE;
using namespace OpenViBEToolkit;
using namespace OpenViBEToolkit::Tools;

boolean OpenViBEToolkit::Tools::Matrix::copy(IMatrix& rDestinationMatrix, const IMatrix& rSourceMatrix)
{
	if(&rDestinationMatrix==&rSourceMatrix)
	{
		return true;
	}

	if(!copyDescription(rDestinationMatrix, rSourceMatrix))
	{
		return false;
	}
	if(!copyContent(rDestinationMatrix, rSourceMatrix))
	{
		return false;
	}
	return true;
}

boolean OpenViBEToolkit::Tools::Matrix::copyDescription(IMatrix& rDestinationMatrix, const IMatrix& rSourceMatrix)
{
	if(&rDestinationMatrix==&rSourceMatrix)
	{
		return true;
	}

	uint32 l_ui32DimensionCount=rSourceMatrix.getDimensionCount();
	uint32 l_ui32DimensionSize=0;
	if(!rDestinationMatrix.setDimensionCount(l_ui32DimensionCount))
	{
		return false;
	}
	for(uint32 i=0; i<l_ui32DimensionCount; i++)
	{
		l_ui32DimensionSize=rSourceMatrix.getDimensionSize(i);
		if(!rDestinationMatrix.setDimensionSize(i, l_ui32DimensionSize))
		{
			return false;
		}
		for(uint32 j=0; j<l_ui32DimensionSize; j++)
		{
			if(!rDestinationMatrix.setDimensionLabel(i, j, rSourceMatrix.getDimensionLabel(i, j)))
			{
				return false;
			}
		}
	}
	return true;
}

boolean OpenViBEToolkit::Tools::Matrix::copyContent(IMatrix& rDestinationMatrix, const IMatrix& rSourceMatrix)
{
	if(&rDestinationMatrix==&rSourceMatrix)
	{
		return true;
	}

	uint32 l_ui32SourceElementCount=rSourceMatrix.getBufferElementCount();
	uint32 l_ui32DestinationElementCount=rDestinationMatrix.getBufferElementCount();
	if(l_ui32DestinationElementCount != l_ui32SourceElementCount)
	{
		return false;
	}
	const float64* l_pSourceBuffer=rSourceMatrix.getBuffer();
	float64* l_pDestinationBuffer=rDestinationMatrix.getBuffer();
	System::Memory::copy(l_pDestinationBuffer, l_pSourceBuffer, l_ui32SourceElementCount*sizeof(float64));
	return true;
}

boolean OpenViBEToolkit::Tools::Matrix::clearContent(IMatrix& rMatrix)
{
	System::Memory::set(rMatrix.getBuffer(), rMatrix.getBufferElementCount()*sizeof(float64), 0);
	return true;
}

boolean OpenViBEToolkit::Tools::Matrix::isDescriptionSimilar(const IMatrix& rSourceMatrix1, const IMatrix& rSourceMatrix2, const boolean bCheckLabels)
{
	if(&rSourceMatrix1==&rSourceMatrix2)
	{
		return true;
	}

	if(rSourceMatrix1.getDimensionCount() != rSourceMatrix2.getDimensionCount())
	{
		return false;
	}

	for(uint32 i=0; i<rSourceMatrix1.getDimensionCount(); i++)
	{
		if(rSourceMatrix1.getDimensionSize(i) != rSourceMatrix2.getDimensionSize(i))
		{
			return false;
		}
	}

	if(bCheckLabels)
	{
		for(uint32 i=0; i<rSourceMatrix1.getDimensionCount(); i++)
		{
			for(uint32 j=0; j<rSourceMatrix1.getDimensionSize(i); j++)
			{
				if(strcmp(rSourceMatrix1.getDimensionLabel(i, j), rSourceMatrix2.getDimensionLabel(i, j))!=0)
				{
					return false;
				}
			}
		}
	}

	return true;
}

boolean OpenViBEToolkit::Tools::Matrix::isContentSimilar(const IMatrix& rSourceMatrix1, const IMatrix& rSourceMatrix2)
{
	if(&rSourceMatrix1==&rSourceMatrix2)
	{
		return true;
	}

	if(rSourceMatrix1.getBufferElementCount() != rSourceMatrix2.getBufferElementCount())
	{
		return false;
	}

	return ::memcmp(rSourceMatrix1.getBuffer(), rSourceMatrix2.getBuffer(), rSourceMatrix1.getBufferElementCount()*sizeof(float64)) == 0;
}

boolean OpenViBEToolkit::Tools::Matrix::isContentValid(const IMatrix& rSourceMatrix, const boolean bCheckNotANumber, const boolean bCheckInfinity)
{
	const float64* l_pBuffer=rSourceMatrix.getBuffer();
	const float64* l_pBufferEnd=rSourceMatrix.getBuffer()+rSourceMatrix.getBufferElementCount();
	while(l_pBuffer!=l_pBufferEnd)
	{
		if(bCheckNotANumber && isnan(*l_pBuffer)) return false;
		if(bCheckInfinity && isinf(*l_pBuffer)) return false;
		l_pBuffer++;
	}
	return true;
}

enum EStatus
{
	Status_Nothing,
	Status_ParsingHeader,
	Status_ParsingHeaderDimension,
	Status_ParsingHeaderLabel,
	Status_ParsingBuffer,
	Status_ParsingBufferValue
};

// tokens in the ascii matrix format
const char CONSTANT_LEFT_SQUARE_BRACKET  = '[';
const char CONSTANT_RIGHT_SQUARE_BRACKET = ']';
const char CONSTANT_HASHTAG              = '#';
const char CONSTANT_DOUBLE_QUOTE         = '"';
const char CONSTANT_TAB                  = '\t';
const char CONSTANT_CARRIAGE_RETURN      = '\r';
const char CONSTANT_EOL                  = '\n';
const char CONSTANT_SPACE                = ' ';

boolean OpenViBEToolkit::Tools::Matrix::fromString(OpenViBE::IMatrix& rMatrix, const OpenViBE::CString& sString)
{
	std::stringstream l_oBuffer;

	l_oBuffer << sString.toASCIIString();

	std::locale l_oLocale("C");
	//current string to parse
	std::string l_sWhat;
	//current parsing status
	uint32 l_ui32Status=Status_Nothing;
	//current element index (incremented every time a value is stored in matrix)
	uint32 l_ui32CurElementIndex = 0;
	//number of dimensions
	// uint32 l_ui32DimensionCount = rDestinationMatrix.getDimensionCount();
	//current dimension index
	uint32 l_ui32CurDimensionIndex = (uint32)-1;
	//vector keeping track of dimension sizes
	std::vector<uint32> l_vDimensionSize;
	//vector keeping track of number of values found in each dimension
	std::vector<uint32> l_vValuesCount;
	// Dim labels
	std::vector<std::string> l_vLabels;
	//current quote-delimited string
	std::string l_sCurString;

	do
	{
		//read current line
		std::getline(l_oBuffer, l_sWhat, CONSTANT_EOL);

		//is line empty?
		if(l_sWhat.length()==0)
		{
			//skip it
			continue;
		}

		//output line to be parsed in debug level
		// getLogManager() << LogLevel_Debug << CString(l_sWhat.c_str()) << "\n";

		//remove ending carriage return (if any) for windows / linux compatibility
		if(l_sWhat[l_sWhat.length()-1]==CONSTANT_CARRIAGE_RETURN)
		{
			l_sWhat.erase(l_sWhat.length()-1, 1);
		}

		//start parsing current line
		std::string::iterator l_oIt = l_sWhat.begin();

		//parse current line
		while(l_oIt != l_sWhat.end())
		{
			switch(l_ui32Status)
			{
				//initial parsing status
				case Status_Nothing:

					//comments starting
					if(*l_oIt == CONSTANT_HASHTAG)
					{
						//ignore rest of line by skipping to last character
						l_oIt = l_sWhat.end()-1;
					}
					//header starting
					else if(*l_oIt == CONSTANT_LEFT_SQUARE_BRACKET)
					{
						//update status
						l_ui32Status = Status_ParsingHeader;
					}
					else if(std::isspace(*l_oIt, l_oLocale) == false)
					{
						// getLogManager() << LogLevel_Trace << "Unexpected character found on line " << l_sWhat.c_str() << ", parsing aborted\n";
						return false;
					}
					break;

				//parse header
				case Status_ParsingHeader:

					//comments starting
					if(*l_oIt == CONSTANT_HASHTAG)
					{
						//ignore rest of line by skipping to last character
						l_oIt = l_sWhat.end()-1;
					}
					//new dimension opened
					else if(*l_oIt == CONSTANT_LEFT_SQUARE_BRACKET)
					{
						//increment dimension count
						l_vDimensionSize.resize(l_vDimensionSize.size()+1);

						//update current dimension index
						l_ui32CurDimensionIndex++;

						//update status
						l_ui32Status = Status_ParsingHeaderDimension;
					}
					//finished parsing header
					else if(*l_oIt == CONSTANT_RIGHT_SQUARE_BRACKET)
					{
						//ensure at least one dimension was found
						if(l_vDimensionSize.size() == 0)
						{
							// getLogManager() << LogLevel_Trace << "End of header section reached, found 0 dimensions : parsing aborted\n";
							return false;
						}

						//resize matrix
						rMatrix.setDimensionCount(l_vDimensionSize.size());
						for(size_t i=0; i<l_vDimensionSize.size(); i++)
						{
							rMatrix.setDimensionSize(i, l_vDimensionSize[i]);
						}

						l_vValuesCount.resize(rMatrix.getDimensionCount());
						
						// set labels now that we know the matrix size
						uint32 l_ui32Element = 0;
						for(uint32 i=0;i<rMatrix.getDimensionCount();i++)
						{
							for(uint32 j=0;j<rMatrix.getDimensionSize(i);j++)
							{
								rMatrix.setDimensionLabel(i,j,l_vLabels[l_ui32Element++].c_str());
							}
						}

						/*
						//dump dimension count and size
						char l_pBuf[1024]={'\0'};
						for(size_t i=0; i<l_vDimensionSize.size(); i++)
						{
							if(i>0)
							{
								strcat(l_pBuf, ", ");
								sprintf(l_pBuf+strlen(l_pBuf), "%d", l_vDimensionSize[i]);
							}
							else
							{
								sprintf(l_pBuf+strlen(l_pBuf), "%d", l_vDimensionSize[i]);
							}
						}
						getLogManager() << LogLevel_Trace
							<< "End of header section reached, found " << (uint32)l_vDimensionSize.size() << " dimensions of size ["
							<< CString(l_pBuf) << "]\n";
						*/

						//reset current dimension index
						l_ui32CurDimensionIndex = (uint32)-1;

						//update status
						l_ui32Status = Status_ParsingBuffer;
					}
					else if(std::isspace(*l_oIt, l_oLocale) == false)
					{
					//	getLogManager() << LogLevel_Trace << "Unexpected character found on line " << CString(l_sWhat.c_str()) << ", parsing aborted\n";
						return false;
					}
					break;

				case Status_ParsingHeaderDimension:

					//comments starting
					if(*l_oIt == CONSTANT_HASHTAG)
					{
						//ignore rest of line by skipping to last character
						l_oIt = l_sWhat.end()-1;
					}
					//new label found
					else if(*l_oIt == CONSTANT_DOUBLE_QUOTE)
					{
						//new element found in current dimension
						l_vDimensionSize[l_ui32CurDimensionIndex]++;

						//update status
						l_ui32Status = Status_ParsingHeaderLabel;
					}
					//finished parsing current dimension header
					else if(*l_oIt == CONSTANT_RIGHT_SQUARE_BRACKET)
					{
						//update status
						l_ui32Status = Status_ParsingHeader;
					}
					else if(std::isspace(*l_oIt, l_oLocale) == false)
					{
					//	getLogManager() << LogLevel_Trace << "Unexpected character found on line " << CString(l_sWhat.c_str()) << ", parsing aborted\n";
						return false;
					}
					break;

				//look for end of label (first '"' char not preceded by the '\' escape char)
				case Status_ParsingHeaderLabel:

					//found '"' char not preceded by escape char : end of label reached
					if(*l_oIt == CONSTANT_DOUBLE_QUOTE && *(l_oIt-1) != '\\')
					{
						// We can only attach the label later after we know the size
						l_vLabels.push_back(l_sCurString);

						// std::cout << " lab " << l_ui32CurDimensionIndex << " " << l_vDimensionSize[l_ui32CurDimensionIndex]-1 <<  " : " << l_sCurString << "\n";

						//clear current string
						l_sCurString.erase();

						//update status
						l_ui32Status = Status_ParsingHeaderDimension;
					}
					//otherwise, keep parsing current label
					else
					{
						l_sCurString.append(1, *l_oIt);
					}
					break;

				case Status_ParsingBuffer:

					//comments starting
					if(*l_oIt == CONSTANT_HASHTAG)
					{
						//ignore rest of line by skipping to last character
						l_oIt = l_sWhat.end()-1;
					}
					//going down one dimension
					else if(*l_oIt == CONSTANT_LEFT_SQUARE_BRACKET)
					{
						//update dimension index
						l_ui32CurDimensionIndex++;

						//ensure dimension count remains in allocated range
						if(l_ui32CurDimensionIndex == rMatrix.getDimensionCount())
						{
						//	getLogManager() << LogLevel_Trace << "Exceeded expected number of dimensions while parsing values, parsing aborted\n";
							return false;
						}

						//ensure values count remains in allocated range
						if(l_vValuesCount[l_ui32CurDimensionIndex] == rMatrix.getDimensionSize(l_ui32CurDimensionIndex))
						{
						//	getLogManager() << LogLevel_Trace << "Exceeded expected number of values for dimension " << l_ui32CurDimensionIndex << ", parsing aborted\n";
							return false;
						}

						//increment values count for current dimension, if it is not the innermost
						if(l_ui32CurDimensionIndex < rMatrix.getDimensionCount() - 1)
						{
							l_vValuesCount[l_ui32CurDimensionIndex]++;
						}
					}
					//going up one dimension
					else if(*l_oIt == CONSTANT_RIGHT_SQUARE_BRACKET)
					{
						//if we are not in innermost dimension
						if(l_ui32CurDimensionIndex < rMatrix.getDimensionCount()-1)
						{
							//ensure the right number of values was parsed in lower dimension
							if(l_vValuesCount[l_ui32CurDimensionIndex+1] != rMatrix.getDimensionSize(l_ui32CurDimensionIndex+1))
							{
							//	getLogManager() << LogLevel_Trace
							//		<< "Found " << l_vValuesCount[l_ui32CurDimensionIndex+1] << " values in dimension "
							//		<< l_ui32CurDimensionIndex+1 << ", expected " << op_pMatrix->getDimensionSize(l_ui32CurDimensionIndex+1) << ", parsing aborted\n";
								return false;
							}
							//reset values count of lower dimension to 0
							l_vValuesCount[l_ui32CurDimensionIndex+1] = 0;
						}
						//ensure dimension count is correct
						else if(l_ui32CurDimensionIndex == (uint32)-1)
						{
							// getLogManager() << LogLevel_Trace << "Found one too many closing bracket character, parsing aborted\n";
							return false;
						}

						//go up one dimension
						l_ui32CurDimensionIndex--;
					}
					//non whitespace character found
					else if(std::isspace(*l_oIt, l_oLocale) == false)
					{
						//if we are in innermost dimension, assume a value is starting here
						if(l_ui32CurDimensionIndex == rMatrix.getDimensionCount()-1)
						{
							//ensure values parsed so far in current dimension doesn't exceed current dimension size
							if(l_vValuesCount.back() == rMatrix.getDimensionSize(l_ui32CurDimensionIndex))
							{
							//	getLogManager() << LogLevel_Trace
							//		<< "Found " << l_vValuesCount.back() << " values in dimension " << l_ui32CurDimensionIndex
							//		<< ", expected " << RDestinationMatrix.getDimensionSize(l_ui32CurDimensionIndex) << ", parsing aborted\n";
								return false;
							}

							//increment values count found in innermost dimension
							l_vValuesCount[l_ui32CurDimensionIndex]++;

							//append current character to current string
							l_sCurString.append(1, *l_oIt);

							//update status
							l_ui32Status = Status_ParsingBufferValue;
						}
						else
						{
						//	getLogManager() << LogLevel_Trace << "Unexpected character found on line " << CString(l_sWhat.c_str()) << ", parsing aborted\n";
							return false;
						}
					}
					break;

				//look for end of value (first '"' char not preceded by the '\' escape char)
				case Status_ParsingBufferValue:

					//values end at first whitespace character or ']' character
					if(std::isspace(*l_oIt, l_oLocale) == true || *l_oIt == CONSTANT_RIGHT_SQUARE_BRACKET)
					{
						//if dimension closing bracket is found
						if(*l_oIt == CONSTANT_RIGHT_SQUARE_BRACKET)
						{
							//move back iterator by one character so that closing bracket is taken into account in Status_ParsingBuffer case
							l_oIt--;
						}

						//retrieve value
						errno = 0;
						const float64 l_f64Value = atof(l_sCurString.c_str());
#if defined TARGET_OS_Windows
						if(errno == ERANGE)
						{
							//string couldn't be converted to a double
						//	getLogManager() << LogLevel_Trace << "Couldn't convert token \"" << CString(l_sCurString.c_str()) << "\" to floating point value, parsing aborted\n";
							return false;
						}
#endif
						//store value in matrix
						(rMatrix.getBuffer())[l_ui32CurElementIndex] = l_f64Value;

						//update element index
						l_ui32CurElementIndex++;

						//reset current string
						l_sCurString.erase();

						//update status
						l_ui32Status = Status_ParsingBuffer;
					}
					//otherwise, append current character to current string
					else
					{
						l_sCurString.append(1, *l_oIt);
					}
					break;

				default:
					break;
			} // switch(l_ui32Status)

			//increment iterator
			l_oIt++;

		} // while(l_oIt != l_sWhat.end()) (read each character of current line)

	} while(l_oBuffer.good()); //read each line in turn

	//If the file is empty or other (like directory)
	if(l_vValuesCount.size() == 0)
	{
		return false;
	}
	//ensure the right number of values were parsed in first dimension
	if(l_vValuesCount[0] != rMatrix.getDimensionSize(0))
	{
	//	getLogManager() << LogLevel_Trace <<
	//		"Found " << l_vValuesCount[0] << " values in dimension 0, expected " << op_pMatrix->getDimensionSize(0) << ", parsing aborted\n";
		return false;
	}

	return true;
}

// A recursive helper function to spool matrix contents to a txt stringstream. 
boolean dumpMatrixBuffer(const OpenViBE::IMatrix& rMatrix, std::stringstream& buffer, uint32 ui32DimensionIndex, uint32& ui32ElementIndex)
{
	//are we in innermost dimension?
	if(ui32DimensionIndex == rMatrix.getDimensionCount()-1)
	{
		//dimension start
		for(uint32 j=0; j<ui32DimensionIndex; j++)
		{
			buffer << CONSTANT_TAB;
		}
		buffer << CONSTANT_LEFT_SQUARE_BRACKET;

		//dump current cell contents
		for(uint32 j=0; j<rMatrix.getDimensionSize(ui32DimensionIndex); j++, ui32ElementIndex++)
		{
			buffer << CONSTANT_SPACE << rMatrix.getBuffer()[ui32ElementIndex];
		}

		//dimension end
		buffer << CONSTANT_SPACE << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;
	}
	else
	{
		//dump all entries in current dimension
		for(uint32 i=0; i<rMatrix.getDimensionSize(ui32DimensionIndex); i++)
		{
			//dimension start
			for(uint32 j=0; j<ui32DimensionIndex; j++)
			{
				buffer << CONSTANT_TAB;
			}
			buffer << CONSTANT_LEFT_SQUARE_BRACKET << CONSTANT_EOL;

			dumpMatrixBuffer(rMatrix, buffer, ui32DimensionIndex+1, ui32ElementIndex);

			//dimension end
			for(uint32 j=0; j<ui32DimensionIndex; j++)
			{
				buffer << CONSTANT_TAB;
			}
			buffer << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;
		}
	}

	return true;
}

boolean OpenViBEToolkit::Tools::Matrix::toString(const OpenViBE::IMatrix& rMatrix, OpenViBE::CString& sString, uint32 ui32Precision /* = 6 */)
{
	std::stringstream l_oBuffer;

	l_oBuffer << std::scientific;
	l_oBuffer.precision(static_cast<std::streamsize>(ui32Precision));

	// Dump header

	//header start
	l_oBuffer << CONSTANT_LEFT_SQUARE_BRACKET << CONSTANT_EOL;

	//dump labels for each dimension
	for(uint32 i=0; i<rMatrix.getDimensionCount(); i++)
	{
		l_oBuffer << CONSTANT_TAB << CONSTANT_LEFT_SQUARE_BRACKET;

		for(uint32 j=0; j<rMatrix.getDimensionSize(i); j++)
		{
			l_oBuffer << CONSTANT_SPACE << CONSTANT_DOUBLE_QUOTE << rMatrix.getDimensionLabel(i, j) << CONSTANT_DOUBLE_QUOTE;
		}

		l_oBuffer << CONSTANT_SPACE << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;
	}

	//header end
	l_oBuffer << CONSTANT_RIGHT_SQUARE_BRACKET << CONSTANT_EOL;

	// Dump buffer using a recursive algorithm
	uint32 l_ui32ElementIndex = 0;
	dumpMatrixBuffer(rMatrix, l_oBuffer, 0, l_ui32ElementIndex);

	sString = CString(l_oBuffer.str().c_str());

	return true;

}

boolean OpenViBEToolkit::Tools::Matrix::loadFromTextFile(OpenViBE::IMatrix& rMatrix, const OpenViBE::CString& sFilename)
{
	std::ifstream m_oDataFile;
	m_oDataFile.open(sFilename.toASCIIString(), std::ios_base::in);
	if(!m_oDataFile.is_open()) {
		return false;
	}

	std::stringstream l_oBuffer; 

	l_oBuffer << m_oDataFile.rdbuf();

	const boolean l_bReturnValue = OpenViBEToolkit::Tools::Matrix::fromString(rMatrix, CString(l_oBuffer.str().c_str()));

	m_oDataFile.close();

	return l_bReturnValue;
}

boolean OpenViBEToolkit::Tools::Matrix::saveToTextFile(const OpenViBE::IMatrix& rMatrix, const OpenViBE::CString& sFilename, uint32 ui32Precision /* = 6 */)
{
	std::ofstream m_oDataFile;
	m_oDataFile.open(sFilename.toASCIIString(), std::ios_base::out | std::ios_base::trunc);
	if(!m_oDataFile.is_open()) {
		return false;
	}
	
	CString l_sMatrix;

	if(!OpenViBEToolkit::Tools::Matrix::toString(rMatrix, l_sMatrix, ui32Precision))
	{
		return false;
	}
	
	m_oDataFile << l_sMatrix.toASCIIString();

	m_oDataFile.close();

	return true;
}

