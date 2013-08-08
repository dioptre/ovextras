#ifndef __ovExternalP300SharedMemoryReader__
#define __ovExternalP300SharedMemoryReader__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <cstring>
//#include <fstream>
#include <iostream>
//#include <sstream>
#include <cstdio>
#include <cstdlib>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include "iocommunication/ovexISharedMemoryReader.h"

using namespace boost::interprocess;

namespace OpenViBEApplications
{
	typedef allocator<OpenViBE::uint32, managed_shared_memory::segment_manager>  ShmemAllocator;
	typedef allocator<OpenViBE::uint32, managed_shared_memory::segment_manager>  ShmemAllocatorStimulation;
	typedef allocator<OpenViBE::CMatrix, managed_shared_memory::segment_manager>  ShmemAllocatorMatrix;
	typedef vector<OpenViBE::uint32, ShmemAllocator> MyVector;
	typedef vector<OpenViBE::uint32, ShmemAllocatorStimulation> MyVectorStimulation;
	typedef vector<OpenViBE::CMatrix, ShmemAllocatorMatrix> MyVectorStreamedMatrix;

	class ExternalP300SharedMemoryReader
	{
		public:
			void openSharedMemory(OpenViBE::CString sharedMemoryName);
			OpenViBE::uint64 readNextPrediction();
			OpenViBE::IMatrix* readNextSymbolProbabilities();
			void clearSymbolProbabilities();
			void closeSharedMemory();

		protected:

			ISharedMemoryReader::SharedVariableHandler* m_pSharedVariableHandler;
	};
};
#endif
