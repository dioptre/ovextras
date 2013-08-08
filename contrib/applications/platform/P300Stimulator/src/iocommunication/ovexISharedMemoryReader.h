#ifndef __ovexISharedMemoryReader__
#define __ovexISharedMemoryReader__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <cstring>
//#include <fstream>
#include <iostream>
//#include <sstream>
#include <cstdio>
#include <cstdlib>
//#include <pair>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

using namespace boost::interprocess;

namespace OpenViBEApplications
{
	class ISharedMemoryReader
	{
	public:
		class SharedVariableHandler
		{
		private:
			std::vector<ISharedMemoryReader*>* m_vSharedMemoryVariables;
			OpenViBE::boolean m_bFailedToOpen;
			
		public:
			SharedVariableHandler(OpenViBE::CString sharedMemoryName);
			
			~SharedVariableHandler()
			{
				for (OpenViBE::uint32 i=0; i<m_vSharedMemoryVariables->size(); i++)
					delete m_vSharedMemoryVariables->at(i);
				m_vSharedMemoryVariables->clear();
				delete m_vSharedMemoryVariables;
			}
			
			//void initialize(OpenViBE::CString sharedMemoryName);
			//void destroy();
			OpenViBE::IObject* front(OpenViBE::uint32 inputIndex);
			OpenViBE::IObject* pop_front(OpenViBE::uint32 inputIndex);
			void clear(OpenViBE::uint32 inputIndex);
		};		
		
		ISharedMemoryReader() : m_sSharedMemoryName("")
		{
			m_bFailedToFind = false;
		}
		
		ISharedMemoryReader(OpenViBE::CString sharedMemoryName, OpenViBE::CString sharedVariableName) 
		: m_sSharedMemoryName(sharedMemoryName), m_sSharedVariableName(sharedVariableName)
		{
			m_bFailedToFind = false;
		}
		
		virtual ~ISharedMemoryReader()
		{
			std::cout << "Deconstructor SharedMemoryReader is called\n";
			named_mutex::remove(m_sMutexName.toASCIIString());
			delete m_pMutex;
		}

	protected:

		virtual OpenViBE::boolean open() = 0;
		virtual OpenViBE::boolean open(OpenViBE::CString sharedMemoryName, OpenViBE::CString sharedVariableName) = 0;
		virtual OpenViBE::IObject* front() = 0;
		virtual OpenViBE::IObject* pop_front() = 0;
		virtual void clear() = 0;
		virtual void close() = 0;

	protected:

		managed_shared_memory m_oSharedMemory;
		OpenViBE::CString m_sSharedMemoryName;
		OpenViBE::CString m_sSharedVariableName;
		OpenViBE::boolean m_bFailedToFind;
		named_mutex* m_pMutex;
		
	private:
		OpenViBE::CString m_sMutexName;
	};
};
#endif