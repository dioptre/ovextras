#include "ovpCBoxAlgorithmAngleSpeed.h"

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

struct Sort_by_second
{
   bool operator() (const std::pair<float64,int> & left, const std::pair<float64,int> & right) const
   {
      return left.second < right.second;
   }
};

boolean CBoxAlgorithmAngleSpeed::initialize(void)
{
	// Feature vector stream encoder
	m_oAngle_FeatureVectorEncoder.initialize(*this);
	m_oLength_FeatureVectorEncoder.initialize(*this);
	m_oVector_FeatureVectorEncoder.initialize(*this);
	m_oVelocity_SignalEncoder.initialize(*this);
	m_oAcceleration_SignalEncoder.initialize(*this);
	m_oHistoAngle_FeatureVectorEncoder.initialize(*this);
	// Signal stream decoder
	m_oInputSignalDecoder.initialize(*this);


    m_oAngle_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pAnglesMatrix);
    m_oLength_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pLengthMatrix);
    m_oVector_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pVectorMatrix);
    m_oHistoAngle_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pHistoMatrix);



    m_oVelocity_SignalEncoder.getInputMatrix().setReferenceTarget(m_pVelocityMatrix);
    m_oVelocity_SignalEncoder.getInputSamplingRate().setReferenceTarget(m_oInputSignalDecoder.getOutputSamplingRate());
    m_oAcceleration_SignalEncoder.getInputMatrix().setReferenceTarget(m_pAccelerationMatrix);
    m_oAcceleration_SignalEncoder.getInputSamplingRate().setReferenceTarget(m_oInputSignalDecoder.getOutputSamplingRate());

    m_pAnglesMatrix = new CMatrix();
    m_pLengthMatrix = new CMatrix();
    m_pVectorMatrix = new CMatrix();
    m_pVelocityMatrix = new CMatrix();
    m_pAccelerationMatrix = new CMatrix();
    m_pHistoMatrix = new CMatrix();

    m_AngleBin1.clear();

    m_f64Acc = 0;
    m_f64Vit = 0;

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmAngleSpeed::uninitialize(void)
{
	m_oAngle_FeatureVectorEncoder.uninitialize();
	m_oLength_FeatureVectorEncoder.uninitialize();
	m_oVector_FeatureVectorEncoder.uninitialize();
	m_oVelocity_SignalEncoder.uninitialize();
	m_oAcceleration_SignalEncoder.uninitialize();
	m_oHistoAngle_FeatureVectorEncoder.uninitialize();

	m_oInputSignalDecoder.uninitialize();

    delete m_pAnglesMatrix;
    m_pAnglesMatrix=NULL;

    delete m_pLengthMatrix;
    m_pLengthMatrix=NULL;

    delete m_pVectorMatrix;
    m_pVectorMatrix=NULL;

    delete m_pVelocityMatrix;
    m_pVelocityMatrix=NULL;

    delete m_pAccelerationMatrix;
    m_pAccelerationMatrix=NULL;

    delete m_pHistoMatrix;
    m_pHistoMatrix=NULL;

	return true;
}


