#include "ovpCAlgorithmClassifierSVM.h"

//#include <map>
#include <sstream>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>

#include <xml/IXMLHandler.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

CAlgorithmClassifierSVM::CAlgorithmClassifierSVM(void)
	:m_pModel(NULL)
{
}

boolean CAlgorithmClassifierSVM::initialize(void)
{
	TParameterHandler < int64 > ip_i64SVMType(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMType));
	TParameterHandler < int64 > ip_i64SVMKernelType(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMKernelType));
	TParameterHandler < int64 > ip_i64Degree(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMDegree));
	TParameterHandler < float64 > ip_f64Gamma(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMGamma));
	TParameterHandler < float64 > ip_f64Coef0(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMCoef0));
	TParameterHandler < float64 > ip_f64Cost(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCost));
	TParameterHandler < float64 > ip_f64Nu(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMNu));
	TParameterHandler < float64 > ip_f64Epsilon(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMEpsilon));
	TParameterHandler < float64 > ip_f64CacheSize(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCacheSize));
	TParameterHandler < float64 > ip_f64EpsilonTolerance(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMTolerance));
	TParameterHandler < boolean > ip_bShrinking(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking));
	//TParameterHandler < boolean > ip_bProbabilityEstimate(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMProbabilityEstimate));
	TParameterHandler < CString* > ip_sWeight(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMweight));
	TParameterHandler < CString* > ip_sWeightLabel(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMWeightLabel));

	ip_i64SVMType=C_SVC;
	ip_i64SVMKernelType=LINEAR;
	ip_i64Degree=3;
	ip_f64Gamma=0;
	ip_f64Coef0=0;
	ip_f64Cost=1;
	ip_f64Nu=0.5;
	ip_f64Epsilon=0.1;
	ip_f64CacheSize=100;
	ip_f64EpsilonTolerance=0.001;
	ip_bShrinking=true;
	//boolean ip_bProbabilityEstimate=true;
	*ip_sWeight="";
	*ip_sWeightLabel="";

	return CAlgorithmClassifier::initialize();
}

