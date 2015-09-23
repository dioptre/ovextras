#include "ovpCBoxAlgorithmDLLBridge.h"

#include <system/ovCMemory.h>

#include <iostream>
#include <cstdio>

#if defined(TARGET_OS_Windows)
#include <windows.h>
#endif

using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::DLLBridge;

using namespace OpenViBEToolkit;

using namespace std;

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
			m_pProcess = NULL;
			m_pUninitialize = NULL;

			m_sDLLFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			m_sParameters =  FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			if(m_sDLLFile == CString(""))
			{
				this->getLogManager() << LogLevel_Error << "Need a DLL file\n";
				return false;
			}

			std::string tmp = m_sDLLFile.toASCIIString();
			std::replace( tmp.begin(), tmp.end(), '/', '\\');
			m_sDLLFile = tmp.c_str();

			m_pLibrary = LoadLibrary(m_sDLLFile.toASCIIString());
			if(!m_pLibrary)
			{
				this->getLogManager() << LogLevel_Error << "Failed to load " << m_sDLLFile << ", error code " << (int64) GetLastError() << "\n";
				return false;
			}

			m_pInitialize = (INITFUNC)GetProcAddress(m_pLibrary, "box_init");
			if(!m_pInitialize) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_init() in the DLL\n";
				return false;
			}

			m_pProcess = (PROCESSFUNC)GetProcAddress(m_pLibrary, "box_process");
			if(!m_pProcess) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_process() in the DLL\n";
				return false;
			}

			m_pUninitialize = (UNINITFUNC)GetProcAddress(m_pLibrary, "box_uninit");
			if(!m_pUninitialize) 
			{
				this->getLogManager() << LogLevel_Error << "Unable to find box_uninit() in the DLL\n";
				return false;
			}

			IBox& l_rStaticBoxContext=this->getStaticBoxContext();
			l_rStaticBoxContext.getInputType(0, m_oInputType);
			if(m_oInputType == OV_TypeId_StreamedMatrix) {
				OpenViBEToolkit::TStreamedMatrixDecoder < OpenViBEPlugins::DLLBridge::CDLLBridge >* l_pDecoder
					= new OpenViBEToolkit::TStreamedMatrixDecoder < OpenViBEPlugins::DLLBridge::CDLLBridge >;
				OpenViBEToolkit::TStreamedMatrixEncoder < OpenViBEPlugins::DLLBridge::CDLLBridge >* l_pEncoder
					= new OpenViBEToolkit::TStreamedMatrixEncoder < OpenViBEPlugins::DLLBridge::CDLLBridge >;

				l_pDecoder->initialize(*this, 0);
				l_pEncoder->initialize(*this, 0);

				l_pEncoder->getInputMatrix().setReferenceTarget(l_pDecoder->getOutputMatrix());

				m_pDecoder = l_pDecoder;
				m_pEncoder = l_pEncoder;
			} else {
				OpenViBEToolkit::TSignalDecoder < OpenViBEPlugins::DLLBridge::CDLLBridge >* l_pDecoder
					= new OpenViBEToolkit::TSignalDecoder < OpenViBEPlugins::DLLBridge::CDLLBridge >;
				OpenViBEToolkit::TSignalEncoder < OpenViBEPlugins::DLLBridge::CDLLBridge >* l_pEncoder
					= new OpenViBEToolkit::TSignalEncoder < OpenViBEPlugins::DLLBridge::CDLLBridge >;

				l_pDecoder->initialize(*this, 0);
				l_pEncoder->initialize(*this, 0);

				l_pEncoder->getInputMatrix().setReferenceTarget(l_pDecoder->getOutputMatrix());
				l_pEncoder->getInputSamplingRate().setReferenceTarget(l_pDecoder->getOutputSamplingRate());

				m_pDecoder = l_pDecoder;
				m_pEncoder = l_pEncoder;

			}
		
			// Do some initialization in DLL
			int32 l_i32ParamsLength = m_sParameters.length();
			int32 l_i32ErrorCode = 0;
			m_pInitialize(&l_i32ParamsLength, m_sParameters.toASCIIString(), &l_i32ErrorCode);
			if(l_i32ErrorCode) 
			{
				this->getLogManager() << LogLevel_Error << "DLL box_init() returned error code " << l_i32ErrorCode << "\n";
				return false;
			}
			return true;
		}

		// This function is called once when user presses Stop in Designer
		OpenViBE::boolean CDLLBridge::uninitialize()
		{
			this->getLogManager() << LogLevel_Debug << "Uninitializing\n";

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

			if(m_pUninitialize)
			{
				// Do some uninitialization in DLL
				int32 l_i32ErrorCode = 0;			
				m_pUninitialize(&l_i32ErrorCode);

				if(l_i32ErrorCode) 
				{
					this->getLogManager() << LogLevel_Error << "DLL box_uninit() returned error code " << l_i32ErrorCode << "\n";
					return false;
				}
			}

			if(m_pLibrary) 
			{
				FreeLibrary(m_pLibrary);
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
				// Ok, casting to base class
				OpenViBEToolkit::TStreamedMatrixDecoder < OpenViBEPlugins::DLLBridge::CDLLBridge >* l_pDecoder 
					= (OpenViBEToolkit::TStreamedMatrixDecoder < OpenViBEPlugins::DLLBridge::CDLLBridge >*)m_pDecoder;
				OpenViBEToolkit::TStreamedMatrixEncoder < OpenViBEPlugins::DLLBridge::CDLLBridge >* l_pEncoder 
					= (OpenViBEToolkit::TStreamedMatrixEncoder < OpenViBEPlugins::DLLBridge::CDLLBridge >*)m_pEncoder;

				l_pDecoder->decode(i);
			
				if(l_pDecoder->isHeaderReceived())
				{ 
					if(l_pDecoder->getOutputMatrix()->getDimensionCount()!=2) 
					{
						this->getLogManager() << LogLevel_Error << "Only 2 dimensional matrices supported\n";
						return false;
					}
					l_pEncoder->encodeHeader();

					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}
				if(l_pDecoder->isBufferReceived())
				{
					IMatrix* l_pMatrix = l_pDecoder->getOutputMatrix();
					int32 nRows = l_pMatrix->getDimensionSize(0);
					int32 nCols = l_pMatrix->getDimensionSize(1);
					
					// Process the sample chunk in DLL
					int32 l_i32ErrorCode = 0;
					m_pProcess(l_pMatrix->getBuffer(), &nRows, &nCols, &l_i32ErrorCode);
					if(l_i32ErrorCode) 
					{
						this->getLogManager() << LogLevel_Error << "DLL box_process() returned error code " << l_i32ErrorCode << "\n";
						return false;
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
