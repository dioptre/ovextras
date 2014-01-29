#include "ovpCAlgorithmClassifierMixtureOfExperts.h"

#if defined TARGET_HAS_ThirdPartyITPP

#include <map>
#include <sstream>
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

/*boolean CAlgorithmClassifierMixtureOfExperts::initialize(void)
{
	//TParameterHandler < boolean > ip_bMode(this->getInputParameter(OVP_Algorithm_ClassifierMOE_InputParameterId_MOE_Mode));
	//ip_bMode=true;
	
	return CAlgorithmClassifier::initialize();
}*/

boolean CAlgorithmClassifierMixtureOfExperts::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	/*uint32 i;
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

	itpp::mat l_oSigmaInverse=itpp::inv(l_oSigma);

	m_oCoefficients=l_oSigmaInverse * (l_oMeanFeatureVector1 - l_oMeanFeatureVector2);
	m_oCoefficients.ins(0, -0.5 * ((l_oMeanFeatureVector1+l_oMeanFeatureVector2).transpose() * m_oCoefficients)[0]);

	std::stringstream l_sClasses;
	std::stringstream l_sCoefficients;

	l_sClasses << m_f64Class1 << " " << m_f64Class2;
	l_sCoefficients << std::scientific << m_oCoefficients[0];
	for(int i=1; i<m_oCoefficients.size(); i++)
	{
		l_sCoefficients << " " << m_oCoefficients[i];
	}

	m_oConfiguration.setSize(0, true);
	XML::IWriter* l_pWriter=XML::createWriter(*this);
	l_pWriter->openChild("OpenViBE-Classifier");
	 l_pWriter->openChild("LDA");
	  l_pWriter->openChild("Classes");
	   l_pWriter->setChildData(l_sClasses.str().c_str());
	  l_pWriter->closeChild();
	  l_pWriter->openChild("Coefficients");
	   l_pWriter->setChildData(l_sCoefficients.str().c_str());
	  l_pWriter->closeChild();
	 l_pWriter->closeChild();
	l_pWriter->closeChild();
	l_pWriter->release();
	l_pWriter=NULL;*/

	return true;
}

boolean CAlgorithmClassifierMixtureOfExperts::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	if(rFeatureVector.getSize()+1!=(unsigned int)m_vCoefficientsClass1[0].size())
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " and hyperplane parameter size " << (uint32) m_vCoefficientsClass1[0].size() << " does not match\n";
		return false;
	}
	
	//TParameterHandler < boolean > ip_bMode(this->getInputParameter(OVP_Algorithm_ClassifierMOE_InputParameterId_MOE_Mode));

	itpp::vec l_oFeatures(rFeatureVector.getBuffer(), rFeatureVector.getSize());
	l_oFeatures.ins(0, 1);

	if (m_ui32Mode==1)
	{
		m_f64Result = 0;
		for (uint32 i=0; i<m_ui32NumberOfExperts; i++)
		{
			m_f64Result+=std::exp(l_oFeatures*m_vCoefficientsClass1[i])
					/(std::exp(l_oFeatures*m_vCoefficientsClass1[i])+std::exp(l_oFeatures*m_vCoefficientsClass2[i]));		
		}
		m_f64Result /= static_cast<float64>(m_ui32NumberOfExperts);
	}
	else
	{
		m_ui32ClassificationCount++;
		m_f64Result+=std::exp(l_oFeatures*m_vCoefficientsClass1[m_ui32ClassificationCount-1])
					/(std::exp(l_oFeatures*m_vCoefficientsClass1[m_ui32ClassificationCount-1])+
					std::exp(l_oFeatures*m_vCoefficientsClass2[m_ui32ClassificationCount-1]));
		/*std::cout << "Feature vectors\n";
		for (uint32 i =0; i<l_oFeatures.size(); i++)
			std::cout << l_oFeatures[i] << " ";
		std::cout << "\n";
		std::cout << "Output: " << (l_oFeatures*m_vCoefficients[m_ui32ClassificationCount-1]) << "\n";*/
		if (m_ui32ClassificationCount%m_ui32NumberOfExperts==0)
			m_f64Result /= static_cast<float64>(m_ui32NumberOfExperts);
	}
	
	if (m_ui32Mode==1 || m_ui32ClassificationCount%m_ui32NumberOfExperts==0)
	{
		//this->getLogManager() << LogLevel_Info << "Classification output " << m_f64Result << "\n";
		rClassificationValues.setSize(1);
		rClassificationValues[0]=m_f64Result;

		if(m_f64Result >= 0.5)
		{
			rf64Class=m_f64Class1;
		}
		else
		{
			rf64Class=m_f64Class2;
		}
		
		m_ui32ClassificationCount = 0;
		m_f64Result = 0;
		
		return true;
	}
	else
		return false;
}

