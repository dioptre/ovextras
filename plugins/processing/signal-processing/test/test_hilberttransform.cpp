/*
 * \author Alison Cellard / Inria
 * \date 30.08.2013
 */

#include <plugins/ovpCHilbertTransform.h>
#include <fstream>


using namespace OpenViBE;
using namespace std;



/* Method to read a file a store it in a buffer */
std::vector fileToBuffer(const Cstring sFileName)
{
	std::vector l_vBuffer;
	std::fstream l_file;

	l_file.open(sFileName, fstream::in);
	while(l_file.good())
	{
		float64 l_f64data;
		l_file >> l_f64data;
		l_vBuffer.push_back(l_f64data);
	}

	return l_vBuffer;
}



/* Method to compare two files */
boolean compareBuffers (const float* &rMatrixBuffer, const std::vector<float64>& rVector, const float64 f64precision)
{
	int cpt = 0;
	float64 l_f64DiffValue;
	// Compare size
	if(rMatrixBuffer->getDimesionSize(0) != rVector.Size())
	{
		return false;
	}
	else // If size are the same compare the data
	{
		l_f64DiffValue = rMatrixBuffer[cpt]-rVector[cpt];
		while(abs(l_f64DiffValue) <= f64precision && cpt < rVector.Size());
		{
			cpt++;
			l_f64DiffValue = rMatrixBuffer[cpt]-rVector[cpt];
		}

		if (cpt == rVector.Size()-1)
		{
			return true;
		}
		else
		{
			return false;
		}

	}
}


/*void initializeHilbertAlgorithm(void)
{
}*/




/* validationTest tests the output with a given input */

	/*
	 *  - open matlab file and store it in buffer -> fileToBuffer
	 *  - compute Hilbert algorithm
	 *  - compare Buffers (hilbert, envelope and phase) -> compareBuffers
	 *  - write out the result in console
	 *
	 */

// validation Test has 5 parameters : the input file containing the data of the input signal to apply algorithm to, the expected outputs(Phase,envelope and hilbert transform) computed by matlab, and the precision tolerance
boolean validationTest(const CString sInputFile, const CString sEnvelopeOutputFile, const CString sPhaseOutputFile, const CString sHilbertOutputFile, const float64 f64precision)
{
	boolean l_bTestPassed = false;
	boolean l_bIsHilbertCorrect = false;
	boolean l_bIsPhaseCorrect = false;
	boolean l_bIsEnvelopeCorrect = false;


	// Get data from input file, and the file containing the correct output
	std::vector<float64> l_vInputVector;
	std::vector<float64> l_vExpectedEnvelopeVector;
	std::vector<float64> l_vExpectedPhaseVector;
	std::vector<float64> l_vExpectedHilbertVector;

	l_vInputVector = fileToBuffer(sInputFile);
	l_vExpectedEnvelopeVector = fileToBuffer(sEnvelopeOutputFile);
	l_vExpectedPhaseVector = fileToBuffer(sPhaseOutputFile);
	l_vExpectedHilbertVector = fileToBuffer(sHilbertOutputFile);


	// Create algorithm instance of Hilbert transform

	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> ip_pHilbertInput;
	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pPhaseMatrix;
	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pHilbertMatrix;
	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pEnvelopeMatrix;

	OpenViBE::Kernel::IAlgorithmProxy* l_pHilbertTransform = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_HilbertTransform));
	l_pHilbertTransform->initialize();

	ip_pHilbertInput.initialize(m_pHilbertTransform->getInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix));
	op_pHilbertMatrix.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix));
	op_pEnvelopeMatrix.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix));
	op_pPhaseMatrix.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix));

	// Setting size of input
	ip_pHilbertInput->setDimensionCount(2); // the input matrix will have 2 dimensions
	ip_pHilbertInput->setDimensionSize(0,l_vInputVector.Size()); //
	ip_pHilbertInput->setDimensionSize(1,1);//

	for(uint32 i =0; i<l_vInputVector.Size();i++)
	{
		ip_pHilbertInput->getBuffer()[i] = l_vInputVector[i];
	}

	if(l_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize)) // Check initialization before doing the process
	{
		m_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Process);

		l_bIsEnvelopeCorrect = compareBuffers(op_pEnvelopeMatrix->getBuffer(), l_vExpectedEnvelopeVector,f64precision);
		l_bIsPhaseCorrect = compareBuffers(op_pPhaseMatrix->getBuffer(), l_vExpectedPhaseVector,f64precision);
		l_bIsHilbertCorrect = compareBuffers(op_pHilbertMatrix->getBuffer(), l_vExpectedHilbertVector,f64precision);


		if(l_bIsHilbertCorrect && l_bIsPhaseCorrect && l_bIsEnvelopeCorrect)
		{
			cout<<"All results are right, test passed \n"<<endl;
			return true;
		}
		else
		{
			cout<<"Algorithm results do not correspond to expected results, test failed \n"<<endl;
			return false;
		}
	}
	else
	{
		cout<<"Algorithm initialization failed, test failed \n"<<endl;
		return false;
	}


	ip_pHilbertInput.uninitialize();
	op_pHilbertMatrix.uninitialize();
	op_pEnvelopeMatrix.uninitialize();
	op_pPhaseMatrix.uninitialize();

	l_pHilbertTransform->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pHilbertTransform);

}