boolean CAlgorithmClassifierSVM::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	// default Param values
	//std::cout<<"param config"<<std::endl;

	TParameterHandler < int64 > ip_i64SVMType(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMType));
	TParameterHandler < int64 > ip_i64SVMKernelType(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMKernelType));
	TParameterHandler < int64 > ip_i64Degree(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMDegree));
	TParameterHandler < float64 > ip_f64Gamma(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMGamma));
	TParameterHandler < float64 > ip_f64Coef0(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMCoef0));
	TParameterHandler < float64 > ip_f64Cost(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCost));
	TParameterHandler < float64 > ip_f64Nu(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMNu));
	TParameterHandler < float64 > ip_f64Epsilon(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMEpsilon));
	TParameterHandler < float64 > ip_f64CacheSize(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCacheSize));
	TParameterHandler < float64 > ip_f64EpsilonTolerance(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMTolerance));
	TParameterHandler < boolean > ip_bShrinking(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking));
	//TParameterHandler < boolean > ip_bProbabilityEstimate(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMProbabilityEstimate));
	TParameterHandler < CString* > ip_sWeight(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMweight));
	TParameterHandler < CString* > ip_sWeightLabel(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMWeightLabel));

	boolean ip_bProbabilityEstimate=true;

	m_oParam.svm_type = (int)ip_i64SVMType;//C_SVC;
	m_oParam.kernel_type = (int)ip_i64SVMKernelType;//LINEAR;
	m_oParam.degree = (int)ip_i64Degree;//3;
	m_oParam.gamma = ip_f64Gamma;//0;	// 1/num_features
	m_oParam.coef0 = ip_f64Coef0;//0;
	m_oParam.nu = ip_f64Nu;//0.5;
	m_oParam.cache_size = ip_f64CacheSize;//100;
	m_oParam.C = ip_f64Cost;//1;
	m_oParam.eps = ip_f64EpsilonTolerance;//1e-3;
	m_oParam.p = ip_f64Epsilon;//0.1;
	m_oParam.shrinking = ip_bShrinking;//1;
	m_oParam.probability = ip_bProbabilityEstimate;//1;
	m_oParam.nr_weight = 0;
	m_oParam.weight = NULL;
	m_oParam.weight_label = NULL;

	std::vector<float64> l_vWeight;
	CString l_sParamWeight=*ip_sWeight;
	std::stringstream l_oStreamString((const char*)l_sParamWeight);
	float64 l_f64CurrentValue;
	while(l_oStreamString>>l_f64CurrentValue)
	{
		l_vWeight.push_back(l_f64CurrentValue);
	}
	m_oParam.nr_weight = l_vWeight.size();
	double * l_pWeight=new double[l_vWeight.size()];
	for(uint32 i=0;i<l_vWeight.size();i++)
	{
		l_pWeight[i]=l_vWeight[i];
	}
	m_oParam.weight = l_pWeight;//NULL;

	std::vector<int64> l_vWeightLabel;
	CString l_sParamWeightLabel=*ip_sWeightLabel;
	std::stringstream l_oStreamStringLabel((const char*)l_sParamWeightLabel);
	int64 l_i64CurrentValue;
	while(l_oStreamStringLabel>>l_i64CurrentValue)
	{
		l_vWeightLabel.push_back(l_i64CurrentValue);
	}

	//the number of weight label need to be equal to the number of weight
	while(l_vWeightLabel.size()<l_vWeight.size())
	{
		l_vWeightLabel.push_back(l_vWeightLabel.size()+1);
	}

	int * l_pWeightLabel=new int[l_vWeight.size()];
	for(uint32 i=0;i<l_vWeight.size();i++)
	{
		l_pWeightLabel[i]=(int)l_vWeightLabel[i];
	}
	m_oParam.weight_label = l_pWeightLabel;//NULL;

	this->getLogManager() << LogLevel_Trace << paramToString(&m_oParam);

	//configure m_oProb
	//std::cout<<"prob config"<<std::endl;
	struct svm_problem l_oProb;
	l_oProb.l=rFeatureVectorSet.getFeatureVectorCount();
	m_ui32NumberOfFeatures=rFeatureVectorSet[0].getSize();

	l_oProb.y = new double[l_oProb.l];
	l_oProb.x = new svm_node*[l_oProb.l];

	//std::cout<< "number vector:"<<l_oProb.l<<" size of vector:"<<m_ui32NumberOfFeatures<<std::endl;

	for(int i=0;i<l_oProb.l;i++)
	{
		l_oProb.x[i] = new svm_node[m_ui32NumberOfFeatures+1];
		l_oProb.y[i] = rFeatureVectorSet[i].getLabel();
		for(uint32 j=0;j<m_ui32NumberOfFeatures;j++)
		{
			l_oProb.x[i][j].index=j;
			l_oProb.x[i][j].value=rFeatureVectorSet[i].getBuffer()[j];
		}
		l_oProb.x[i][m_ui32NumberOfFeatures].index=-1;
	}
	if(m_oParam.gamma == 0 && m_ui32NumberOfFeatures > 0)
	{
		m_oParam.gamma = 1.0/m_ui32NumberOfFeatures;
	}

	if(m_oParam.kernel_type == PRECOMPUTED)
	{
		for(int i=0;i<l_oProb.l;i++)
		{
			if(l_oProb.x[i][0].index!=0)
			{
				this->getLogManager() << LogLevel_Error << "Wrong input format: first column must be 0:sample_serial_number\n";
				return false;
			}
			if(l_oProb.x[i][0].value <= 0 || l_oProb.x[i][0].value > m_ui32NumberOfFeatures)
			{
				this->getLogManager() << LogLevel_Error << "Wrong input format: sample_serial_number out of range\n";
				return false;
			}
		}
	}

	this->getLogManager() << LogLevel_Trace << problemToString(&l_oProb);

	//make a model
	//std::cout<<"svm_train"<<std::endl;
	if(m_pModel)
	{
		//std::cout<<"delete model"<<std::endl;
		delete m_pModel;
		m_pModel=NULL;
	}
	m_pModel=svm_train(&l_oProb,&m_oParam);

	if(m_pModel == NULL)
	{
		this->getLogManager() << LogLevel_Error << "the training with SVM had failed\n";
		return false;
	}

	//std::cout<<"log model"<<std::endl;
	this->getLogManager() << LogLevel_Trace << modelToString();

	//xml file
	//std::cout<<"model save"<<std::endl;

	std::vector <OpenViBE::CString> l_vSVCoef;
	std::vector <OpenViBE::CString> l_vSVValue;

	//std::cout<<"model save: rho"<<std::endl;
	std::stringstream l_sRho;
	l_sRho << std::scientific << m_pModel->rho[0];

	for(int i=1;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
	{
		l_sRho << " " << m_pModel->rho[i];
	}

	//std::cout<<"model save: sv_coef and SV"<<std::endl;
	for(int i=0;i<m_pModel->l;i++)
	{
		std::stringstream l_sSVCoef;
		std::stringstream l_sSVValue;

		l_sSVCoef << m_pModel->sv_coef[0][i];
		for(int j=1;j<m_pModel->nr_class-1;j++)
		{
			l_sSVCoef << " " << m_pModel->sv_coef[j][i];
		}

		const svm_node *p = m_pModel->SV[i];

		if(m_pModel->param.kernel_type == PRECOMPUTED)
		{
			l_sSVValue << "0:" << (double)(p->value);
		}
		else
		{
			if(p->index != -1)
			{
				l_sSVValue << p->index << ":" <<p->value;
				p++;
			}
			while(p->index != -1)
			{
				l_sSVValue << " " << p->index << ":" << p->value;
				p++;
			}
		}
		l_vSVCoef.push_back(CString(l_sSVCoef.str().c_str()));
		l_vSVValue.push_back(CString(l_sSVValue.str().c_str()));
	}

	//std::cout<<"xml save"<<std::endl;
	m_pConfigurationNode = XML::createNode("OpenViBE-Classifier");

	XML::IXMLNode *l_pSVMNode = XML::createNode("SVM");

	//Param node
	XML::IXMLNode *l_pParamNode = XML::createNode("Param");
	XML::IXMLNode *l_pTempNode = XML::createNode("svm_type");
	l_pTempNode->setPCData(get_svm_type(m_pModel->param.svm_type));
	l_pParamNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode("kernel_type");
	l_pTempNode->setPCData(get_kernel_type(m_pModel->param.kernel_type));
	l_pParamNode->addChild(l_pTempNode);

	if(m_pModel->param.kernel_type == POLY)
	{
		std::stringstream l_sParamDegree;
		l_sParamDegree << m_pModel->param.degree;

		l_pTempNode = XML::createNode("degree");
		l_pTempNode->setPCData(l_sParamDegree.str().c_str());
		l_pParamNode->addChild(l_pTempNode);
	}
	if(m_pModel->param.kernel_type == POLY || m_pModel->param.kernel_type == RBF || m_pModel->param.kernel_type == SIGMOID)
	{
		std::stringstream l_sParamGamma;
		l_sParamGamma << m_pModel->param.gamma;

		l_pTempNode = XML::createNode("gamma");
		l_pTempNode->setPCData(l_sParamGamma.str().c_str());
		l_pParamNode->addChild(l_pTempNode);
	}
	if(m_pModel->param.kernel_type == POLY || m_pModel->param.kernel_type == SIGMOID)
	{
		std::stringstream l_sParamCoef0;
		l_sParamCoef0 << m_pModel->param.coef0;

		l_pTempNode = XML::createNode("coef0");
		l_pTempNode->setPCData(l_sParamCoef0.str().c_str());
		l_pParamNode->addChild(l_pTempNode);
	}
	l_pSVMNode->addChild(l_pParamNode);
	//End param node

	//Model Node
	XML::IXMLNode * l_pModelNode = XML::createNode("Model");
	{
		l_pTempNode = XML::createNode("nr_class");
		std::stringstream l_sModelNrClass;
		l_sModelNrClass << m_pModel->nr_class;
		l_pTempNode->setPCData(l_sModelNrClass.str().c_str());
		l_pModelNode->addChild(l_pTempNode);

		l_pTempNode = XML::createNode("total_sv");
		std::stringstream l_sModelTotalSV;
		l_sModelTotalSV << m_pModel->l;
		l_pTempNode->setPCData(l_sModelTotalSV.str().c_str());
		l_pModelNode->addChild(l_pTempNode);

		l_pTempNode = XML::createNode("rho");
		l_pTempNode->setPCData(l_sRho.str().c_str());
		l_pModelNode->addChild(l_pTempNode);

		if(m_pModel->label)
		{
			std::stringstream l_sLabel;
			l_sLabel << m_pModel->label[0];
			for(int i=1;i<m_pModel->nr_class;i++)
			{
				l_sLabel << " " << m_pModel->label[i];
			}

			l_pTempNode = XML::createNode("label");
			l_pTempNode->setPCData(l_sLabel.str().c_str());
			l_pModelNode->addChild(l_pTempNode);
		}
		if(m_pModel->probA)
		{
			std::stringstream l_sProbA;
			for(int i=1;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
			{
				l_sProbA << " " << m_pModel->probA[i];
			}
			l_sProbA << std::scientific << m_pModel->probA[0];

			l_pTempNode = XML::createNode("probA");
			l_pTempNode->setPCData(l_sProbA.str().c_str());
			l_pModelNode->addChild(l_pTempNode);
		}
		if(m_pModel->probB)
		{
			std::stringstream l_sProbB;
			for(int i=1;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
			{
				 l_sProbB << " " << m_pModel->probB[i];
			}
			l_sProbB << std::scientific << m_pModel->probB[0];

			l_pTempNode = XML::createNode("probB");
			l_pTempNode->setPCData(l_sProbB.str().c_str());
			l_pModelNode->addChild(l_pTempNode);
		}
		if(m_pModel->nSV)
		{
			std::stringstream l_sNrSV;
			l_sNrSV << m_pModel->nSV[0];
			for(int i=1;i<m_pModel->nr_class;i++)
			{
				l_sNrSV << " " << m_pModel->nSV[i];
			}

			l_pTempNode = XML::createNode("nr_sv");
			l_pTempNode->setPCData(l_sNrSV.str().c_str());
			l_pModelNode->addChild(l_pTempNode);
		}

		XML::IXMLNode * l_pSVsNode = XML::createNode("SVs");
		{
			for(int i=0;i<m_pModel->l;i++)
			{
				XML::IXMLNode * l_pSVNode = XML::createNode("SV");
				{
					l_pTempNode = XML::createNode("coef");
					l_pTempNode->setPCData(l_vSVCoef[i]);
					l_pSVNode->addChild(l_pTempNode);

					l_pTempNode = XML::createNode("value");
					l_pTempNode->setPCData(l_vSVValue[i]);
					l_pSVNode->addChild(l_pTempNode);
				}
				l_pSVsNode->addChild(l_pSVNode);
			}
		}
		l_pModelNode->addChild(l_pSVsNode);
	}
	l_pSVMNode->addChild(l_pModelNode);
	m_pConfigurationNode->addChild(l_pSVMNode);

	for(int i=0;i<l_oProb.l;i++)
	{
		delete[] l_oProb.x[i];
	}
	delete[] l_oProb.y;
	delete[] l_oProb.x;
	l_oProb.y=NULL;
	l_oProb.x=NULL;
	delete m_pModel;
	m_pModel=NULL;
	return true;
}

boolean CAlgorithmClassifierSVM::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	//std::cout<<"classify"<<std::endl;
	if(m_pModel==NULL)
	{
		this->getLogManager() << LogLevel_Error << "classify impossible with a model equal NULL\n";
		return false;
	}
	if(m_pModel->nr_class==0||m_pModel->rho==NULL)
	{
		this->getLogManager() << LogLevel_Error << "the model wasn't load correctly\n";
		return false;
	}
	//std::cout<<"create l_pX"<<std::endl;
	svm_node* l_pX=new svm_node[rFeatureVector.getSize()+1];
	//std::cout<<"rFeatureVector.getSize():"<<rFeatureVector.getSize()<<"m_ui32NumberOfFeatures"<<m_ui32NumberOfFeatures<<std::endl;
	for(unsigned int i=0;i<rFeatureVector.getSize();i++)
	{
		l_pX[i].index=i;
		l_pX[i].value=rFeatureVector.getBuffer()[i];
		//std::cout<< l_pX[i].index << ";"<<l_pX[i].value<<" ";
	}
	l_pX[rFeatureVector.getSize()].index=-1;

	//std::cout<<"create l_pProbEstimates"<<std::endl;
	double *l_pProbEstimates=NULL;
	l_pProbEstimates = new double[m_pModel->nr_class];
	for(int i=0;i<m_pModel->nr_class;i++)
	{
		l_pProbEstimates[i]=0;
	}

	rf64Class=svm_predict_probability(m_pModel,l_pX,l_pProbEstimates);
	//std::cout<<rf64Class<<std::endl;
	//std::cout<<"probability"<<std::endl;
	this->getLogManager() << LogLevel_Trace <<"Label predict: "<<rf64Class<<"\n";

	for(int i=0;i<m_pModel->nr_class;i++)
	{
		this->getLogManager() << LogLevel_Trace << "index:"<<i<<" label:"<< m_pModel->label[i]<<" probability:"<<l_pProbEstimates[i]<<"\n";
		if( m_pModel->label[i] == 1 )
		{
			rClassificationValues.setSize(1);
			rClassificationValues[0]=1-l_pProbEstimates[i];

		}
	}

	//std::cout<<";"<<rf64Class<<";"<<rClassificationValues[0] <<";"<<l_pProbEstimates[0]<<";"<<l_pProbEstimates[1]<<std::endl;
	//std::cout<<"Label predict "<<rf64Class<< " proba:"<<rClassificationValues[0]<<std::endl;
	//std::cout<<"end classify"<<std::endl;
	delete[] l_pX;
	delete[] l_pProbEstimates;

	return true;
}

uint32 CAlgorithmClassifierSVM::getBestClassification(IMatrix& rFirstClassificationValue, IMatrix& rSecondClassificationValue)
{
    if(rFirstClassificationValue[0]  < rSecondClassificationValue[0] )
        return -1;
    else if(rFirstClassificationValue[0] == rSecondClassificationValue[0])
        return 0;
    else
       return 1;
}

XML::IXMLNode* CAlgorithmClassifierSVM::saveConfiguration(void)
{
	return m_pConfigurationNode;
}

boolean CAlgorithmClassifierSVM::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	if(m_pModel)
	{
		//std::cout<<"delete m_pModel load config"<<std::endl;
		delete m_pModel;
		m_pModel=NULL;
	}
	//std::cout<<"load config"<<std::endl;
	m_pModel=new svm_model();
	m_pModel->rho = NULL;
	m_pModel->probA = NULL;
	m_pModel->probB = NULL;
	m_pModel->label = NULL;
	m_pModel->nSV = NULL;
	m_i32IndexSV=-1;

	loadParamNodeConfiguration(pConfigurationNode->getChild(0)->getChildByName("Param"));
	loadModelNodeConfiguration(pConfigurationNode->getChild(0)->getChildByName("Model"));

	this->getLogManager() << LogLevel_Trace << modelToString();
	return true;
}

