#include "ovpCBoxAlgorithmDLLBridge.h"

#include <system/ovCMemory.h>

#include <iostream>
#include <cstdio>

#if defined(TARGET_OS_Windows)
#include <windows.h>
#elif defined(TARGET_OS_Linux)
#include <dlfcn.h>
#endif


using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::DLLBridge;

using namespace OpenViBEToolkit;

using namespace std;

// #define DLLBRIDGE_DEBUG
#if defined(DLLBRIDGE_DEBUG)
static void dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const CMatrix& mat, const CString &desc)
{
	rMgr << LogLevel_Info << desc << "\n";
	for(uint32 i=0;i<mat.getDimensionSize(0);i++) {
		rMgr << LogLevel_Info << "Row " << i << ": ";
		for(uint32 j=0;j<mat.getDimensionSize(1);j++) {
			rMgr << mat.getBuffer()[i*mat.getDimensionSize(1)+j] << " ";
		}
		rMgr << "\n";
	}
}
#endif

namespace OpenViBEPlugins
{
	namespace DLLBridge
	{

		// This function is called once when user presses play in Designer
		OpenViBE::boolean CDLLBridge::initialize()
		{		
			this->getLogManager() << LogLevel_Debug << "Initializing\n";

			m_pEncoder = NULL;
			m_pDecoder = NULL;
			m_pLibrary = NULL;
			m_pInitialize = NULL;
			m_pProcessHeader = NULL;
			m_pProcess = NULL;
			m_pUninitialize = NULL;

			m_sDLLFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			m_sParameters =  FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			if(m_sDLLFile == CString(""))
			{
				this->getLogManager() << LogLevel_Error << "Need a DLL file\n";
				return false;
			}


#if defined(TARGET_OS_Windows)
			std::string tmp = m_sDLLFile.toASCIIString();
			std::replace( tmp.begin(), tmp.end(), '/', '\\');
			m_sDLLFile = tmp.c_str();
			
			m_pLibrary = LoadLibrary(m_sDLLFile.toASCIIString());
#elif defined(TARGET_OS_Linux)
			m_pLibrary = dlopen(m_sDLLFile.toASCIIString(), RTLD_LAZY);
#endif
			if(!m_pLibrary)
			{
#if defined(TARGET_OS_Windows)
				this->getLogManager() << LogLevel_Error << "Failed to load " << m_sDLLFile << ", error code " << (int64) GetLastError() << "\n";
#elif defined(TARGET_OS_Linux)
				this->getLogManager() << LogLevel_Error << "Failed to load " << m_sDLLFile << ", error code " << (int64) dlerror() << "\n";
#endif
				return false;
			}

#if defined(TARGET_OS_Windows)
			m_pInitialize = (INITFUNC)GetProcAddress(m_pLibrary, "box_init");
#elif defined(TARGET_OS_Linux)
			m_pInitialize = (INITFUNC)dlsym(m_pLibrary, "box_init");
#endif
			if(!m_pInitialize) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_init() in the DLL\n";
				return false;
			}

#if defined(TARGET_OS_Windows)
			m_pProcess = (PROCESSFUNC)GetProcAddress(m_pLibrary, "box_process");
#elif defined(TARGET_OS_Linux)
			m_pProcess = (PROCESSFUNC)dlsym(m_pLibrary, "box_process");
#endif
			if(!m_pProcess) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_process() in the DLL\n";
				return false;
			}

#if defined(TARGET_OS_Windows)
			m_pProcessHeader = (PROCESSHEADERFUNC)GetProcAddress(m_pLibrary, "box_process_header");
#elif defined(TARGET_OS_Linux)
			m_pProcessHeader = (PROCESSHEADERFUNC)dlsym(m_pLibrary, "box_process_header");
#endif
			if(!m_pProcessHeader) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_process_header() in the DLL\n";
				return false;
			}

#if defined(TARGET_OS_Windows)
			m_pUninitialize = (UNINITFUNC)GetProcAddress(m_pLibrary, "box_uninit");
#elif defined(TARGET_OS_Linux)
			m_pUninitialize = (UNINITFUNC)dlsym(m_pLibrary, "box_uninit");
