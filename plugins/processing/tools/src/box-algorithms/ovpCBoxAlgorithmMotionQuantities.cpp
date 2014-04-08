#include "ovpCBoxAlgorithmMotionQuantities.h"

#include <openvibe/ovITimeArithmetics.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Tools;

using namespace std;


#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif


boolean CBoxAlgorithmMotionQuantities::initialize(void)
{
	// Feature vector stream encoder
	m_oAngle_FeatureVectorEncoder.initialize(*this);
	m_oHistoAngle_FeatureVectorEncoder.initialize(*this);
	m_oLength_FeatureVectorEncoder.initialize(*this);
	m_oVelocity_SignalEncoder.initialize(*this);
	m_oAcceleration_SignalEncoder.initialize(*this);

	// Signal stream decoder
	m_oInputSignalDecoder.initialize(*this);


        // Set reference target for feature vector
        m_oAngle_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pAnglesMatrix);
        m_oHistoAngle_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pHistoMatrix);
        m_oLength_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pLengthMatrix);


        // Set reference target for signals
        m_oVelocity_SignalEncoder.getInputMatrix().setReferenceTarget(m_pVelocityMatrix);
        m_oVelocity_SignalEncoder.getInputSamplingRate().setReferenceTarget(m_oInputSignalDecoder.getOutputSamplingRate());
        m_oAcceleration_SignalEncoder.getInputMatrix().setReferenceTarget(m_pAccelerationMatrix);
        m_oAcceleration_SignalEncoder.getInputSamplingRate().setReferenceTarget(m_oInputSignalDecoder.getOutputSamplingRate());

        m_pAnglesMatrix = new CMatrix();
        m_pHistoMatrix = new CMatrix();
        m_pLengthMatrix = new CMatrix();
        m_pVelocityMatrix = new CMatrix();
        m_pAccelerationMatrix = new CMatrix();

        m_AngleBin.clear();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmMotionQuantities::uninitialize(void)
{
	m_oAngle_FeatureVectorEncoder.uninitialize();
	m_oHistoAngle_FeatureVectorEncoder.uninitialize();
	m_oLength_FeatureVectorEncoder.uninitialize();
	m_oVelocity_SignalEncoder.uninitialize();
	m_oAcceleration_SignalEncoder.uninitialize();

	m_oInputSignalDecoder.uninitialize();

        delete m_pAnglesMatrix;
        m_pAnglesMatrix=NULL;

        delete m_pHistoMatrix;
        m_pHistoMatrix=NULL;

        delete m_pLengthMatrix;
        m_pLengthMatrix=NULL;

        delete m_pVelocityMatrix;
        m_pVelocityMatrix=NULL;

        delete m_pAccelerationMatrix;
        m_pAccelerationMatrix=NULL;

	return true;
}


