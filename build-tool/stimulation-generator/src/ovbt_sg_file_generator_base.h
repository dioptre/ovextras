#include "ovbt_sg_defines.h"

#include <fstream>

class CFileGeneratorBase{
public:
	virtual bool openFile(const char* sFilename) =0;
	virtual bool appendStimulation(SStimulation &rStim) =0;
	virtual bool closeFile(void) =0;
protected:
	std::ofstream m_oFile;
};



class CCppDefineGenerator: public CFileGeneratorBase{
public:
	virtual bool openFile(const char* sFilename);
	virtual bool appendStimulation(SStimulation &rStim);
	virtual bool closeFile(void);
};

class CCppCodeGenerator: public CFileGeneratorBase{
public:
	virtual bool openFile(const char* sFilename);
	virtual bool appendStimulation(SStimulation &rStim);
	virtual bool closeFile(void);
};