/* Test with wrong input */

	/*
	 * - create buffer with wrong input size (e.g. size=1)
	 * - compute Hilbert algorithm
	 * - see if it crashes or not (how?)
	 *
	 */

boolean badInputTest(const std::vector<float64> vBadInput)
{
	boolean l_bTestPassed = false;

	// Create algorithm instance of Hilbert transform

	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> ip_pHilbertInput;
	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pPhaseMatrix;
	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pHilbertMatrix;
	OpenViBE::Kernel::TParameterHandler <OpenViBE::IMatrix*> op_pEnvelopeMatrix;

	OpenViBE::Kernel::IAlgorithmProxy* l_pHilbertTransform = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_HilbertTransform));
	l_pHilbertTransform->initialize();

	ip_pHilbertInput.initialize(m_pHilbertTransform->getInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix));
	op_pHilbertMatrix.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix));
	op_pEnvelopeMatrix.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix));
	op_pPhaseMatrix.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix));

	// Setting size of input
	ip_pHilbertInput->setDimensionCount(2); // the input matrix will have 2 dimensions
	ip_pHilbertInput->setDimensionSize(0,vBadInput.Size()); //
	ip_pHilbertInput->setDimensionSize(1,1);//

	for(uint32 i =0; i<vBadInput.Size();i++)
	{
		ip_pHilbertInput->getBuffer()[i] = vBadInput[i];
	}

	if(l_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize)) // Check initialization before doing the process
	{
		m_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Process);
		if(m_pHilbertTransform->isInputTriggerActive(OVP_Algorithm_HilbertTransform_InputTriggerId_ProcessDone))
		{
			cout<<"Process done without crashing, test passed \n"<<endl;
			return true;
		}
		else
		{
			cout<<"Algorithm failed, test failed \n"<<endl;
			return false;
		}
	}
	else
	{
		cout<<"Initialization failed, the algorithm identified the input was not what expected, the test is considered as passed \n"<<endl;
		return true;
	}


}



int main(int argc, char *argv[])
{
	const float64 l_f64precisionTolerance = 0.001;
	std::vector<float64> l_vBadInput (3);

	boolean l_bValidationTestPassed = false;
	boolean l_bBadInputTestPassed = false;

	l_bValidationTestPassed = validationTest("sin50.mat","envelopeSin50.mat","phaseSin50.mat", "hilbertSin50.mat",l_f64precisionTolerance);

	if(!l_bValidationTestPassed)
	{
		cout<<"Algorithm failed validation test \n"<<endl;
	}

	l_bBadInputTestPassed = badInputTest(l_vBadInput);
	if(!l_bBadInputTestPassed)
	{
		cout<<"Algorithm failed bad input test for input = "<<l_vBadInput<<"\n"<<endl;
	}

}
