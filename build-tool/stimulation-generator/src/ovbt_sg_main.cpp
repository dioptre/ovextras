#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ovbt_sg_defines.h"
#include "ovbt_sg_file_generator_base.h"

using namespace std;

typedef enum{
	CPP, MATLAB, PYTHON, LUA, UNKNOWN
}generation_type;


generation_type parse_argument(string option)
{
	if(option == "--cpp"){
		return CPP;
	}
	else if(option == "--matlab"){
		return MATLAB;
	}
	else if(option == "--python"){
		return PYTHON;
	}
	else if(option == "--lua"){
		return LUA;
	}
	else{
		return UNKNOWN;
	}
}

string getBrutHexaCode(string l_oFormatedHexaCode)
{
	string res = l_oFormatedHexaCode;
	res.erase(res.begin(), res.begin()+2);
	return res;
}

int generate_generator_list(vector<CFileGeneratorBase*> & rList, generation_type rType, int argc, char** argv)
{
	switch(rType)
	{
		case CPP:
		{
			if(argc < 4){
				return -1;
			}
			CFileGeneratorBase* gen = new CCppDefineGenerator();
			if(!gen->openFile(argv[3])){
				cerr << "Unable to open " << argv[3] << endl;
				return -1;
			}
			rList.push_back(gen);

			gen = new CCppCodeGenerator();
			if(!gen->openFile(argv[4])){
				cerr << "Unable to open " << argv[4] << endl;
				return -1;
			}
			rList.push_back(gen);
			return 0;
		}

		case MATLAB:
		case PYTHON:
		case LUA:
		case UNKNOWN:
		default:
		{
			cerr << "Unhandle type. Fatal error" << endl;
			return -1;
		}
	}
}

int main (int argc, char** argv)
{
	if(argc < 3)
		return -1;
	generation_type l_eType = parse_argument(argv[1]);

	vector<SStimulation> l_oStimulationList;
	vector<CFileGeneratorBase*> l_oGeneratorList;

	ifstream l_oStimulationFile(argv[2]);
	string l_sName, l_sId, l_sHexaCode;
	while(l_oStimulationFile >> l_sName >> l_sId >> l_sHexaCode)
	{
		SStimulation l_oTemp = {l_sName, l_sId, l_sHexaCode};
		l_oStimulationList.push_back(l_oTemp);
	}

	if(generate_generator_list(l_oGeneratorList, l_eType, argc, argv)){
		return -1;
	}

	//Now we generate all files that needs to be done
	for(vector<SStimulation>::iterator it = l_oStimulationList.begin(); it != l_oStimulationList.end(); ++it)
	{
		SStimulation &l_oTemp = *it;
		for(vector<CFileGeneratorBase*>::iterator it_gen = l_oGeneratorList.begin(); it_gen != l_oGeneratorList.end(); ++it_gen)
		{
			(*it_gen)->appendStimulation(l_oTemp);
		}
	}

	for(vector<CFileGeneratorBase*>::iterator it_gen = l_oGeneratorList.begin(); it_gen != l_oGeneratorList.end(); ++it_gen)
	{
		(*it_gen)->closeFile();
	}

	return 0;
}
