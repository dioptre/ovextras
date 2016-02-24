/*
 * \author Jussi T. Lindgren / Inria
 *
 * This is currently rather a simple benchmark than a test. It can be used to see how Eigen loads the cores,
 * simulating spatial filter (matrix multiplication use) with large matrices. Basically the core load should be
 * smaller during the stream simulation test than the burn test, unless 1) OpenMP/Eigen is busy-waiting the cores
 * 2) the matrix sizes really demand all the computational power available.
 *
 * The behavior may also depend on the matrix sizes.
 *
 * \date 24.02.2016
 */

#include <iostream>

#if defined(TARGET_HAS_ThirdPartyEIGEN)
#include <Eigen/Dense>

#if defined(TARGET_OS_Windows)
#include <Windows.h>
#include <MMSystem.h>
#endif

#include <system/ovCTime.h>
#include <openvibe/ovITimeArithmetics.h>

typedef Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

int main(int argc, char *argv[])
{
	int returnValue = 0;

#if defined(TARGET_OS_Windows)
	// Set the clock precision to 1ms (default on Win7: 15ms)
	timeBeginPeriod(1);
#endif

	const unsigned int nChannelsIn  = 256;
	const unsigned int nChannelsOut = 128;
	const unsigned int chunkSize = 32;
	const System::uint64 samplingFreq = 1000;

	Eigen::MatrixXd filterMatrix,dataMatrix;
	filterMatrix.resize(nChannelsOut, nChannelsIn);
	dataMatrix.resize(nChannelsIn, chunkSize);

	filterMatrix.setRandom();
	dataMatrix.setRandom();
	
#if EIGEN_MAJOR_VERSION >= 2 && EIGEN_MINOR_VERSION >= 8
	//Eigen::setNbThreads(1);
	//Eigen::initParallel();

	std::cout << "Eigen is using " << Eigen::nbThreads() << " threads\n";
#endif

	const double chunksPerSec = samplingFreq / static_cast<double>(chunkSize);
	const System::uint64 chunkDuration = OpenViBE::ITimeArithmetics::sampleCountToTime(samplingFreq, chunkSize);

	std::cout << "Filter is " << nChannelsOut << "x" << nChannelsIn << ", data chunk is " << nChannelsIn << "x" << chunkSize << "\n";

	System::uint64 before = System::Time::zgetTime();
	System::uint64 after = before;

	if (1)
	{
		System::uint64 matricesProcessed = 0;
		System::uint64 timeOut = 20;
		std::cout << "Running multiplication burn test\n";
		while (after - before < (timeOut << 32))
		{
			Eigen::MatrixXd output = filterMatrix*dataMatrix;
			matricesProcessed++;
			after = System::Time::zgetTime();
		}
		std::cout << "Managed to do " << matricesProcessed << " multiplications in " << timeOut << "s, " << matricesProcessed/static_cast<double>(timeOut) << " per sec.\n";
	}
	
	std::cout << "Running scheduler simulator\n";

	std::cout << "Using " << samplingFreq << "Hz sampling frequency, requires " << chunksPerSec << " chunks per sec\n";
	std::cout << "Time available for each chunk is " << OpenViBE::ITimeArithmetics::timeToSeconds(chunkDuration * 1000) << "ms.\n";

	System::uint64 timeUsed = 0;
	const System::uint64 totalChunks = static_cast<System::uint64> (30 * chunksPerSec);

	for (System::uint64 i = 0; i < totalChunks; i++)
	{
		before = System::Time::zgetTime();

		Eigen::MatrixXd output = filterMatrix*dataMatrix;

		after = System::Time::zgetTime();

		timeUsed += (after - before);

		// busy wait, this is the OV scheduler default
		if (1)
		{
			while (after - before < chunkDuration)
			{
				after = System::Time::zgetTime();
			}
		}
		else
		{
			System::uint64 elapsed = after - before;
			if (elapsed < chunkDuration)
			{
				System::Time::zsleep(chunkDuration - elapsed);
			}
		}
	}

	std::cout << "Avg chunk process time was " << OpenViBE::ITimeArithmetics::timeToSeconds(timeUsed / totalChunks * 1000) << "ms.\n";

	if (timeUsed / totalChunks > chunkDuration)
	{
		std::cout << "Error: Cannot reach realtime with these parameters\n";
		returnValue = 1;
	}
	std::cout << "All tests completed\n";

#if defined(TARGET_OS_Windows)
	timeEndPeriod(1);
#endif

	return returnValue;

}

#else
int main(int argc, char *argv[])
{
	std::cout << "Eigen was not present on the system when compiling\n";
	return 1;
}
#endif
