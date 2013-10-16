#include "ovpCAlgorithmClassifierRelearnPLDA.h"

#if defined TARGET_HAS_ThirdPartyITPP

#include <map>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

#define CIRCULAR_BUFFER_SIZE 3600

boolean CAlgorithmClassifierRelearnPLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	TParameterHandler < int64 > ip_i64ShrinkageType(this->getInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Shrinkage));
	TParameterHandler < float64 > l_f64Lambda(this->getInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Lambda));
	float l_f32Lambda = static_cast<float>(l_f64Lambda);
	
	uint32 i;

	uint32 l_ui32NumberOfFeatures=rFeatureVectorSet[0].getSize();
	uint32 l_ui32NumberOfFeatureVectors=rFeatureVectorSet.getFeatureVectorCount();
	
	for (uint32 i=0; i<l_ui32NumberOfFeatureVectors; i++)
	{
		if (m_ui32BufferPointer==CIRCULAR_BUFFER_SIZE)
		{
			m_ui32BufferPointer = 0;
			m_bBufferFilled = true;
		}
		if (m_ui32BufferPointer+1>m_vCircularSampleBuffer.size())
		{
			m_vCircularSampleBuffer.push_back(itpp::vec(rFeatureVectorSet[i].getBuffer(), l_ui32NumberOfFeatures));
			m_vCircularLabelBuffer.push_back(rFeatureVectorSet[i].getLabel());
		}
		else
		{
			m_vCircularSampleBuffer[m_ui32BufferPointer] = itpp::vec(rFeatureVectorSet[i].getBuffer(), l_ui32NumberOfFeatures);
			m_vCircularLabelBuffer[m_ui32BufferPointer] = rFeatureVectorSet[i].getLabel();
		}	
		
		//std::cout << "CAlgorithmClassifierPLDA::Label " << m_vCircularLabelBuffer[m_ui32BufferPointer] << "\n";
		
		m_ui32BufferPointer++;
	}
	
	std::cout << "Buffer pointer info " << m_bBufferFilled << " " << CIRCULAR_BUFFER_SIZE << " " << m_ui32BufferPointer << "\n";
	
	m_ui32BufferEnd = m_bBufferFilled?CIRCULAR_BUFFER_SIZE:m_ui32BufferPointer;
	
	std::map < float64, uint64 > l_vClassLabels;
	for(i=0; i<m_ui32BufferEnd; i++)
	{
		//std::cout << "Label " << i << " " << m_vCircularLabelBuffer[i] << "\n";
		l_vClassLabels[m_vCircularLabelBuffer[i]]++;
	}
	/*std::cout << "Buffer end " << m_ui32BufferEnd << "\n";
	std::cout << "Number of objects for class " << l_vClassLabels.begin()->first << " is " << l_vClassLabels.begin()->second << "\n";
	std::cout << "Number of objects for class " << l_vClassLabels.rbegin()->first << " is " << l_vClassLabels.rbegin()->second << "\n";*/

	if(l_vClassLabels.size() != 2)
	{
		this->getLogManager() << LogLevel_Error << "A LDA classifier can only be trained with 2 classes, not more, not less - got " << (uint32)l_vClassLabels.size() << "\n";
		return false;
	}	

	m_f64Class1=l_vClassLabels.begin()->first;
	m_f64Class2=l_vClassLabels.rbegin()->first;

	itpp::vec l_oMeanFeatureVector1(l_ui32NumberOfFeatures);
	itpp::vec l_oMeanFeatureVector2(l_ui32NumberOfFeatures);

	l_oMeanFeatureVector1.zeros();
	l_oMeanFeatureVector2.zeros();

	//FILE* timingFile = fopen(OpenViBE::Directories::getDataDir() + "/train_samples.txt","w");		
	for(i=0; i<m_ui32BufferEnd; i++)
	{
		if(m_vCircularLabelBuffer[i]==m_f64Class1)
		{
			l_oMeanFeatureVector1+=m_vCircularSampleBuffer[i];
		}

		if(m_vCircularLabelBuffer[i]==m_f64Class2)
		{
			l_oMeanFeatureVector2+=m_vCircularSampleBuffer[i];
		}
		
		//fprintf(timingFile, "m_vCircularSampleBuffer %i label %i \n",i,static_cast<int>(m_vCircularLabelBuffer[i]));
		/*for(uint32 ii=0; ii<m_vCircularSampleBuffer[i].size(); ii++)
			fprintf(timingFile, "%f ",static_cast<float>(m_vCircularSampleBuffer[i][ii]));	
		fprintf(timingFile, "%f ",static_cast<float>(m_vCircularLabelBuffer[i]));	
		fprintf(timingFile, "\n");*/
	}
	//fclose(timingFile);

	l_oMeanFeatureVector1/=(double)l_vClassLabels[m_f64Class1];
	l_oMeanFeatureVector2/=(double)l_vClassLabels[m_f64Class2];

	itpp::vec l_oDiff;
	itpp::mat l_oSigma(l_ui32NumberOfFeatures, l_ui32NumberOfFeatures);
	l_oSigma.zeros();

	for(i=0; i<m_ui32BufferEnd; i++)
	{
		if(m_vCircularLabelBuffer[i]==m_f64Class1)
		{
			l_oDiff=m_vCircularSampleBuffer[i] - l_oMeanFeatureVector1;
			l_oSigma+=itpp::outer_product(l_oDiff, l_oDiff);
		}

		if(m_vCircularLabelBuffer[i]==m_f64Class2)
		{
			l_oDiff=m_vCircularSampleBuffer[i] - l_oMeanFeatureVector2;
			l_oSigma+=itpp::outer_product(l_oDiff, l_oDiff);
		}
	}
	l_oSigma = l_oSigma / (m_ui32BufferEnd-2.);
 	if (ip_i64ShrinkageType==FULL)
	{
	}
	else if (ip_i64ShrinkageType==DIAG)
	{
		itpp::vec l_vDiagonal = itpp::diag(l_oSigma);
		l_oSigma.zeros();
		for (int32 i=0; i<l_vDiagonal.length(); i++)
			l_oSigma.set(i,i,l_vDiagonal[i]);
	}
	else if (ip_i64ShrinkageType==SHRINK_TO_DIAG)
	{
		itpp::vec l_vDiagonal = itpp::diag(l_oSigma);
		itpp::mat l_mDiagonal = itpp::mat(l_vDiagonal.length(), l_vDiagonal.length());
		l_mDiagonal.zeros();
		for (int32 i=0; i<l_vDiagonal.length(); i++)
			l_mDiagonal.set(i,i,l_vDiagonal[i]);
 		l_oSigma = (1.0-l_f32Lambda)*l_oSigma + l_f32Lambda*l_mDiagonal;
	}
	else if (ip_i64ShrinkageType==SHRINK_TO_UNITY)
	{
		itpp::mat l_mDiagonal = itpp::mat(l_oSigma.rows(), l_oSigma.rows());
		l_mDiagonal.zeros();
		for (int32 i=0; i<l_oSigma.rows(); i++)
			l_mDiagonal.set(i,i,1.0);
 		l_oSigma = (1.0-l_f32Lambda)*l_oSigma + l_f32Lambda*l_mDiagonal;		
	}	
	itpp::mat l_oSigmaInverse=itpp::inv(l_oSigma);

	/*std::cout << "Training mean 1\n";
	for(i=0; i<l_oMeanFeatureVector1.size(); i++)
		std::cout << " " << l_oMeanFeatureVector1[i];
	std::cout << "\n";	
	std::cout << "Training mean 2\l_ui32NumberOfFeatureVectorsn";
	for(i=0; i<l_oMeanFeatureVector2.size(); i++)
		std::cout << " " << l_oMeanFeatureVector2[i];
	std::cout << "\n";	*/	
	
	m_oCoefficientsClass1=l_oSigmaInverse * l_oMeanFeatureVector1;
	m_oCoefficientsClass1.ins(0, -0.5 * (l_oMeanFeatureVector1.transpose() * l_oSigmaInverse * l_oMeanFeatureVector1));
	m_oCoefficientsClass2=l_oSigmaInverse * l_oMeanFeatureVector2;
	m_oCoefficientsClass2.ins(0, -0.5 * (l_oMeanFeatureVector2.transpose() * l_oSigmaInverse * l_oMeanFeatureVector2));

	/*std::cout << "Training m_oCoefficientsClass1\n";
	for(i=0; i<m_oCoefficientsClass1.size(); i++)
		std::cout << " " << m_oCoefficientsClass1[i];
	std::cout << "\n";	
	std::cout << "Training m_oCoefficientsClass2\n";
	for(i=0; i<m_oCoefficientsClass2.size(); i++)
		std::cout << " " << m_oCoefficientsClass2[i];
	std::cout << "\n";	*/
	
	std::stringstream l_sClasses;
	std::stringstream l_sCoefficientsClass1;
	std::stringstream l_sCoefficientsClass2;
	std::stringstream l_sSamples;

	l_sClasses << m_f64Class1 << " " << m_f64Class2;
	l_sCoefficientsClass1 << std::scientific << m_oCoefficientsClass1[0];
	l_sCoefficientsClass2 << std::scientific << m_oCoefficientsClass2[0];
	for(int i=1; i<m_oCoefficientsClass1.size(); i++)
	{
		l_sCoefficientsClass1 << " " << m_oCoefficientsClass1[i];
		l_sCoefficientsClass2 << " " << m_oCoefficientsClass2[i];
	}		

	m_oConfiguration.setSize(0, true);
	XML::IWriter* l_pWriter=XML::createWriter(*this);
	l_pWriter->openChild("OpenViBE-Classifier");
	 l_pWriter->openChild("LDA");
	  l_pWriter->openChild("Classes");
	   l_pWriter->setChildData(l_sClasses.str().c_str());
	  l_pWriter->closeChild();
	  l_pWriter->openChild("CoefficientsClass1");
	   l_pWriter->setChildData(l_sCoefficientsClass1.str().c_str());
	  l_pWriter->closeChild();
	  l_pWriter->openChild("CoefficientsClass2");
	   l_pWriter->setChildData(l_sCoefficientsClass2.str().c_str());
	  l_pWriter->closeChild();	  
	 l_pWriter->closeChild();
	 
	 l_pWriter->openChild("SampleSet");
		for(i=0; i<m_ui32BufferEnd; i++)
		{	
			l_pWriter->openChild("Sample");
			l_sClasses.str(std::string());
			l_sClasses << m_vCircularLabelBuffer[i];
			l_pWriter->setAttribute("label",l_sClasses.str().c_str());
			l_sSamples.str(std::string());
			for (uint32 j=0; j<m_vCircularSampleBuffer[i].length(); j++)
				l_sSamples << " " << m_vCircularSampleBuffer[i][j];
			l_pWriter->setChildData(l_sSamples.str().c_str());
			l_pWriter->closeChild();
		}	 
	 l_pWriter->closeChild();
	l_pWriter->closeChild();
	l_pWriter->release();
	l_pWriter=NULL;

	return true;
}

boolean CAlgorithmClassifierRelearnPLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	if(rFeatureVector.getSize()+1!=(unsigned int)m_oCoefficientsClass1.size())
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " and hyperplane parameter size " << (uint32) m_oCoefficientsClass1.size() << " does not match\n";
		return false;
	}
	
	/*std::cout << "Test m_oCoefficientsClass1\n";
	for(uint32 i=0; i<m_oCoefficientsClass1.size(); i++)
		std::cout << " " << m_oCoefficientsClass1[i];
	std::cout << "\n";	
	std::cout << "Test m_oCoefficientsClass2\n";
	for(uint32 i=0; i<m_oCoefficientsClass2.size(); i++)
		std::cout << " " << m_oCoefficientsClass2[i];
	std::cout << "\n";	*/

	itpp::vec l_oFeatures(rFeatureVector.getBuffer(), rFeatureVector.getSize());

	l_oFeatures.ins(0, 1);

	float64 l_f64Result=std::exp(l_oFeatures*m_oCoefficientsClass1)
					/(std::exp(l_oFeatures*m_oCoefficientsClass1)+std::exp(l_oFeatures*m_oCoefficientsClass2));
					
	this->getLogManager() << LogLevel_Debug << "p(Class1|x)=" << l_f64Result << "\n";
	
	rClassificationValues.setSize(1);
	rClassificationValues[0]=l_f64Result;

	if(l_f64Result >= 0.5)
	{
		rf64Class=m_f64Class1;
	}
	else
	{
		rf64Class=m_f64Class2;
	}

	return true;
}

