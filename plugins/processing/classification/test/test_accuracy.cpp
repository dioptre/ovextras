#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

using namespace std;

double threshold = 72;

int main (int argc, char** argv)
{
	if(argc!=2 && argc!=3) 
	{
		cout << "Usage: test_accuracy <filename> <threshold>\n";
		return 3;
	}
	if(argc==3) 
	{
		threshold = atof(argv[2]);
	}

	ifstream file(argv[1], ios::in);

	if(file.good() && !file.bad() && file.is_open()) // ...
	{
		string line;
		while(getline(file, line))
		{
			size_t pos;
			if((pos = line.find("Cross-validation")) != string::npos)
			{
				string cutline =  line.substr(pos);
				pos = cutline.find("is")+ 3;//We need to cut the coloration
				cutline = cutline.substr(pos);

				pos = cutline.find("%");

				cutline = cutline.substr(0, pos);
				stringstream l_sPercentage(cutline);

				double l_fPercentage;
				l_sPercentage >> l_fPercentage;

				if(l_fPercentage < threshold)
				{
					cout << "Accuracy too low ( " << l_fPercentage << " % )" << endl;
					return 1;
				}
				else{
					cout << "Test ok ( " << l_fPercentage << " % )" << endl;
					return 0;
				}
			}
		}
	}
	cout << "Error: Problem opening [" << argv[1] << "]\n";
	cerr << "Error: " << strerror(errno);

	return 2;
}
