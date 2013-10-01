#ifndef __ovexSharedStimulusReader__
#define __ovexSharedStimulusReader__

#include "ovexISharedMemoryReader.h"

/*#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>*/

namespace OpenViBEApplications
{

	typedef allocator<OpenViBE::uint32, managed_shared_memory::segment_manager>  ShmemAllocatorStimulation;
	typedef vector<OpenViBE::uint32, ShmemAllocatorStimulation> MyVectorStimulation;
	
	class SharedStimulusReader : public ISharedMemoryReader
	{
		
	public:
		SharedStimulusReader() : ISharedMemoryReader() {}
		SharedStimulusReader(OpenViBE::CString sharedMemoryName, OpenViBE::CString sharedVariableName) 
		: ISharedMemoryReader(sharedMemoryName, sharedVariableName) {}
			
		virtual ~SharedStimulusReader()
		{
			//ISharedMemoryReader::~ISharedMemoryReader();	
			//normally the client should not close/clean up the allocated memory, the creating application should 
		}
	protected:
		virtual OpenViBE::boolean open();
		virtual OpenViBE::boolean open(OpenViBE::CString sharedMemoryName, OpenViBE::CString sharedVariableName);
		virtual OpenViBE::IStimulationSet* front();
		virtual OpenViBE::IStimulationSet* pop_front();
		virtual void close();	
		virtual void clear();
		
	private:
		OpenViBE::IStimulationSet* _front();
		
	protected:
		MyVectorStimulation* m_vStimulusVector;
	};
};

#endif