boolean CAlgorithmClassifierRelearnPLDA::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{
	std::cout << "saving config relearn plda\n";
	rMemoryBuffer.setSize(0, true);
	rMemoryBuffer.append(m_oConfiguration);
	return true;
}

boolean CAlgorithmClassifierRelearnPLDA::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{
	m_f64Class1=0;
	m_f64Class2=0;
	m_ui32BufferPointer = 0;
	m_bBufferFilled = false;
	m_vCircularSampleBuffer = std::vector<itpp::vec>(CIRCULAR_BUFFER_SIZE);
	m_vCircularLabelBuffer = std::vector<uint64>(CIRCULAR_BUFFER_SIZE);
	//std::cout << "load configuration 1\n";
	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;
	//std::cout << "load configuration 2\n";

	return true;
}

void CAlgorithmClassifierRelearnPLDA::write(const char* sString)
{
	//std::cout << " " << sString << "\n";
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CAlgorithmClassifierRelearnPLDA::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
	
	if(CString(sName)==CString("Sample"))
	{
		for (uint32 i=0; i<ui64AttributeCount; i++)
			if (CString(*(sAttributeName+i))==CString("label"))
				m_ui64TmpLabel = getConfigurationManager().expandAsUInteger(CString(*(sAttributeValue+i)));
	}		
}