void CAlgorithmClassifierSVM::loadParamNodeConfiguration(XML::IXMLNode *pParamNode)
{
	//svm_type
	XML::IXMLNode* l_pTempNode = pParamNode->getChildByName("svm_type");
	for(int i =0; get_svm_type(i);i++)
	{
		if ( strcmp(get_svm_type(i),l_pTempNode->getPCData().c_str())==0)
		{
			m_pModel->param.svm_type=i;
		}
	}
	if(get_svm_type(m_pModel->param.svm_type) == NULL)
	{
		this->getLogManager() << LogLevel_Error << "load configuration error: bad value for the parameter svm_type\n";
	}

	//kernel_type
	l_pTempNode = pParamNode->getChildByName("kernel_type");
	for(int i =0; get_kernel_type(i);i++)
	{
		if ( strcmp(get_kernel_type(i), l_pTempNode->getPCData().c_str())==0)
		{
			m_pModel->param.kernel_type=i;
		}
	}
	if(get_kernel_type(m_pModel->param.kernel_type) == NULL)
	{
		this->getLogManager() << LogLevel_Error << "load configuration error: bad value for the parameter kernel_type\n";
	}

	//Following parameters aren't required

	//degree
	l_pTempNode = pParamNode->getChildByName("degree");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		l_sData >> m_pModel->param.degree;
	}

	//gamma
	l_pTempNode = pParamNode->getChildByName("gamma");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		l_sData >> m_pModel->param.gamma;
	}

	//coef0
	l_pTempNode = pParamNode->getChildByName("coef0");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		l_sData >> m_pModel->param.coef0;
	}
}