boolean CBoxAlgorithmMotionQuantities::processInput(uint32 ui32InputIndex)
{
	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmMotionQuantities::process(void)
{
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

        IMatrix* l_pMousePositionMatrix = m_oInputSignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
        uint64 l_ui64SamplingFrequency = m_oInputSignalDecoder.getOutputSamplingRate(); // the sampling rate of the signal

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oInputSignalDecoder.decode(0,i);

		uint32 l_ui32SamplesPerChannel =  l_pMousePositionMatrix->getDimensionSize(1);

		m_AngleBin.resize(5);
		fill(m_AngleBin.begin(), m_AngleBin.end(), 0);

		if(m_oInputSignalDecoder.isHeaderReceived())
		{

                        m_pAnglesMatrix->setDimensionCount(2);
                        m_pAnglesMatrix->setDimensionSize(0,1);
                        m_pAnglesMatrix->setDimensionSize(1, l_ui32SamplesPerChannel-2);
                        m_pAnglesMatrix->setDimensionLabel(0,0, "Angle");

                        m_pHistoMatrix->setDimensionCount(2);
                        m_pHistoMatrix->setDimensionSize(0,1);
                        m_pHistoMatrix->setDimensionSize(1,5);
                        m_pHistoMatrix->setDimensionLabel(0,0,"Angle Bins");

                        m_pLengthMatrix->setDimensionCount(2);
                        m_pLengthMatrix->setDimensionSize(0,1);
                        m_pLengthMatrix->setDimensionSize(1, l_ui32SamplesPerChannel-2);
                        m_pLengthMatrix->setDimensionLabel(0,0, "Length");

                        m_pVelocityMatrix->setDimensionCount(2);
                        m_pVelocityMatrix->setDimensionSize(0,2);
                        m_pVelocityMatrix->setDimensionSize(1, l_ui32SamplesPerChannel-1);
                        m_pVelocityMatrix->setDimensionLabel(0,0, "Velocity x");
                        m_pVelocityMatrix->setDimensionLabel(0,1, "Velocity y");

                        m_pAccelerationMatrix->setDimensionCount(2);
                        m_pAccelerationMatrix->setDimensionSize(0,2);
                        m_pAccelerationMatrix->setDimensionSize(1,l_ui32SamplesPerChannel-2);
                        m_pAccelerationMatrix->setDimensionLabel(0,0,"Acceleration x");
                        m_pAccelerationMatrix->setDimensionLabel(0,1,"Acceleration y");


			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oAngle_FeatureVectorEncoder.encodeHeader(0);
			m_oHistoAngle_FeatureVectorEncoder.encodeHeader(1);
			m_oLength_FeatureVectorEncoder.encodeHeader(2);
			m_oVelocity_SignalEncoder.encodeHeader(3);
			m_oAcceleration_SignalEncoder.encodeHeader(4);


			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(3, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(4, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		if(m_oInputSignalDecoder.isBufferReceived())
		{

		    // Angle computation
		    for(uint32 k = 1; k<l_ui32SamplesPerChannel-1; k++)
		      {

			// Coordinates of the 2 vectors starting from current point
			float64 l_f64Vec1_x;
			float64 l_f64Vec1_y;
			float64 l_f64Vec2_x;
			float64 l_f64Vec2_y;

                        // Norm of the 2 vectors
                        float64 l_f64NormVec1;
                        float64 l_f64NormVec2;

                        // Cosine of angle between those 2 vectors
                        float64 l_f64CosTheta;

                        float64 l_f64AngleRad; // Angle in radians
                        float64 l_f64AngleDeg; // Angle in degrees

                        /*
                         Computation of coordinates of the 2 vectors starting from current point:

                         Vector 1 = Previous_point - Current_point
                         Vector 2 = Next_point - Current_point
                        */

                        l_f64Vec1_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(k-1)]-l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+k];
                        l_f64Vec1_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(k-1)]-l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+k];

                        l_f64Vec2_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(k+1)]-l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+k];
                        l_f64Vec2_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(k+1)]-l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+k];

                        // Norm of vectors computation
                        l_f64NormVec1 = sqrt( pow(l_f64Vec1_x,2) + pow(l_f64Vec1_y,2) );
                        l_f64NormVec2 = sqrt( pow(l_f64Vec2_x,2) + pow(l_f64Vec2_y,2) );


                        // Compute angle between the 2 vectors
                        if(l_f64NormVec1 == 0 || l_f64NormVec2 == 0)
                          {
                            l_f64CosTheta = -1; // Handle division by zero case
                          }
                        else
                          {
                            l_f64CosTheta = (l_f64Vec1_x*l_f64Vec2_x + l_f64Vec1_y*l_f64Vec2_y) / (l_f64NormVec1*l_f64NormVec2);
                          }

                        l_f64AngleRad = acos(l_f64CosTheta);
                        l_f64AngleDeg = l_f64AngleRad*180/M_PI;

                        m_pAnglesMatrix->getBuffer()[k-1] = l_f64AngleDeg;
                        m_pLengthMatrix->getBuffer()[k-1]=l_f64NormVec2;


                        // Construct histogram (count angles that fall in interval), interval boundaries in degrees: [0-30[ ; [30-75[ ; [75-105[ ; [105-150[ ; [150-180]
                        if(l_f64AngleDeg>=0 && l_f64AngleDeg<30)
                          {
                            m_AngleBin[1]++;
                          }
                        else if(l_f64AngleDeg>=30 && l_f64AngleDeg<75)
                          {
                            m_AngleBin[2]++;
                          }
                        else if (l_f64AngleDeg>=75 && l_f64AngleDeg<105)
                          {
                            m_AngleBin[3]++;
                          }
                        else if (l_f64AngleDeg>=105 && l_f64AngleDeg<150)
                          {
                            m_AngleBin[4]++;
                          }
                        else if (l_f64AngleDeg>=150 && l_f64AngleDeg<=180)
                          {
                           m_AngleBin[5]++;
                          }
                        else
                          {
                            cout<<"Warning, error measure in angle : "<<l_f64AngleDeg<<"   "<<l_f64AngleRad<<endl;
                          }

                       }

                    for(uint32 j=0;j<m_pHistoMatrix->getDimensionSize(1);j++)
                      {
                        if(m_pHistoMatrix->getDimensionSize(1)==m_AngleBin.size())
                          {
                             m_pHistoMatrix->getBuffer()[j] = m_AngleBin[j];
                          }
                        else
                          {
                            cout<<"Warning, error not enough bin : "<<endl;
                          }

                      }

                    //Velocity and acceleration computation
                    for(uint32 k = 1; k<l_ui32SamplesPerChannel; k++)
                    {
                       float64 l_f64Previous_x;
                       float64 l_f64Previous_y;

                       float64 l_f64Current_x;
                       float64 l_f64Current_y;

                       l_f64Previous_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(k-1)];
                       l_f64Previous_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(k-1)];

                       l_f64Current_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+k];
                       l_f64Current_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+k];

                       m_pVelocityMatrix->getBuffer()[0*(l_ui32SamplesPerChannel-1)+ (k-1)] = (l_f64Current_x-l_f64Previous_x)*l_ui64SamplingFrequency;
                       m_pVelocityMatrix->getBuffer()[1*(l_ui32SamplesPerChannel-1)+ (k-1)] = (l_f64Current_y-l_f64Previous_y)*l_ui64SamplingFrequency;

                     }

                     for(uint32 k = 1; k<l_ui32SamplesPerChannel-1; k++)
                       {
                         m_pAccelerationMatrix->getBuffer()[0*(l_ui32SamplesPerChannel-2)+(k-1)] = (m_pVelocityMatrix->getBuffer()[k]-m_pVelocityMatrix->getBuffer()[k-1])*l_ui64SamplingFrequency;
                         m_pAccelerationMatrix->getBuffer()[1*(l_ui32SamplesPerChannel-2)+(k-1)] = (m_pVelocityMatrix->getBuffer()[1*(l_ui32SamplesPerChannel-1)+ k]-m_pVelocityMatrix->getBuffer()[1*(l_ui32SamplesPerChannel-1)+ (k-1)])*l_ui64SamplingFrequency;
                       }

		    // Encode the output buffer :
		    m_oAngle_FeatureVectorEncoder.encodeBuffer(0);
		    m_oHistoAngle_FeatureVectorEncoder.encodeBuffer(1);
		    m_oLength_FeatureVectorEncoder.encodeBuffer(2);
		    m_oVelocity_SignalEncoder.encodeBuffer(3);
		    m_oAcceleration_SignalEncoder.encodeBuffer(4);


		    // and send it to the next boxes :
		    l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(3, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(4, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			
		}

		if(m_oInputSignalDecoder.isEndReceived())
		{
		    // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
		    m_oAngle_FeatureVectorEncoder.encodeEnd(0);
		    m_oHistoAngle_FeatureVectorEncoder.encodeEnd(1);
		    m_oLength_FeatureVectorEncoder.encodeEnd(2);
		    m_oVelocity_SignalEncoder.encodeEnd(3);
		    m_oAcceleration_SignalEncoder.encodeEnd(4);

		    l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(3, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(4, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

	}
	return true;
}
