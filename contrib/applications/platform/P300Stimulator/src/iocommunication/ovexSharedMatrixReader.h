#ifndef __ovexSharedMatrixReader__
#define __ovexSharedMatrixReader__

#include "ovexISharedMemoryReader.h"

/*#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>*/

namespace OpenViBEApplications
{
	struct SMatrix
	{
		int rowDimension;
		int columnDimension;
		offset_ptr<double> data;
	};	
	
	typedef allocator<offset_ptr<SMatrix>, managed_shared_memory::segment_manager>  ShmemAllocatorMatrix;
	typedef vector<offset_ptr<SMatrix>, ShmemAllocatorMatrix> MyVectorStreamedMatrix;	

	class SharedMatrixReader : public ISharedMemoryReader
	{
	public:
		SharedMatrixReader() : ISharedMemoryReader() {}
		SharedMatrixReader(OpenViBE::CString sharedMemoryName, OpenViBE::CString sharedVariableName) 
		: ISharedMemoryReader(sharedMemoryName, sharedVariableName) {}
			
		virtual ~SharedMatrixReader()
		{
			//ISharedMemoryReader::~ISharedMemoryReader();
			//normally the client should not close/clean up the allocated memory, the creating application should 
		}
	protected:
		virtual OpenViBE::boolean open();
		virtual OpenViBE::boolean open(OpenViBE::CString sharedMemoryName, OpenViBE::CString sharedVariableName);
		virtual OpenViBE::IMatrix* front();
		virtual OpenViBE::IMatrix* pop_front();
		virtual void clear();
		virtual void close();	
		
	private:
		OpenViBE::IMatrix* _front();
		
	protected:
		MyVectorStreamedMatrix* m_vMatrixVector;
	};
};

#endif