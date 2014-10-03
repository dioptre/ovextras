
#if defined(TARGET_HAS_ThirdPartyFFTW3) // fftw3 required by wavelet2s

#include "ovpCBoxAlgorithmDiscreteWaveletTransform.h"

#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <map>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>

#include "../../../contrib/packages/wavelet2d/wavelet2s.h"


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

boolean CBoxAlgorithmDiscreteWaveletTransform::initialize(void)
{

    IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// Signal stream decoder
	m_oAlgo0_SignalDecoder.initialize(*this);

    //Signal stream encoder
    m_AlgoInfo_SignalEncoder.initialize(*this);

    m_CStringWaveletType=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_CStringDecompositionLevel=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_oAlgoX_SignalEncoder = new OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmDiscreteWaveletTransform > [l_rStaticBoxContext.getOutputCount()-1];

    //OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmDiscreteWaveletTransform > m_oAlgoX_SignalEncoder[l_rStaticBoxContext.getOutputCount()];

    for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
    {
        m_oAlgoX_SignalEncoder[o].initialize(*this);
    }


	return true;
}


boolean CBoxAlgorithmDiscreteWaveletTransform::uninitialize(void)
{
    IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_oAlgo0_SignalDecoder.uninitialize();
    m_AlgoInfo_SignalEncoder.uninitialize();

    for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
    {
        m_oAlgoX_SignalEncoder[o].uninitialize();
    }

	if(m_oAlgoX_SignalEncoder) {
		delete[] m_oAlgoX_SignalEncoder;
		m_oAlgoX_SignalEncoder = NULL;
	}
	
	return true;
}


boolean CBoxAlgorithmDiscreteWaveletTransform::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}