boolean CAlgorithmClassifierMixtureOfExperts::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{
	rMemoryBuffer.setSize(0, true);
	rMemoryBuffer.append(m_oConfiguration);
	return true;
}

boolean CAlgorithmClassifierMixtureOfExperts::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{
	m_f64Class1=0;
	m_f64Class2=0;
	m_ui32NumberOfExperts=0;
	m_ui32ClassificationCount=0;
	m_f64Result = 0;
	std::cout << "load configuration\n";
	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;

	return true;
}

void CAlgorithmClassifierMixtureOfExperts::write(const char* sString)
{
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CAlgorithmClassifierMixtureOfExperts::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
	
	
	if (CString(sName)==CString("OpenViBE-Classifier") && ui64AttributeCount==1)
	{
		std::stringstream l_sData(*sAttributeValue);
		l_sData >> m_ui32Mode;
		//this->getLogManager() << LogLevel_Info << "Mode of classifier " << m_ui32Mode << "\n";
	}
}

void CAlgorithmClassifierMixtureOfExperts::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

	if(m_vNode.top()==CString("Classes"))
	{
		l_sData >> m_f64Class1;
		l_sData >> m_f64Class2;
	}

	if(m_vNode.top()==CString("CoefficientsClass1") || m_vNode.top()==CString("CoefficientsClass2"))
	{
		std::vector < float64 > l_vCoefficients;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vCoefficients.push_back(l_f64Value);
		}

		if(m_vNode.top()==CString("CoefficientsClass1"))
		{
			m_vCoefficientsClass1.push_back(itpp::vec(l_vCoefficients.size()));
			//m_vCoefficients[m_ui32NumberOfExperts].set_size(l_vCoefficients.size());
			for(size_t i=0; i<l_vCoefficients.size(); i++)
			{
				m_vCoefficientsClass1[m_ui32NumberOfExperts][i]=l_vCoefficients[i];
				//std::cout << m_vCoefficients[m_ui32NumberOfExperts][i] << " ";
			}	
		}
		
		if(m_vNode.top()==CString("CoefficientsClass2"))
		{
			m_vCoefficientsClass2.push_back(itpp::vec(l_vCoefficients.size()));
			//m_vCoefficients[m_ui32NumberOfExperts].set_size(l_vCoefficients.size());
			for(size_t i=0; i<l_vCoefficients.size(); i++)
			{
				m_vCoefficientsClass2[m_ui32NumberOfExperts][i]=l_vCoefficients[i];
				//std::cout << m_vCoefficients[m_ui32NumberOfExperts][i] << " ";
			}	
		}		
		//std::cout << "\n " << l_vCoefficients.size() << "\n";
	}
}

void CAlgorithmClassifierMixtureOfExperts::closeChild(void)
{
	if(m_vNode.top()==CString("LDA"))
	{
		m_ui32NumberOfExperts++;
		//this->getLogManager() << LogLevel_Info << "Number of experts  " << m_ui32NumberOfExperts << "\n";
	}
	m_vNode.pop();
}

#endif // TARGET_HAS_ThirdPartyITPP
