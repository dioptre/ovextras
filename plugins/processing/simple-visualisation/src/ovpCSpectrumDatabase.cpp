#include <system/ovCMemory.h>
#include <algorithm>
#include <cmath>

#include "ovpCSpectrumDatabase.h"

using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

using namespace OpenViBEToolkit;

using namespace std;

CSpectrumDatabase::CSpectrumDatabase(OpenViBEToolkit::TBoxAlgorithm<Plugins::IBoxAlgorithm>& oPlugin) :
CStreamedMatrixDatabase(oPlugin)
{
}

CSpectrumDatabase::~CSpectrumDatabase()
{
}

bool CSpectrumDatabase::initialize()
{
	if(m_pDecoder != NULL)
	{
		return false;
	}

	m_pDecoder = &m_oParentPlugin.getAlgorithmManager().getAlgorithm(
		m_oParentPlugin.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumStreamDecoder));

	m_pDecoder->initialize();

	return true;
}

uint32_t CSpectrumDatabase::getFrequencyAbscissaCount()
{
	return m_FrequencyAbscissa.size();
}

//float64 CSpectrumDatabase::getFrequencyBandWidth()
//{
//	if(m_FrequencyAbscissa.size() == 0)
//	{
//		return 0;
//	}
//	else
//	{
//		return m_oFrequencyBands[0].second - m_oFrequencyBands[0].first;
//	}
//}

//float64 CSpectrumDatabase::getFrequencyBandStart(uint32 ui32FrequencyBandIndex)
//{
//	if(m_oFrequencyBands.size() == 0)
//	{
//		return 0;
//	}
//	else
//	{
//		return m_oFrequencyBands[ui32FrequencyBandIndex].first;
//	}
//}

//float64 CSpectrumDatabase::getFrequencyBandStop(uint32 ui32FrequencyBandIndex)
//{
//	if(ui32FrequencyBandIndex >= m_oFrequencyBands.size())
//	{
//		return 0;
//	}
//	else
//	{
//		return m_oFrequencyBands[ui32FrequencyBandIndex].second;
//	}
//}

bool CSpectrumDatabase::decodeHeader()
{
	//retrieve spectrum header
	OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > frequencyAbscissaMatrix;
	frequencyAbscissaMatrix.initialize(m_pDecoder->getOutputParameter(OVP_GD_Algorithm_SpectrumStreamDecoder_OutputParameterId_FrequencyAbscissa));

	//store frequency bands
	for(uint32_t i=0; i< frequencyAbscissaMatrix->getDimensionSize(0); i++)
	{
		m_FrequencyAbscissa.push_back(frequencyAbscissaMatrix->getBuffer()[i]);
	}

	CStreamedMatrixDatabase::decodeHeader();

	return true;
}