boolean CBoxAlgorithmDiscreteWaveletTransform::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	


    std::vector<std::vector<double> > sig;
    int J = std::atoi(m_CStringDecompositionLevel);
    std::string nm (m_CStringWaveletType.toASCIIString());
    std::vector<std::vector<double> > dwt_output;
    std::vector<std::vector<double> >flag;
    std::vector<std::vector<int> >length;

    uint32 l_uint32NbChannels0;
    uint32 l_uint32NbSamples0;


    for(uint32 ii=0; ii<l_rDynamicBoxContext.getInputChunkCount(0); ii++)
    {
        //Decode input signal
        m_oAlgo0_SignalDecoder.decode(0,ii);

        l_uint32NbChannels0 = m_oAlgo0_SignalDecoder.getOutputMatrix()->getDimensionSize(0);
        l_uint32NbSamples0 = m_oAlgo0_SignalDecoder.getOutputMatrix()->getDimensionSize(1);

        if (l_uint32NbSamples0<=std::pow(2.0,J+1))
        {
            this->getLogManager() << LogLevel_Warning << "Verify quantity of samples and number of decomposition levels" << "\n";
            this->getLogManager() << LogLevel_Warning << "You can introduce a Time based epoching to have more samples per chunk or reduce the decomposition levels" << "\n";
            return false;
        }


        IMatrix* l_pMatrix_0 = m_oAlgo0_SignalDecoder.getOutputMatrix();

        float64* l_pBuffer0 = l_pMatrix_0->getBuffer();

            //sig will be resized to the number of channels and the total number of samples (Channels x Samples)
            sig.resize(l_uint32NbChannels0);
            for(uint32 i = 0; i < l_uint32NbChannels0; i++)
            {
                sig[i].resize(l_uint32NbSamples0);
            }

        //sig will store the samples of the different channels
        for(uint32 i=0; i<l_uint32NbChannels0; i++)    //Number of EEG channels
        {
            for(uint32 j=0; j<l_uint32NbSamples0; j++)    //Number of Samples per Chunk
            {
                sig[i][j]=(l_pBuffer0[j+i*l_uint32NbSamples0]);
            }
        }


            //dwt_output is the vector containing the decomposition levels
            dwt_output.resize(l_uint32NbChannels0);

            //flag is an auxiliar vector (see wavelet2d library)
            flag.resize(l_uint32NbChannels0);

            //length contains the legth of each decomposition level
            length.resize(l_uint32NbChannels0);

            //Calcul of wavelets coefficients for each channel
                  for(uint32 i=0; i<l_uint32NbChannels0; i++)
                  {
                  dwt(sig[i],J,nm,dwt_output[i],flag[i],length[i]);
                  }



                  m_AlgoInfo_SignalEncoder.getInputSamplingRate().setReferenceTarget(m_oAlgo0_SignalDecoder.getOutputSamplingRate());


                  for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
                  {
                  //m_oAlgoX_SignalEncoder[o].getInputSamplingRate().setReferenceTarget(m_oAlgo0_SignalDecoder.getOutputSamplingRate());
					const float64 l_f64SamplingRate = static_cast<float64>(m_oAlgo0_SignalDecoder.getOutputSamplingRate()) / std::pow(2.0,static_cast<int32>(o));
					m_oAlgoX_SignalEncoder[o].getInputSamplingRate() = static_cast<uint64>(std::floor(l_f64SamplingRate));
                  }

                  uint32 l_infolength = (length[0].size()+flag[0].size()+2);
                  m_AlgoInfo_SignalEncoder.getInputMatrix()->setDimensionCount(2);
                  m_AlgoInfo_SignalEncoder.getInputMatrix()->setDimensionSize(0,l_uint32NbChannels0);
                  m_AlgoInfo_SignalEncoder.getInputMatrix()->setDimensionSize(1,l_infolength);



          for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
          {
              m_oAlgoX_SignalEncoder[o].getInputMatrix()->setDimensionCount(2);
              m_oAlgoX_SignalEncoder[o].getInputMatrix()->setDimensionSize(0,l_uint32NbChannels0);
              m_oAlgoX_SignalEncoder[o].getInputMatrix()->setDimensionSize(1,length[0].at(o));

          }


          for (uint32 d_i=0; d_i<l_uint32NbChannels0; d_i++)
          {
              for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
              {
              m_oAlgoX_SignalEncoder[o].getInputMatrix()->setDimensionLabel(0,d_i,m_oAlgo0_SignalDecoder.getOutputMatrix()->getDimensionLabel(0,d_i));
              }
          }


          //Transmission of some information (flag and legth) to the inverse dwt box
          for(uint32 i=0; i<l_uint32NbChannels0; i++)
          {
          uint32 f=0;
          m_AlgoInfo_SignalEncoder.getInputMatrix()->getBuffer()[f+i*l_infolength]=length[i].size();
          for (uint32 l=0;l<length[i].size();l++)
          {
          m_AlgoInfo_SignalEncoder.getInputMatrix()->getBuffer()[l+1+i*l_infolength]=length[i].at(l);
          f=l;
          }
          m_AlgoInfo_SignalEncoder.getInputMatrix()->getBuffer()[f+2+i*l_infolength]=flag[i].size();
          for (uint32 l=0;l<flag[i].size();l++)
          {
          m_AlgoInfo_SignalEncoder.getInputMatrix()->getBuffer()[f+3+l+i*l_infolength]=flag[i].at(l);
          }
          }




          //Store dwt coefficients to each decomposition level
          uint32 l_uint32Vector_Position=0;

          for(uint32 i=0; i<l_uint32NbChannels0; i++)
          {
          for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
          {  
                for (uint32 l=0; l<(uint32)length[i].at(o); l++)
                {
                    m_oAlgoX_SignalEncoder[o].getInputMatrix()->getBuffer()[l+i*length[i].at(o)] = dwt_output[i][l+l_uint32Vector_Position];
                }
                l_uint32Vector_Position=l_uint32Vector_Position+length[i].at(o);

           }
          l_uint32Vector_Position=0;
          }


          //Encode buffer
              if(m_oAlgo0_SignalDecoder.isHeaderReceived())
              {
                  m_AlgoInfo_SignalEncoder.encodeHeader(0);
                  l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, ii), l_rDynamicBoxContext.getInputChunkEndTime(0, ii));

                  for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
                  {
                  m_oAlgoX_SignalEncoder[o].encodeHeader(o+1);
                  l_rDynamicBoxContext.markOutputAsReadyToSend(o+1, l_rDynamicBoxContext.getInputChunkStartTime(0, ii), l_rDynamicBoxContext.getInputChunkEndTime(0, ii));
                  }
              }

              if(m_oAlgo0_SignalDecoder.isBufferReceived())
              {
                  m_AlgoInfo_SignalEncoder.encodeBuffer(0);
                  l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, ii), l_rDynamicBoxContext.getInputChunkEndTime(0, ii));

                  for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
                  {
                  m_oAlgoX_SignalEncoder[o].encodeBuffer(o+1);
                  l_rDynamicBoxContext.markOutputAsReadyToSend(o+1, l_rDynamicBoxContext.getInputChunkStartTime(0, ii), l_rDynamicBoxContext.getInputChunkEndTime(0, ii));
                  }
              }
              if(m_oAlgo0_SignalDecoder.isEndReceived())
              {
                  m_AlgoInfo_SignalEncoder.encodeEnd(0);
                  l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, ii), l_rDynamicBoxContext.getInputChunkEndTime(0, ii));

                  for (uint32 o = 0; o < l_rStaticBoxContext.getOutputCount()-1; o++)
                  {
                  m_oAlgoX_SignalEncoder[o].encodeEnd(o+1);
                  l_rDynamicBoxContext.markOutputAsReadyToSend(o+1, l_rDynamicBoxContext.getInputChunkStartTime(0, ii), l_rDynamicBoxContext.getInputChunkEndTime(0, ii));
                  }
              }

    }

	return true;
}



#endif