void CAlgorithmClassifierSVM::loadModelNodeConfiguration(XML::IXMLNode *pModelNode)
{

	//nr_class
	XML::IXMLNode* l_pTempNode = pModelNode->getChildByName("nr_class");
	std::stringstream l_sModelNrClass(l_pTempNode->getPCData());
	l_sModelNrClass >> m_pModel->nr_class;
	//total_sv
	l_pTempNode = pModelNode->getChildByName("total_sv");
	std::stringstream l_sModelTotalSv(l_pTempNode->getPCData());
	l_sModelTotalSv >> m_pModel->l;
	//rho
	l_pTempNode = pModelNode->getChildByName("rho");
	std::stringstream l_sModelRho(l_pTempNode->getPCData());
	m_pModel->rho = new double[m_pModel->nr_class*(m_pModel->nr_class-1)/2];
	for(int i=0;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
	{
		l_sModelRho >> m_pModel->rho[i];
	}

	//label
	l_pTempNode = pModelNode->getChildByName("label");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		m_pModel->label = new int[m_pModel->nr_class];
		for(int i=0;i<m_pModel->nr_class;i++)
		{
			l_sData >> m_pModel->label[i];
		}
	}
	//probA
	l_pTempNode = pModelNode->getChildByName("probA");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		m_pModel->probA = new double[m_pModel->nr_class*(m_pModel->nr_class-1)/2];
		for(int i=0;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
		{
			l_sData >> m_pModel->probA[i];
		}
	}
	//probB
	l_pTempNode = pModelNode->getChildByName("probB");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		m_pModel->probB = new double[m_pModel->nr_class*(m_pModel->nr_class-1)/2];
		for(int i=0;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
		{
			l_sData >> m_pModel->probB[i];
		}
	}
	//nr_sv
	l_pTempNode = pModelNode->getChildByName("nr_sv");
	if(l_pTempNode != NULL)
	{
		std::stringstream l_sData(l_pTempNode->getPCData());
		m_pModel->nSV = new int[m_pModel->nr_class];
		for(int i=0;i<m_pModel->nr_class;i++)
		{
			l_sData >> m_pModel->nSV[i];
		}
	}

	loadModelSVsNodeConfiguration(pModelNode->getChildByName("SVs"));
}

