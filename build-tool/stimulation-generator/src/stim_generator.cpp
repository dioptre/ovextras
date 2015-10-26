#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "defines.h"
#include "file_generator_base.h"

using namespace std;


string getBrutHexaCode(string l_oFormatedHexaCode)
{
   string res = l_oFormatedHexaCode;
   res.erase(res.begin(), res.begin()+2);
   return res;
}

int main (int argc, char** argv)
{
  if(argc < 3)
    return -1;
  vector<SStimulation> l_oStimulationList;
  vector<CFileGeneratorBase*> l_oGeneratorList;
  
  ifstream l_oStimulationFile(argv[1]);
  string l_sName, l_sId, l_sHexaCode;
  while(l_oStimulationFile >> l_sName >> l_sId >> l_sHexaCode)
  {
    SStimulation l_oTemp = {l_sName, l_sId, l_sHexaCode};
    l_oStimulationList.push_back(l_oTemp);
  }
  
  CFileGeneratorBase* gen = new CCppDefineGenerator();
  if(!gen->openFile(argv[2]))
  {
	  cerr << "Unable to open " << argv[2] << endl;
  }
  l_oGeneratorList.push_back(gen);
  
  gen = new CCppCodeGenerator();
  if(!gen->openFile(argv[3]))
  {
	  cerr << "Unable to open " << argv[3] << endl;
  }
  l_oGeneratorList.push_back(gen);
  
  //Now we generate all files that needs to be done
  for(vector<SStimulation>::iterator it = l_oStimulationList.begin(); it != l_oStimulationList.end(); ++it)
  {
    SStimulation &l_oTemp = *it;
	//cout << l_oTemp.m_sName << "," << l_oTemp.m_sId << "," << l_oTemp.m_sHexaCode << endl;
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
