#include "ovpCBoxAlgorithmTwoSampleTTest.h"
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessingStatistics;

using namespace boost::math;

CBoxAlgorithmTwoSampleTTest::CBoxAlgorithmTwoSampleTTest() : m_ui32InputCounter()
{
	std::cout << " Constructor called! ";
}

boolean CBoxAlgorithmTwoSampleTTest::initialize(void)
{
	// Signal stream decoder
	m_oAlgo0_StreamedMatrixDecoder.initialize(*this);
	// Streamed matrix stream encoder
	m_oAlgo1_StreamedMatrixEncoder.initialize(*this);

	return true;
}

boolean CBoxAlgorithmTwoSampleTTest::uninitialize(void)
{
	m_oAlgo0_StreamedMatrixDecoder.uninitialize();
	m_oAlgo1_StreamedMatrixEncoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmTwoSampleTTest::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmTwoSampleTTest::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	//IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	m_bNonEmpty = true;

	//iterate over all chunk on input 0
	for(uint32 j=0; j<2; j++)
	{
		for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(j); i++)
		{
			// decode the chunk i on input 0
			m_oAlgo0_StreamedMatrixDecoder.decode(j,i);
			// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
			if(m_oAlgo0_StreamedMatrixDecoder.isHeaderReceived())
			{
				// Header received. This happens only once when pressing "play". For example with a StreamedMatrix input, you now know the dimension count, sizes, and labels of the matrix
				uint32 l_iSize = m_oAlgo0_StreamedMatrixDecoder.getOutputMatrix()->getBufferElementCount();
				this->getLogManager() << LogLevel_Debug << "Header received for input " << j << ", length of signal (channels time samples): " << l_iSize << "\n";
				m_vMean[j].set_size(l_iSize); m_vMean[j].zeros();
				m_vM[j].set_size(l_iSize); m_vM[j].zeros();
			
				// Pass the header to the next boxes, by encoding a header on the output 0:
				m_oAlgo1_StreamedMatrixEncoder.getInputMatrix()->setDimensionCount(1);
				m_oAlgo1_StreamedMatrixEncoder.getInputMatrix()->setDimensionSize(0,1);
				//m_oAlgo1_StreamedMatrixEncoder.getInputMatrix()->setDimensionSize(1,1);
				//OpenViBEToolkit::Tools::Matrix::copyDescription(*m_oAlgo1_StreamedMatrixEncoder.getInputMatrix(), *m_oAlgo0_SignalDecoder.getOutputMatrix());
				m_oAlgo1_StreamedMatrixEncoder.encodeHeader(0);
				// send the output chunk containing the header. The dates are the same as the input chunk:
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			}
			if(m_oAlgo0_StreamedMatrixDecoder.isBufferReceived())
			{
				// Buffer received. For example the signal values
				// Access to the buffer can be done thanks to :
				IMatrix* l_pMatrix = new CMatrix(); 
				l_pMatrix->setDimensionCount(2);
				l_pMatrix->setDimensionSize(0, m_oAlgo0_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(0));
				l_pMatrix->setDimensionSize(1, m_oAlgo0_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(1));
				OpenViBEToolkit::Tools::Matrix::copy(*l_pMatrix, *m_oAlgo0_StreamedMatrixDecoder.getOutputMatrix()); // the StreamedMatrix of samples.
			
				this->getLogManager() << LogLevel_Debug << "first element of input matrix[" << j << "]=" << *l_pMatrix->getBuffer() << " and last element=" << *(l_pMatrix->getBuffer()+m_oAlgo0_StreamedMatrixDecoder.getOutputMatrix()->getDimensionSize(1)-1) << "\n";

				m_vInputBuffer[j].push_back(l_pMatrix);
		
			}
			if(m_oAlgo0_StreamedMatrixDecoder.isEndReceived())
			{
				// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
				m_oAlgo1_StreamedMatrixEncoder.encodeEnd(0);
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			}
		}

		if ((int)m_vInputBuffer[j].empty())
			m_bNonEmpty=false;

		this->getLogManager() << LogLevel_Debug << "Input " << j << " buffer size " << (int)m_vInputBuffer[j].size() << ", nonempty: " << m_bNonEmpty << "\n";		
	}

	for(uint32 j=0; j<2; j++)
	{
		while (m_bNonEmpty && !m_vInputBuffer[j].empty())
		{
			m_ui32InputCounter[j]++;
			uint32 l_iSize = m_vInputBuffer[j].back()->getBufferElementCount();
			Vec<float64> l_vMatrix(m_vInputBuffer[j].back()->getBuffer(),l_iSize);
			m_vInputBuffer[j].pop_back();
			Vec<float64> delta = l_vMatrix - m_vMean[j];
			m_vMean[j] += delta/(float64)m_ui32InputCounter[j];
			m_vM[j] +=  elem_mult(delta, (l_vMatrix - m_vMean[j]));
			if (m_ui32InputCounter[j]>1)
				m_f64Variance[j] =  m_vM[j]/(float64)(m_ui32InputCounter[j]-1);
		}
	}

	if (m_bNonEmpty && m_ui32InputCounter[0]>1 && m_ui32InputCounter[1]>1)
	{
		Vec<float64> l_Sxy = itpp::sqrt( (((float64)(m_ui32InputCounter[0]-1.0))*m_f64Variance[0] + ((float64)(m_ui32InputCounter[1]-1.0))*m_f64Variance[1])/((float64)(m_ui32InputCounter[0]+m_ui32InputCounter[1]-2)) );


		this->getLogManager() << LogLevel_Debug << "Length m_vMean[0] " << (int)m_vMean[0].length() << ", length m_vMean[1] " << (int)m_vMean[1].length() << ", length l_Sxy " << (int)l_Sxy.length() << ", length m_f64Variance[0] " << (int)m_f64Variance[0].length() << ", length m_f64Variance[1] " << (int)m_f64Variance[1].length() << "\n";
		this->getLogManager() << LogLevel_Debug << "first element of m_vMean[0] " << (double)m_vMean[0][0] << ", first element of m_vMean[1] " << (double)m_vMean[1][0] << ", first element of l_Sxy " << (double)l_Sxy[0] << ", first element of m_f64Variance[0] " << (double)m_f64Variance[0][0] << ", first element of m_f64Variance[1] " << (double)m_f64Variance[1][0] << ", first input count " << m_ui32InputCounter[0] << ", second input count " << m_ui32InputCounter[1] << "\n";

		Vec<float64> l_tStatistic = elem_div(m_vMean[0]-m_vMean[1], l_Sxy*std::sqrt(1.0/(float)m_ui32InputCounter[0]+1.0/(float)m_ui32InputCounter[1]));

		this->getLogManager() << LogLevel_Debug << "first element of statistic " << l_tStatistic[0] << "\n"; 

		Vec<float64> l_pValue;
		l_pValue.set_size(l_tStatistic.length());
		for (int i=0; i<l_tStatistic.length(); i++)
			l_pValue[i] = cdf(complement(students_t(m_ui32InputCounter[0]+m_ui32InputCounter[1]-2), fabs(l_tStatistic[i])));
		this->getLogManager() << LogLevel_Debug << "first element of p-value " << l_pValue[0] << "\n"; 

		IMatrix* l_oOutputMatrix = m_oAlgo1_StreamedMatrixEncoder.getInputMatrix();
		l_oOutputMatrix->setDimensionCount(2);
		l_oOutputMatrix->setDimensionSize(0,1);
		l_oOutputMatrix->setDimensionSize(1,1);
		*l_oOutputMatrix->getBuffer() = itpp::min(l_pValue);

		this->getLogManager() << LogLevel_Debug << "Minimum p-value " << *l_oOutputMatrix->getBuffer() << "\n";
		// Encode the output buffer :
		m_oAlgo1_StreamedMatrixEncoder.encodeBuffer(0);
		// and send it to the next boxes :
		l_rDynamicBoxContext.markOutputAsReadyToSend(0, this->getPlayerContext().getCurrentTime(), this->getPlayerContext().getCurrentTime());
	}

	return true;
}
