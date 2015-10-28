#include "ovbt_sg_defines.h"
#include "ovbt_sg_file_generator_base.h"

#include <fstream>

using namespace std;


string getBrutHexaCode(string l_oFormatedHexaCode)
{
	string res = l_oFormatedHexaCode;
	res.erase(res.begin(), res.begin()+2);
	return res;
}

bool CMatlabGenerator::openFile(const char* sFilename)
{
	m_oFile.open(sFilename, ios::out | ios::trunc);
	if(!m_oFile.is_open())
		return false;
	m_oFile << "function OV_stimulations()" << endl << endl;

	m_oFile << "global OVTK_StimulationId_LabelStart;"<< endl;
	m_oFile << "OVTK_StimulationId_LabelStart = uint64(hex2dec('00008100'));"<< endl << endl;
	m_oFile << "global OVTK_StimulationId_LabelEnd;"<< endl;
	m_oFile << "OVTK_StimulationId_LabelEnd = uint64(hex2dec('000081ff'));"<< endl << endl;

	return true;
}

bool CMatlabGenerator::appendStimulation(SStimulation &rStim)
{
	m_oFile << "\tglobal " << rStim.m_sId << ";" << endl;
	m_oFile << "\t" << rStim.m_sId << " = uint64(hex2dec('" << getBrutHexaCode(rStim.m_sHexaCode)<< "'));" << endl << endl;
	return true;
}


bool CMatlabGenerator::closeFile(void)
{
	m_oFile << "end" << endl;
	m_oFile.close();
	return true;
}