void CAlgorithmClassifierSVM::loadModelSVsNodeConfiguration(XML::IXMLNode *pSVsNodeParam)
{
	//Reserve all memory space required
	m_pModel->sv_coef = new double*[m_pModel->nr_class-1];
	for(int i=0;i<m_pModel->nr_class-1;i++)
	{
		m_pModel->sv_coef[i]=new double[m_pModel->l];
	}
	m_pModel->SV = new svm_node*[m_pModel->l];

	//Now fill SV
	for(uint32 i = 0; i < pSVsNodeParam->getChildCount(); ++i)
	{
		XML::IXMLNode *l_pTempNode = pSVsNodeParam->getChild(i);
		std::stringstream l_sCoefData(l_pTempNode->getChildByName("coef")->getPCData());
		for(int j=0;j<m_pModel->nr_class-1;j++)
		{
			l_sCoefData >> m_pModel->sv_coef[j][i];
		}

		std::stringstream l_sValueData(l_pTempNode->getChildByName("value")->getPCData());
		std::vector <int> l_vSVMIndex;
		std::vector <double> l_vSVMValue;
		char l_cSeparateChar;
		while(!l_sValueData.eof())
		{
			int l_iIndex;
			double l_dValue;
			l_sValueData >> l_iIndex;
			l_sValueData >> l_cSeparateChar;
			l_sValueData >> l_dValue;
			l_vSVMIndex.push_back(l_iIndex);
			l_vSVMValue.push_back(l_dValue);
		}

		m_ui32NumberOfFeatures=l_vSVMIndex.size();
		m_pModel->SV[i] = new svm_node[l_vSVMIndex.size()+1];
		for(size_t j=0;j<l_vSVMIndex.size();j++)
		{
			m_pModel->SV[i][j].index=l_vSVMIndex[j];
			m_pModel->SV[i][j].value=l_vSVMValue[j];
		}
		m_pModel->SV[i][l_vSVMIndex.size()].index=-1;
	}
}

