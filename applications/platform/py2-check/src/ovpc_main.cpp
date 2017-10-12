#if defined(WIN32) && !defined(TARGET_BUILDTYPE_Debug) && defined TARGET_HAS_ThirdPartyPython 
//&& defined(PY_MAJOR_VERSION) && (PY_MAJOR_VERSION == 2)

#include <Python.h>
#define PYCHECK

#else
#pragma message ("WARNING: Python 2.x headers are required to build the Python plugin, different includes found, skipped")
#endif

#include <string>
#include <iostream>

int main(int argc, char** argv)
{
	// std::cout << "hello" << std::endl;
#ifdef PYCHECK
	// std::cout << "hello1" << std::endl;
	Py_Initialize();
	// std::cout << "hello2" << std::endl;
	Py_Finalize();
	// std::cout << "hello3" << std::endl;
	return 0;
#else
	return 1;
#endif
}
