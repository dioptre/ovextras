#include "defines.h"
#include "file_generator_base.h"

#include <fstream>

using namespace std;

bool CCppDefineGenerator::openFile(const char* sFilename)
{
  m_oFile.open(sFilename, ios::out | ios::trunc);
  if(!m_oFile.is_open())
	  return false;
  m_oFile << "#ifndef __OpenViBEToolkit_Stimulations_Defines_H__" << endl;
  m_oFile << "#define __OpenViBEToolkit_Stimulations_Defines_H__" << endl << endl;

  return true;
}


bool CCppDefineGenerator::appendStimulation(SStimulation &rStim)
{
  m_oFile << "#define " << rStim.m_sId << "  " << rStim.m_sHexaCode << endl;
  return true;
}


bool CCppDefineGenerator::closeFile(void)
{
  m_oFile << endl << "#endif // __OpenViBEToolkit_Stimulations_Defines_H__" << endl;
  m_oFile.close();
  return true;
}