void CAlgorithmClassifierRelearnPLDA::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

	if(m_vNode.top()==CString("Classes"))
	{
		l_sData >> m_f64Class1;
		l_sData >> m_f64Class2;
		//std::cout << "Classes \n";
	}

	if(m_vNode.top()==CString("CoefficientsClass1") || m_vNode.top()==CString("CoefficientsClass2"))
	{
		//std::cout << "Coefficients \n";
		std::vector < float64 > l_vCoefficients;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vCoefficients.push_back(l_f64Value);
		}
		if(m_vNode.top()==CString("CoefficientsClass1"))
		{
			m_oCoefficientsClass1.set_size(l_vCoefficients.size());
			//std::cout << "coefficients 1\n";
			for(size_t i=0; i<l_vCoefficients.size(); i++)
			{
				m_oCoefficientsClass1[i]=l_vCoefficients[i];
				//std::cout << m_oCoefficientsClass1[i] << " ";
			}
			//std::cout << "\n";
		}	
		else if (m_vNode.top()==CString("CoefficientsClass2"))
		{
			m_oCoefficientsClass2.set_size(l_vCoefficients.size());
			//std::cout << "coefficients 2\n";
			for(size_t i=0; i<l_vCoefficients.size(); i++)
			{
				m_oCoefficientsClass2[i]=l_vCoefficients[i];
				//std::cout << m_oCoefficientsClass2[i] << " ";
			}
			//std::cout << "\n";
		}		
	}
	
	if(m_vNode.top()==CString("Sample"))
	{
		std::vector < float64 > l_vSample;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vSample.push_back(l_f64Value);
		}	
		itpp::vec l_vSampleVector(l_vSample.size());
		for(size_t i=0; i<l_vSample.size(); i++)
			l_vSampleVector[i]=l_vSample[i];
		
		if (m_ui32BufferPointer==CIRCULAR_BUFFER_SIZE)
		{
			m_ui32BufferPointer = 0;
			m_bBufferFilled = true;
		}
		m_vCircularSampleBuffer[m_ui32BufferPointer] = l_vSampleVector;
		m_vCircularLabelBuffer[m_ui32BufferPointer] = m_ui64TmpLabel;
		m_ui32BufferPointer++;
	}		
}

void CAlgorithmClassifierRelearnPLDA::closeChild(void)
{
	m_vNode.pop();
}

#endif // TARGET_HAS_ThirdPartyITPP