boolean CBoxAlgorithmAngleSpeed::processInput(uint32 ui32InputIndex)
{
	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmAngleSpeed::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
//	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

        IMatrix* l_pMousePositionMatrix = m_oInputSignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
        uint64 l_ui64SamplingFrequency = m_oInputSignalDecoder.getOutputSamplingRate(); // the sampling rate of the signal



 //       cout<<"freq = "<<l_ui64SamplingFrequency<<endl;

        float64 l_f64Somme = 0;


	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oInputSignalDecoder.decode(0,i);

		uint32 l_ui32SamplesPerChannel =  l_pMousePositionMatrix->getDimensionSize(1);

		m_AngleBin1.resize(5);
		fill(m_AngleBin1.begin(), m_AngleBin1.end(), 0);
/*		m_AngleBin2 = 0;
		m_AngleBin3 = 0;
		m_AngleBin4 = 0;
		m_AngleBin5 = 0;*/


		if(m_oInputSignalDecoder.isHeaderReceived())
		{

                        m_pAnglesMatrix->setDimensionCount(2);
                        m_pAnglesMatrix->setDimensionSize(0,1);
                        m_pAnglesMatrix->setDimensionSize(1, l_ui32SamplesPerChannel-2);
                        m_pAnglesMatrix->setDimensionLabel(0,0, "Angle");

                        m_pLengthMatrix->setDimensionCount(2);
                        m_pLengthMatrix->setDimensionSize(0,1);
                        m_pLengthMatrix->setDimensionSize(1, l_ui32SamplesPerChannel-2);
                        m_pLengthMatrix->setDimensionLabel(0,0, "Length");

                        m_pVectorMatrix->setDimensionCount(2);
                        m_pVectorMatrix->setDimensionSize(0,2);
                        m_pVectorMatrix->setDimensionSize(1, l_ui32SamplesPerChannel-2);
                        m_pVectorMatrix->setDimensionLabel(0,0, "Vector x");
                        m_pVectorMatrix->setDimensionLabel(0,1, "Vector y");

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

                        m_pHistoMatrix->setDimensionCount(2);
                        m_pHistoMatrix->setDimensionSize(0,1);
                        m_pHistoMatrix->setDimensionSize(1,5);
                        m_pHistoMatrix->setDimensionLabel(0,0,"Angle Bins");


			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oAngle_FeatureVectorEncoder.encodeHeader(0);
			m_oLength_FeatureVectorEncoder.encodeHeader(1);
			m_oVector_FeatureVectorEncoder.encodeHeader(2);
			m_oVelocity_SignalEncoder.encodeHeader(3);
			m_oAcceleration_SignalEncoder.encodeHeader(4);
			m_oHistoAngle_FeatureVectorEncoder.encodeHeader(5);

			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(3, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(4, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(5, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		if(m_oInputSignalDecoder.isBufferReceived())
		{

		    //create vector of pair to keep track of original indexes
		    vector <pair <float64,uint32> > l_oTheta;
//		    vector <pair <float64,uint32> > l_oThetaSorted;

//		    vector <pair <float64,uint32> > angleSeuil;
//		    vector<float64> l_oTheta;
/*		    m_oTheta.reserve(m_pAnglesMatrix->getDimensionSize(1));
		    for(uint32 j=0; j<m_oTheta.size();j++)
		      {
			m_oTheta.push_back(DBL_MAX);
		      }*/

		    //Angle computation
		    for(uint32 k = 1; k<l_ui32SamplesPerChannel-1; k++)
		      {
			float64 l_f64Vec1_x;
			float64 l_f64Vec1_y;
			float64 l_f64Vec2_x;
			float64 l_f64Vec2_y;

                        float64 l_f64NormVec1;
                        float64 l_f64NormVec2;

                        float64 l_f64CosTheta;

                        float64 l_f64AngleRad;
                        float64 l_f64AngleDeg;

                        l_f64Vec1_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(k-1)]-l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+k];
                        l_f64Vec1_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(k-1)]-l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+k];

                        l_f64Vec2_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(k+1)]-l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+k];
                        l_f64Vec2_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(k+1)]-l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+k];

                        l_f64NormVec1 = sqrt( pow(l_f64Vec1_x,2) + pow(l_f64Vec1_y,2) );
                        l_f64NormVec2 = sqrt( pow(l_f64Vec2_x,2) + pow(l_f64Vec2_y,2) );

                        if(l_f64NormVec1 == 0 || l_f64NormVec2 == 0)
                          {
                            l_f64CosTheta = -1;
                          }
                        else
                          {
                            l_f64CosTheta = (l_f64Vec1_x*l_f64Vec2_x + l_f64Vec1_y*l_f64Vec2_y) / (l_f64NormVec1*l_f64NormVec2);
                          }