#endif
			if(!m_pUninitialize) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_uninit() in the DLL\n";
				return false;
			}

			IBox& l_rStaticBoxContext=this->getStaticBoxContext();
			l_rStaticBoxContext.getInputType(0, m_oInputType);
			if(m_oInputType == OV_TypeId_StreamedMatrix) {
				OpenViBEToolkit::TStreamedMatrixDecoder< CDLLBridge >* l_pDecoder
					= new OpenViBEToolkit::TStreamedMatrixDecoder< CDLLBridge >;
				OpenViBEToolkit::TStreamedMatrixEncoder< CDLLBridge >* l_pEncoder
					= new OpenViBEToolkit::TStreamedMatrixEncoder< CDLLBridge >;

				l_pDecoder->initialize(*this, 0);
				l_pEncoder->initialize(*this, 0);

				m_pDecoder = l_pDecoder;
				m_pEncoder = l_pEncoder;
			} 
			else if(m_oInputType == OV_TypeId_Signal)
			{
				OpenViBEToolkit::TSignalDecoder< CDLLBridge >* l_pDecoder
					= new OpenViBEToolkit::TSignalDecoder < CDLLBridge >;
				OpenViBEToolkit::TSignalEncoder< CDLLBridge >* l_pEncoder
					= new OpenViBEToolkit::TSignalEncoder< CDLLBridge >;

				l_pDecoder->initialize(*this, 0);
				l_pEncoder->initialize(*this, 0);

				m_pDecoder = l_pDecoder;
				m_pEncoder = l_pEncoder;
			}
			else
			{
				this->getLogManager() << LogLevel_Error << "Unknown input type " << m_oInputType << ". This should never happen.\n";
				return false;
			}
		
			this->getLogManager() << LogLevel_Trace << "DLL box_init() : Calling\n";

			// Do some initialization in DLL
			int32 l_i32ParamsLength = m_sParameters.length();
			int32 l_i32ErrorCode = 0;
			m_pInitialize(&l_i32ParamsLength, m_sParameters.toASCIIString(), &l_i32ErrorCode);
			if(l_i32ErrorCode) 
			{
				this->getLogManager() << LogLevel_Error << "DLL box_init() : Returned error code " << l_i32ErrorCode << "\n";
				return false;
			}
			else
			{
				this->getLogManager() << LogLevel_Trace << "DLL box_init() : Return ok\n";
			}
			return true;
		}

		// This function is called once when user presses Stop in Designer
		OpenViBE::boolean CDLLBridge::uninitialize()
		{
			this->getLogManager() << LogLevel_Debug << "Uninitializing\n";

			if(m_pUninitialize)
			{
				this->getLogManager() << LogLevel_Trace << "DLL box_uninit() : Calling\n";

				// Do some uninitialization in DLL
				int32 l_i32ErrorCode = 0;			
				m_pUninitialize(&l_i32ErrorCode);

				if(l_i32ErrorCode) 
				{
					this->getLogManager() << LogLevel_Error << "DLL box_uninit() : Returned error code " << l_i32ErrorCode << "\n";
					return false;
				}
				else
				{
					this->getLogManager() << LogLevel_Trace << "DLL box_uninit() : Return ok\n";
				}
			}

			if(m_pEncoder)
			{
				m_pEncoder->uninitialize();
				delete m_pEncoder;
				m_pEncoder = NULL;
			}
			if(m_pDecoder)
			{
				m_pDecoder->uninitialize();
				delete m_pDecoder;
				m_pDecoder = NULL;
			}

			if(m_pLibrary) 
			{
#if defined(TARGET_OS_Windows)
				FreeLibrary(m_pLibrary);
#elif defined(TARGET_OS_Linux)
				dlclose(m_pLibrary);
#endif
				m_pLibrary = NULL;
			}
						
			return true;
		}

		OpenViBE::boolean CDLLBridge::processInput(uint32 ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

			return true;
		}

		// This function is called for every signal chunk (matrix of N samples [channels x samples]).
		OpenViBE::boolean CDLLBridge::process()
		{
			this->getLogManager() << LogLevel_Debug << "Process chunk\n";

		    IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
		
			for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
			{
				// m_pDecoder/m_pEncoder should point to StreamedMatrix*coder or Signal*Coder by construction,
				// the latter appears to be static castable to the former, in practice. 
				// n.b. dynamic cast does not work here runtime (this does, for the moment). I do not hazard 
				// an explanation here. Anybody wanting to find out the reasons be prepared to digest the 
				// codec template inheritance relationships in toolkit for a while.
				OpenViBEToolkit::TStreamedMatrixDecoder< CDLLBridge >* l_pDecoder 
					= static_cast< OpenViBEToolkit::TStreamedMatrixDecoder< CDLLBridge >* >(m_pDecoder);
				OpenViBEToolkit::TStreamedMatrixEncoder< CDLLBridge >* l_pEncoder 
					= static_cast< OpenViBEToolkit::TStreamedMatrixEncoder< CDLLBridge >* >(m_pEncoder);

				l_pDecoder->decode(i);
			
				if(l_pDecoder->isHeaderReceived())
				{ 
					if(l_pDecoder->getOutputMatrix()->getDimensionCount()!=2) 
					{
						this->getLogManager() << LogLevel_Error << "Only 2 dimensional matrices supported\n";
						return false;
					}

					int32 l_i32SamplingRateIn = 0;

					if(m_oInputType == OV_TypeId_Signal)
					{
						OpenViBEToolkit::TSignalDecoder< CDLLBridge >* l_pSignalDecoder 
							= static_cast< OpenViBEToolkit::TSignalDecoder< CDLLBridge >* >(m_pDecoder);

						l_i32SamplingRateIn = static_cast<int32>(l_pSignalDecoder->getOutputSamplingRate());
					}

					int32 l_i32nRowsIn = l_pDecoder->getOutputMatrix()->getDimensionSize(0);
					int32 l_i32nColsIn = l_pDecoder->getOutputMatrix()->getDimensionSize(1);

					this->getLogManager() << LogLevel_Trace << "DLL box_process_header() : Calling\n";

					int32 l_i32ErrorCode = 0;
					int32 l_i32nRowsOut=0,l_i32nColsOut=0;
					int32 l_i32SamplingRateOut = 0;
					m_pProcessHeader(&l_i32nRowsIn, &l_i32nColsIn, &l_i32SamplingRateIn, &l_i32nRowsOut, &l_i32nColsOut, &l_i32SamplingRateOut, &l_i32ErrorCode);
					if(l_i32ErrorCode) 
					{
						this->getLogManager() << LogLevel_Error << "DLL box_process_header() : Returned error code " << l_i32ErrorCode << "\n";
						return false;
					}
					else
					{
						this->getLogManager() << LogLevel_Trace << "DLL box_process_header() : Return ok\n";
						this->getLogManager() << LogLevel_Trace << "The function declared" 
							<< " rowsOut=" << l_i32nRowsOut
							<< " colsOut=" << l_i32nColsOut
							<< " sRateOut=" << l_i32SamplingRateOut
							<< "\n";
					}

					l_pEncoder->getInputMatrix()->setDimensionCount(2);
					l_pEncoder->getInputMatrix()->setDimensionSize(0, l_i32nRowsOut);
					l_pEncoder->getInputMatrix()->setDimensionSize(1, l_i32nColsOut);

					if(m_oInputType == OV_TypeId_Signal)
					{
						OpenViBEToolkit::TSignalEncoder< CDLLBridge >* l_pSignalEncoder 
							= static_cast< OpenViBEToolkit::TSignalEncoder< CDLLBridge >* >(m_pEncoder);

						l_pSignalEncoder->getInputSamplingRate() = l_i32SamplingRateOut;
					}

					l_pEncoder->encodeHeader();

					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}

				if(l_pDecoder->isBufferReceived())
				{
					float64* l_pInput = l_pDecoder->getOutputMatrix()->getBuffer();
					float64* l_pOutput = l_pEncoder->getInputMatrix()->getBuffer();

					this->getLogManager() << LogLevel_Trace << "DLL box_process() : Calling\n";

					// Process the sample chunk in DLL
					int32 l_i32ErrorCode = 0;
					m_pProcess(l_pInput, l_pOutput, &l_i32ErrorCode);
					if(l_i32ErrorCode) 
					{
						this->getLogManager() << LogLevel_Error << "DLL box_process() : Returned error code " << l_i32ErrorCode << "\n";
						return false;
					}
					else
					{
						this->getLogManager() << LogLevel_Trace << "DLL box_process() : Return ok\n";
					}

					l_pEncoder->encodeBuffer();

					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				
				}
				if(l_pDecoder->isEndReceived())
				{
					l_pEncoder->encodeEnd();
					
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}
			}
			
			return true;
		}

	};
};
