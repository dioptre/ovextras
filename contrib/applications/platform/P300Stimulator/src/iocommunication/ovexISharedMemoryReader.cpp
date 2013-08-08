#include "ovexISharedMemoryReader.h"
#include "ovexSharedMatrixReader.h"
#include "ovexSharedStimulusReader.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

typedef allocator<char, managed_shared_memory::segment_manager> CharAllocator;
typedef basic_string<char, std::char_traits<char>, CharAllocator> ShmString;
//typedef allocator<ShmString, managed_shared_memory::segment_manager> StringAllocator;      	
typedef allocator< std::pair<ShmString, OpenViBE::CIdentifier > , managed_shared_memory::segment_manager>  ShmemAllocatorMetaInfo;
typedef vector< std::pair<ShmString, OpenViBE::CIdentifier >, ShmemAllocatorMetaInfo > MyVectorMetaInfo;	

ISharedMemoryReader::SharedVariableHandler::SharedVariableHandler(CString sharedMemoryName)
{
	m_vSharedMemoryVariables = new std::vector<ISharedMemoryReader*>();
	m_bFailedToOpen = false;
	managed_shared_memory l_oSharedMemory;
	try
	{
		l_oSharedMemory = managed_shared_memory(open_only, sharedMemoryName.toASCIIString());
		MyVectorMetaInfo* l_vMetaInfo = l_oSharedMemory.find< MyVectorMetaInfo >("MetaInfo").first;
		for (uint32 i=0; i<l_vMetaInfo->size(); i++)
		{
			const char *  l_sVariableName = l_vMetaInfo->at(i).first.c_str();
			if (l_vMetaInfo->at(i).second==OVTK_TypeId_StreamedMatrix)
			{
				ISharedMemoryReader* l_pMatrixReader = new SharedMatrixReader(sharedMemoryName,CString(l_sVariableName));
				if(l_pMatrixReader->open())
				{
					m_vSharedMemoryVariables->push_back(l_pMatrixReader);
					std::cout << "SharedVariableHandler created reader object for streamed matrix variable with name " << l_sVariableName << "\n";
				}
			}
			else if (l_vMetaInfo->at(i).second==OVTK_TypeId_Stimulations)
			{
				ISharedMemoryReader* l_pStimulusReader = new SharedStimulusReader(sharedMemoryName,CString(l_sVariableName));
				if(l_pStimulusReader->open())
				{
					m_vSharedMemoryVariables->push_back(l_pStimulusReader);
					std::cout << "SharedVariableHandler created reader object for stimuli variable with name " << l_sVariableName << "\n";
				}
			}
		}
	}
	catch (interprocess_exception &ex) 
	{ 
		m_bFailedToOpen = true;
		std::cerr << "Opening shared memory failed, make sure that the OpenViBE scenario is running. Error: " << ex.what() << std::endl; 
	} 	
}

IObject* ISharedMemoryReader::SharedVariableHandler::front(uint32 inputIndex)
{
	if (!m_bFailedToOpen && inputIndex<m_vSharedMemoryVariables->size())
		return m_vSharedMemoryVariables->at(inputIndex)->front();
	else
		return NULL;
}

IObject* ISharedMemoryReader::SharedVariableHandler::pop_front(uint32 inputIndex)
{
	if (!m_bFailedToOpen && inputIndex<m_vSharedMemoryVariables->size())
		return m_vSharedMemoryVariables->at(inputIndex)->pop_front();
	else
		return NULL;
}

void ISharedMemoryReader::SharedVariableHandler::clear(uint32 inputIndex)
{
	if (!m_bFailedToOpen && inputIndex<m_vSharedMemoryVariables->size())
		m_vSharedMemoryVariables->at(inputIndex)->clear();
}

boolean ISharedMemoryReader::open()
{
	m_sMutexName = m_sSharedMemoryName+CString("_Mutex");
	m_pMutex = new named_mutex(open_or_create, m_sMutexName.toASCIIString());
	
	try
	{
		m_oSharedMemory = managed_shared_memory(open_only, m_sSharedMemoryName.toASCIIString());	
		/*bool b1 = m_oSharedMemory.check_sanity();
		int n = m_oSharedMemory.get_num_named_objects();
		std::cout << "Sanity check of shared memory " << b1 << ", number of objects found: " << n << "\n";*/
	}
	catch (interprocess_exception &ex) 
	{ 
		m_bFailedToFind = true;
		std::cerr << "Opening shared memory failed, make sure that the OpenViBE scenario is running. Error: " << ex.what() << std::endl; 
	} 
	
	return m_bFailedToFind;
}

boolean ISharedMemoryReader::open(CString sharedMemoryName, CString sharedVariableName)
{
	m_sSharedMemoryName = sharedMemoryName;
	m_sSharedVariableName = sharedVariableName;
	return this->open();
}

void ISharedMemoryReader::close()
{
	shared_memory_object::remove(m_sSharedMemoryName.toASCIIString());
}