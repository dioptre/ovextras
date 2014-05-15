#include "ovpCAlgorithmClassifierPLDA.h"

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

boolean CAlgorithmClassifierPLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	TParameterHandler < int64 > ip_i64ShrinkageType(this->getInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Shrinkage));
	TParameterHandler < float64 > l_f64Lambda(this->getInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Lambda));
	float l_f32Lambda = static_cast<float>(l_f64Lambda);
	
	uint32 i;
	std::map < float64, uint64 > l_vClassLabels;
	for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	if(l_vClassLabels.size() != 2)
	{
		this->getLogManager() << LogLevel_Error << "A LDA classifier can only be trained with 2 classes, not more, not less - got " << (uint32)l_vClassLabels.size() << "\n";
		return false;
	}

	uint32 l_ui32NumberOfFeatures=rFeatureVectorSet[0].getSize();
	uint32 l_ui32NumberOfFeatureVectors=rFeatureVectorSet.getFeatureVectorCount();

	m_f64Class1=l_vClassLabels.begin()->first;
	m_f64Class2=l_vClassLabels.rbegin()->first;

	itpp::vec l_oMeanFeatureVector1(l_ui32NumberOfFeatures);
	itpp::vec l_oMeanFeatureVector2(l_ui32NumberOfFeatures);

	l_oMeanFeatureVector1.zeros();
	l_oMeanFeatureVector2.zeros();

	for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		const IFeatureVector& l_rFeatureVector=rFeatureVectorSet[i];

		float64 l_f64Label=l_rFeatureVector.getLabel();

		if(l_f64Label==m_f64Class1)
		{
			l_oMeanFeatureVector1+=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize());
		}

		if(l_f64Label==m_f64Class2)
		{
			l_oMeanFeatureVector2+=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize());
		}
	}

	l_oMeanFeatureVector1/=(double)l_vClassLabels[m_f64Class1];
	l_oMeanFeatureVector2/=(double)l_vClassLabels[m_f64Class2];

	itpp::vec l_oDiff;
	itpp::mat l_oSigma(l_ui32NumberOfFeatures, l_ui32NumberOfFeatures);
	l_oSigma.zeros();

	for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		const IFeatureVector& l_rFeatureVector=rFeatureVectorSet[i];

		float64 l_f64Label=l_rFeatureVector.getLabel();

		if(l_f64Label==m_f64Class1)
		{
			l_oDiff=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize()) - l_oMeanFeatureVector1;
			l_oSigma+=itpp::outer_product(l_oDiff, l_oDiff) / (l_ui32NumberOfFeatureVectors-2.);
		}

		if(l_f64Label==m_f64Class2)
		{
			l_oDiff=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize()) - l_oMeanFeatureVector2;
			l_oSigma+=itpp::outer_product(l_oDiff, l_oDiff) / (l_ui32NumberOfFeatureVectors-2.);
		}
	}
	
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
	l_pWriter->closeChild();
	l_pWriter->release();
	l_pWriter=NULL;

	return true;
}

boolean CAlgorithmClassifierPLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
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

boolean CAlgorithmClassifierPLDA::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{
	rMemoryBuffer.setSize(0, true);
	rMemoryBuffer.append(m_oConfiguration);
	return true;
}

boolean CAlgorithmClassifierPLDA::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{
	m_f64Class1=0;
	m_f64Class2=0;
	//std::cout << "load configuration 1\n";
	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;
	//std::cout << "load configuration 2\n";

	return true;
}

void CAlgorithmClassifierPLDA::write(const char* sString)
{
	//std::cout << " " << sString << "\n";
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CAlgorithmClassifierPLDA::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);	
}

void CAlgorithmClassifierPLDA::processChildData(const char* sData)
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
}

void CAlgorithmClassifierPLDA::closeChild(void)
{
	m_vNode.pop();
}

#endif // TARGET_HAS_ThirdPartyITPP