/*                        for(uint32 j=0; j<m_oTheta.size();j++)
                          {
                            if (acos(l_f64CosTheta)<=m_oTheta[j])
                              {

                              }
                          }*/

                        l_f64AngleRad = acos(l_f64CosTheta);
                        l_f64AngleDeg = l_f64AngleRad*180/M_PI;

                        l_oTheta.push_back(make_pair(l_f64AngleDeg,k-1));
  //                      l_oTheta.push_back(acos(l_f64CosTheta));
  //                      cout<< "Angles = "<<(l_oTheta[k-1].first)<<endl;
                        m_pAnglesMatrix->getBuffer()[k-1] = l_oTheta[k-1].first;


                        m_pVectorMatrix->getBuffer()[0*m_pVectorMatrix->getDimensionSize(1) + (k-1)] = l_f64Vec2_x;
                        m_pVectorMatrix->getBuffer()[1*m_pVectorMatrix->getDimensionSize(1) + (k-1)] = l_f64Vec2_y;

                        m_pLengthMatrix->getBuffer()[k-1]=l_f64NormVec2;

                        if(l_f64AngleDeg>=0 && l_f64AngleDeg<30)
                          {
                            m_AngleBin1[1]++;
                          }
                        else if(l_f64AngleDeg>=30 && l_f64AngleDeg<75)
                          {
                            m_AngleBin1[2]++;
                          }
                        else if (l_f64AngleDeg>=75 && l_f64AngleDeg<105)
                          {
                            m_AngleBin1[3]++;
                          }
                        else if (l_f64AngleDeg>=105 && l_f64AngleDeg<150)
                          {
                            m_AngleBin1[4]++;
                          }
                        else if (l_f64AngleDeg>=150 && l_f64AngleDeg<=180)
                          {
                           m_AngleBin1[5]++;
                          }
                        else
                          {
                            cout<<"Warning, error measure in angle : "<<l_f64AngleDeg<<"   "<<l_f64AngleRad<<endl;
                          }

                       }

                    for(uint32 j=0;j<m_pHistoMatrix->getDimensionSize(1);j++)
                      {
                        if(m_pHistoMatrix->getDimensionSize(1)==m_AngleBin1.size())
                          {
                             m_pHistoMatrix->getBuffer()[j] = m_AngleBin1[j];
                          }
                        else
                          {
                            cout<<"Warning, error not enough bin : "<<endl;
                          }

                      }



 /*                   int cpt =0;
                    for(uint32 j=0; j<l_oTheta.size(); j++)
                      {

                        if(l_oTheta[j].first>0.7 && l_oTheta[j].first<3.1 && cpt<4)
                          {
                            angleSeuil.push_back(make_pair(l_oTheta[j].first, l_oTheta[j].second));

                          }

                        cpt++;
                       }


                    sort(angleSeuil.begin(),angleSeuil.end());

                    for(uint32 j=0; j<angleSeuil.size(); j++)
                      {
                        l_oThetaSorted.push_back(make_pair(angleSeuil[j].first, angleSeuil[j].second));
                      }

                    sort(l_oThetaSorted.begin(), l_oThetaSorted.end(),Sort_by_second());


                    if(m_pAnglesMatrix->getDimensionSize(1) == l_oThetaSorted.size())

                      {
 //                       uint32 l_ui32Index = l_oTheta[j].second;
                        for(uint32 j=0; j<m_pAnglesMatrix->getDimensionSize(1); j++)
                          {
                            m_pAnglesMatrix->getBuffer()[j] = l_oThetaSorted[j].first;

                            float64 l_f64SegmentLength = 0;
                            float64 l_f64Vector_x = 0;
                            float64 l_f64Vector_y = 0;

                            if(j==m_pAnglesMatrix->getDimensionSize(1)-1)
                              {
                                l_f64Vector_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(l_oTheta[0].second)]-l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+l_oTheta[j].second];
                                l_f64Vector_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(l_oTheta[0].second)]-l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(l_oTheta[j].second)];
                              }
                            else
                              {
                                l_f64Vector_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(l_oTheta[j+1].second)]-l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+l_oTheta[j].second];
                                l_f64Vector_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(l_oTheta[j+1].second)]-l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(l_oTheta[j].second)];
                              }

                            l_f64SegmentLength = sqrt( pow(l_f64Vector_x,2) + pow(l_f64Vector_y,2) );

                            m_pLengthMatrix->getBuffer()[j]=l_f64SegmentLength;

     //                       m_pVectorMatrix->getBuffer()[0*m_pVectorMatrix->getDimensionSize(1) + j] = l_f64Vector_x;
     //                       m_pVectorMatrix->getBuffer()[1*m_pVectorMatrix->getDimensionSize(1) + j] = l_f64Vector_x;
                          }
                        for(uint32 j=0; j<l_oThetaSorted.size()-1; j++)
                          {
                            cout<<"size ="<<l_oThetaSorted.size()<<endl;
                            l_f64Somme += l_oThetaSorted[j].first;
                              cout<<"Plus petits Angles = "<<l_oThetaSorted[j].first<<endl;
                              cout<<"size ="<<l_oThetaSorted.size()<<endl;
                          }
                        cout<<"Somme des angles = "<<l_f64Somme<<endl;
                        m_pVectorMatrix->getBuffer()[0]= l_f64Somme;
                        }

                        else
                          {
                            cout<<"Not enough angles in threshold : "<<l_oThetaSorted.size()<<endl;
                    //        return false;
                          }
*/

                    //Velocity computation
                    for(uint32 k = 1; k<l_ui32SamplesPerChannel; k++)
                    {
                       float64 l_f64Previous_x;
                       float64 l_f64Previous_y;

                       float64 l_f64Current_x;
                       float64 l_f64Current_y;

                       float64 l_f64Distance;

                       l_f64Previous_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+(k-1)];
                       l_f64Previous_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+(k-1)];

                       l_f64Current_x = l_pMousePositionMatrix->getBuffer()[0*l_ui32SamplesPerChannel+k];
                       l_f64Current_y = l_pMousePositionMatrix->getBuffer()[1*l_ui32SamplesPerChannel+k];

                       l_f64Distance = sqrt( pow(l_f64Current_x-l_f64Previous_x,2) + pow(l_f64Current_y-l_f64Previous_y,2) );

    //                   m_pVelocityMatrix->getBuffer()[k-1] = l_f64Distance*l_ui64SamplingFrequency;
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
		    m_oLength_FeatureVectorEncoder.encodeBuffer(1);
		    m_oVector_FeatureVectorEncoder.encodeBuffer(2);
		    m_oVelocity_SignalEncoder.encodeBuffer(3);
		    m_oAcceleration_SignalEncoder.encodeBuffer(4);
		    m_oHistoAngle_FeatureVectorEncoder.encodeBuffer(5);

		    // and send it to the next boxes :
		    l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(3, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(4, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(5, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			
		}

		if(m_oInputSignalDecoder.isEndReceived())
		{
		    // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
		    m_oAngle_FeatureVectorEncoder.encodeEnd(0);
		    m_oLength_FeatureVectorEncoder.encodeEnd(1);
		    m_oVector_FeatureVectorEncoder.encodeEnd(2);
		    m_oVelocity_SignalEncoder.encodeEnd(3);
		    m_oAcceleration_SignalEncoder.encodeEnd(4);
		    m_oHistoAngle_FeatureVectorEncoder.encodeEnd(5);

		    l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(3, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(4, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		    l_rDynamicBoxContext.markOutputAsReadyToSend(5, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

	}
	return true;
}
