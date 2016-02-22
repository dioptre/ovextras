
/*
 * Tests writing and reading of CMatrices from an ASCII format on the disk. Since
 * the format doesn't retain full precision, the test takes this into account.
 *
 */

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include <toolkit/ovtk_all.h>
#include <system/ovCMath.h>

using namespace std;
using namespace OpenViBE;

static const uint32 precision = 6;
static const char textFile[]="output_matrix.txt";

void fillMatrix(CMatrix& mat) 
{
	for(uint32 i=0;i<mat.getDimensionCount();i++) 
	{
		for(uint32 j=0;j<mat.getDimensionSize(i);j++)
		{
			std::stringstream dimensionLabel;

			dimensionLabel << "Dim " << j+1 << " of " << i+1 ;

			mat.setDimensionLabel(i,j, dimensionLabel.str().c_str());
		}
	}

	for(uint32 i=0;i<mat.getBufferElementCount();i++) {
		mat.getBuffer()[i] = System::Math::randomFloat32BetweenZeroAndOne() * System::Math::randomSInterger8();
	}
}

void printMatrixInfo(CMatrix &source) {
	std::cout << "Test dimCount=" << source.getDimensionCount() << " matrix of subdimensions ";
	for(uint32 i=0;i<source.getDimensionCount(); i++)
	{
		std::cout << source.getDimensionSize(i) << " ";
	}
	std::cout << "\n";
}

bool testMatrix(CMatrix &source) {

	OpenViBE::CMatrix dest;

	const float64 th = 1.0 / std::pow(10.0,(double)(precision-2));

	printMatrixInfo(source); 

	fillMatrix(source);
	if(!OpenViBEToolkit::Tools::Matrix::saveToTextFile(source, textFile, precision)) 
	{ 
		std::cout << "  Error: saving to file " << textFile << "\n";
		return false;
	}
	if(!OpenViBEToolkit::Tools::Matrix::loadFromTextFile(dest, textFile))
	{
		std::cout << "  Error: loading matrix from file " << textFile << "\n";
		return false;
	}
	

	if(!OpenViBEToolkit::Tools::Matrix::isDescriptionSimilar(source,dest))
	{
		std::cout << "  Error: Descriptions differ between saved and loaded\n";
		return false;
	}
	for(uint32 i=0;i<source.getBufferElementCount();i++) 
	{
		const float64 error =  std::fabs(source.getBuffer()[i] - dest.getBuffer()[i]);
		if( error > th) 
		{
			std::cout << "  Error: Data differed too much at index " << i << ", error " << error << " (th = " << th << ")\n";
			
			return false;
		}
	}

	return true;
}

int main (int argc, char** argv)
{
	System::Math::initializeRandomMachine(777);

	bool allOkay = true;

	OpenViBE::CMatrix source;

	source.setDimensionCount(1);
	source.setDimensionSize(0,1);
	allOkay &= testMatrix(source);

	source.setDimensionCount(1);
	source.setDimensionSize(0,5);
	allOkay &= testMatrix(source);

	source.setDimensionCount(2);
	source.setDimensionSize(0,1);
	source.setDimensionSize(1,1);
	allOkay &= testMatrix(source);

	source.setDimensionCount(2);
	source.setDimensionSize(0,1);
	source.setDimensionSize(1,7);
	allOkay &= testMatrix(source);

	source.setDimensionCount(2);
	source.setDimensionSize(0,9);
	source.setDimensionSize(1,1);
	allOkay &= testMatrix(source);

	source.setDimensionCount(2);
	source.setDimensionSize(0,2);
	source.setDimensionSize(1,4);
	allOkay &= testMatrix(source);

	source.setDimensionCount(2);
	source.setDimensionSize(0,3);
	source.setDimensionSize(1,15);
	allOkay &= testMatrix(source);

	source.setDimensionCount(3);
	source.setDimensionSize(0,1);
	source.setDimensionSize(1,1);
	source.setDimensionSize(2,1);
	allOkay &= testMatrix(source);

	source.setDimensionCount(3);
	source.setDimensionSize(0,1);
	source.setDimensionSize(1,1);
	source.setDimensionSize(2,5);
	allOkay &= testMatrix(source);

	source.setDimensionCount(3);
	source.setDimensionSize(0,2);
	source.setDimensionSize(1,3);
	source.setDimensionSize(2,6);
	allOkay &= testMatrix(source);

	source.setDimensionCount(4);
	source.setDimensionSize(0,9);
	source.setDimensionSize(1,5);
	source.setDimensionSize(2,2);
	source.setDimensionSize(3,3);
	allOkay &= testMatrix(source);

	return (allOkay ? 0 : 1);
}
