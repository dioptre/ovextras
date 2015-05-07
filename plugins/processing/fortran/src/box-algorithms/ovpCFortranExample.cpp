#include "ovpCFortranExample.h"

#include <system/ovCMemory.h>

#include <iostream>
#include <cstdio>

using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Fortran;

using namespace OpenViBEToolkit;

using namespace std;

extern "C" void fortran_init(int32 *errorCode);
extern "C" void fortran_process(OpenViBE::float64* mat, int32* m, int32* n, int32 *errorCode);
extern "C" void fortran_uninit(int32 *errorCode);

void dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const CMatrix& mat, const CString &desc)
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
	namespace Fortran
	{


		OpenViBE::boolean CFortranExample::initialize()
		{			
			this->getLogManager() << LogLevel_Info << "Initializing\n";

			m_oDecoder.initialize(*this, 0);
			m_oEncoder.initialize(*this, 0);

			m_oEncoder.getInputMatrix().setReferenceTarget(m_oDecoder.getOutputMatrix());
		
			int32 l_i32ErrorCode = 0;
			fortran_init(&l_i32ErrorCode);
			if(l_i32ErrorCode) 
			{
				this->getLogManager() << LogLevel_Error << "Fortran init() returned error code " << l_i32ErrorCode << "\n";
				return false;
			}
			return true;
		}

		OpenViBE::boolean CFortranExample::uninitialize()
		{
			this->getLogManager() << LogLevel_Info << "Uninitializing\n";

			int32 l_i32ErrorCode = 0;			
			fortran_uninit(&l_i32ErrorCode);

			m_oEncoder.uninitialize();
			m_oDecoder.uninitialize();
			
			if(l_i32ErrorCode) 
			{
				this->getLogManager() << LogLevel_Error << "Fortran uninit() returned error code " << l_i32ErrorCode << "\n";
				return false;
			}
						
			return true;
		}

		boolean CFortranExample::processInput(uint32 ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

			return true;
		}

		OpenViBE::boolean CFortranExample::process()
		{

		    IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
		
			for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
			{
				m_oDecoder.decode(i);
			
				if(m_oDecoder.isHeaderReceived())
				{ 
					if(m_oDecoder.getOutputMatrix()->getDimensionCount()!=2) 
					{
						this->getLogManager() << LogLevel_Error << "Only 2 dimensional matrices supported\n";
						return false;
					}
					m_oEncoder.encodeHeader();

					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}
				if(m_oDecoder.isBufferReceived())
				{
					IMatrix* l_pMatrix = m_oDecoder.getOutputMatrix();
					int32 nRows = l_pMatrix->getDimensionSize(0);
					int32 nCols = l_pMatrix->getDimensionSize(1);
					
					int32 l_i32ErrorCode = 0;
					fortran_process(l_pMatrix->getBuffer(), &nRows, &nCols, &l_i32ErrorCode);
					if(l_i32ErrorCode) 
					{
						this->getLogManager() << LogLevel_Error << "Fortran process() returned error code " << l_i32ErrorCode << "\n";
						return false;
					}
					
					m_oEncoder.encodeBuffer();

					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				
				}
				if(m_oDecoder.isEndReceived())
				{
					m_oEncoder.encodeEnd();
					
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}
			}
			// dumpMatrix(this->getLogManager(), poo, CString(""));
			
			return true;
		}

	};
};
