#include "ovbt_sg_defines.h"
#include "ovbt_sg_file_generator_base.h"

#include <fstream>

using namespace std;

bool CCppCodeGenerator::openFile(const char* sFilename)
{
	m_oFile.open(sFilename, ios::out | ios::trunc);
	if(!m_oFile.is_open())
		return false;
	m_oFile << "#include \"toolkit/ovtk_all.h\"" << endl << endl;


	m_oFile << "using namespace OpenViBE;" << endl;
	m_oFile << "using namespace OpenViBE::Kernel;" << endl;
	m_oFile << "using namespace OpenViBEToolkit;" << endl << endl << endl;

	m_oFile << "boolean OpenViBEToolkit::initializeStimulationList(const IKernelContext& rKernelContext)" << endl;
	m_oFile << "{" << endl;
	m_oFile << "\tITypeManager& l_rTypeManager=rKernelContext.getTypeManager();" << endl << endl;
	return true;
}


bool CCppCodeGenerator::appendStimulation(SStimulation &rStim)
{
	m_oFile << "\tl_rTypeManager.registerEnumerationEntry(OV_TypeId_Stimulation, \""
			<< rStim.m_sName
			<< "\", "
			<< rStim.m_sId
			<< ");"
			<< endl;
	return true;
}


bool CCppCodeGenerator::closeFile(void)
{
	m_oFile << endl << "\treturn true;" << endl;
	m_oFile << "}" << endl;
	m_oFile.close();
	return true;
}
