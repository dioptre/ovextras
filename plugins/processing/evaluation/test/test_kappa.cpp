#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace std;

int main (int argc, char** argv)
{
	if(argc!=2) 
	{
		cout << "Usage: test_evaluation <filename>\n";
		return 3;
	}

	ifstream file(argv[1], ios::in);

	if(file.good() && !file.bad() && file.is_open()) // ...
	{
		string line;
		while(getline(file, line))
		{
			size_t pos;
			if(line.find("Final value of Kappa") != string::npos)
			{
				cout << "Found kappa line " << line << endl; 
			  
				pos = line.rfind(" ");
				string cutline = line.substr(pos);

				stringstream l_sKappa(cutline);

				double l_fKappaCoefficient;
				l_sKappa >> l_fKappaCoefficient;

				if(l_fKappaCoefficient != 0.840677)
				{
					cout << "Wrong Kappa coefficient. Found " << l_fKappaCoefficient << " instead of 0.840677" <<  endl;
					return 1;
				}
				else{
					cout << "Test ok" << endl;
					return 0;
				}
			}
		}
	}
	cout << "Error: Problem opening [" << argv[1] << "]\n";

	return 2;
}