CString CAlgorithmClassifierSVM::paramToString(svm_parameter *pParam)
{
	std::stringstream l_sParam;
	if(pParam == NULL)
	{
		l_sParam << "Param: NULL\n";
		return CString(l_sParam.str().c_str());
	}
	l_sParam << "Param:\n";
	l_sParam << "\tsvm_type: "<<get_svm_type(pParam->svm_type)<<"\n";
	l_sParam << "\tkernel_type: "<<get_kernel_type(pParam->kernel_type)<<"\n";
	l_sParam << "\tdegree: "<<pParam->degree<<"\n";
	l_sParam << "\tgamma: "<<pParam->gamma<<"\n";
	l_sParam << "\tcoef0: "<<pParam->coef0<<"\n";
	l_sParam << "\tnu: "<<pParam->nu << "\n";
	l_sParam << "\tcache_size: "<<pParam->cache_size << "\n";
	l_sParam << "\tC: "<<pParam->C << "\n";
	l_sParam << "\teps: "<<pParam->eps << "\n";
	l_sParam << "\tp: "<<pParam->p << "\n";
	l_sParam << "\tshrinking: "<<pParam->shrinking << "\n";
	l_sParam << "\tprobability: "<<pParam->probability << "\n";
	l_sParam << "\tnr weight: "<<pParam->nr_weight << "\n";
	std::stringstream l_sWeightLabel;
	for(int i=0;i<pParam->nr_weight;i++)
	{
		l_sWeightLabel<<pParam->weight_label[i]<<";";
	}
	l_sParam << "\tweight label: "<<l_sWeightLabel.str().c_str()<< "\n";
	std::stringstream l_sWeight;
	for(int i=0;i<pParam->nr_weight;i++)
	{
		l_sWeight<<pParam->weight[i]<<";";
	}
	l_sParam << "\tweight: "<<l_sWeight.str().c_str()<< "\n";
	return CString(l_sParam.str().c_str());
}
CString CAlgorithmClassifierSVM::modelToString()
{
	std::stringstream l_sModel;
	if(m_pModel==NULL)
	{
		l_sModel << "Model: NULL\n";
		return CString(l_sModel.str().c_str());
	}
	l_sModel << paramToString(&m_pModel->param);
	l_sModel << "Model:" << "\n";
	l_sModel << "\tnr_class: "<< m_pModel->nr_class <<"\n";
	l_sModel << "\ttotal_sv: "<<m_pModel->l<<"\n";
	l_sModel << "\trho: ";
	if(m_pModel->rho)
	{
		l_sModel << m_pModel->rho[0];
		for(int i=1;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
		{
			l_sModel << " "<<m_pModel->rho[i];
		}
	}
	l_sModel << "\n";
	l_sModel << "\tlabel: ";
	if(m_pModel->label)
	{
		l_sModel << m_pModel->label[0];
		for(int i=1;i<m_pModel->nr_class;i++)
		{
			l_sModel << " "<<m_pModel->label[i];
		}
	}
	l_sModel << "\n";
	l_sModel << "\tprobA: ";
	if(m_pModel->probA)
	{
		l_sModel << m_pModel->probA[0];
		for(int i=1;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
		{
			l_sModel << " "<<m_pModel->probA[i];
		}
	}
	l_sModel << "\n";
	l_sModel << "\tprobB: ";
	if(m_pModel->probB)
	{
		l_sModel << m_pModel->probB[0];
		for(int i=1;i<m_pModel->nr_class*(m_pModel->nr_class-1)/2;i++)
		{
			l_sModel << " "<<m_pModel->probB[i];
		}
	}
	l_sModel << "\n";
	l_sModel << "\tnr_sv: ";
	if(m_pModel->nSV)
	{
		l_sModel << m_pModel->nSV[0];
		for(int i=1;i<m_pModel->nr_class;i++)
		{
			l_sModel << " "<<m_pModel->nSV[i];
		}
	}
	l_sModel << "\n";
	/*l_sModel << "\tSV: "<<"\n";
	if(m_pModel->sv_coef !=NULL && m_pModel->SV != NULL)
	{
		for(int i=0;i<m_pModel->l;i++)
		{
			l_sModel << "\t("<<i<<")\tcoef: "<<m_pModel->sv_coef[0][i];
			for(int j=1;j<m_pModel->nr_class-1;j++)
			{
				l_sModel << " "<<m_pModel->sv_coef[j][i];
			}
			l_sModel << "\n";
			l_sModel << "\t("<<i<<")\tvalue: "<<m_pModel->SV[i][0].index<<":"<<m_pModel->SV[i][0].value;
			for(int j=1;j<m_ui32NumberOfFeatures+1;j++)
			{
				l_sModel << " "<<m_pModel->SV[i][j].index<<":"<<m_pModel->SV[i][0].value;
			}
			l_sModel << "\n";
		}
	}*/
	return CString(l_sModel.str().c_str());
}
CString CAlgorithmClassifierSVM::problemToString(svm_problem *pProb)
{
	std::stringstream l_sProb;
	if(pProb == NULL)
	{
		l_sProb << "Problem: NULL\n";
		return CString(l_sProb.str().c_str());
	}
	l_sProb << "Problem";
	l_sProb << "\ttotal sv: "<<pProb->l<<"\n";
	l_sProb << "\tnb features: " << m_ui32NumberOfFeatures << "\n";
	/*for(int i=0;i<pProb->l;i++)
	{
		l_sProb << "\t("<<i<<")\ty = " << pProb->y[i]<<": ";
		for(uint32 j=0;j<m_ui32NumberOfFeatures+1;j++)
		{
			l_sProb << " " <<pProb->x[i][j].index <<";"<<pProb->x[i][j].value;
		}
		l_sProb<<"\n";
	}*/
	return CString(l_sProb.str().c_str());
}
