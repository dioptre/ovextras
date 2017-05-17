#include "ovpCPowerSpectrumDatabase.h"
#include "../../ovpCBufferDatabase.h"

#include <system/ovCMemory.h>

#include <algorithm>

#include <cmath>

using namespace OpenViBE;
using namespace Plugins;
using namespace Kernel;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

using namespace OpenViBEToolkit;

using namespace std;

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{

		CPowerSpectrumDatabase::CPowerSpectrumDatabase(OpenViBEToolkit::TBoxAlgorithm<Plugins::IBoxAlgorithm>& oPlugin) :
			m_oParentPlugin(oPlugin),
			m_pDrawable(nullptr),
			m_pBuffer(nullptr),
			m_bFirstBufferReceived(false),
			m_MinDisplayedFrequencyAbscissa(0),
			m_MaxDisplayedFrequencyAbscissa(0),
			m_f64MinDisplayedValue(+DBL_MAX),
			m_f64MaxDisplayedValue(-DBL_MAX)
		{
		}

		CPowerSpectrumDatabase::~CPowerSpectrumDatabase()
		{
			if(m_pBuffer)
			{
				delete[] m_pBuffer;
			}
		}

		void CPowerSpectrumDatabase::setChannelCount(const uint32_t ui32ChannelCount)
		{
			m_pChannelLabels.resize(ui32ChannelCount);
			m_oMinMaxDisplayedValues.resize(ui32ChannelCount);
		}

		void CPowerSpectrumDatabase::setChannelName(const uint32_t ui32ChannelIndex, const char* sChannelName)
		{
			if(ui32ChannelIndex >= m_pChannelLabels.size())
				m_pChannelLabels.resize(ui32ChannelIndex+1);

			m_pChannelLabels[ui32ChannelIndex] = sChannelName;
		}

		void CPowerSpectrumDatabase::setFrequencyAbscissaCount(const uint32_t ui32FrequencyAbscissaCount)
		{
			m_pFrequencyAbscissaLabels.resize(ui32FrequencyAbscissaCount);
			m_pFrequencyAbscissa.resize(ui32FrequencyAbscissaCount);
		}

		void CPowerSpectrumDatabase::setFrequencyAbscissaName(const uint32_t ui32FrequencyAbscissaIndex, const char* sFrequencyAbscissaName)
		{
			if(ui32FrequencyAbscissaIndex >= m_pFrequencyAbscissaLabels.size())
				m_pFrequencyAbscissaLabels.resize(ui32FrequencyAbscissaIndex+1);

			m_pFrequencyAbscissaLabels[ui32FrequencyAbscissaIndex] = sFrequencyAbscissaName;
		}

		void CPowerSpectrumDatabase::setFrequencyAbscissaValue(const uint32_t ui32FrequencyAbscissaIndex, const float64 f64FrequencyAbscissaValue)
		{
			if(ui32FrequencyAbscissaIndex >= m_pFrequencyAbscissaLabels.size())
				m_pFrequencyAbscissa.resize(ui32FrequencyAbscissaIndex+1);

			m_pFrequencyAbscissa[ui32FrequencyAbscissaIndex] = f64FrequencyAbscissaValue;
		}

		void CPowerSpectrumDatabase::setBuffer(const float64* pBuffer)
		{
			//init some members when receiving first buffer
			if(m_bFirstBufferReceived == false)
			{
				//initialize displayed frequency bands
				if(m_pFrequencyAbscissa.size() > 0)
				{
					m_MinDisplayedFrequencyAbscissa = 0;
					m_MaxDisplayedFrequencyAbscissa = m_pFrequencyAbscissa.size()-1;
				}

				m_pDrawable->init();

				m_pBuffer = new float64[(size_t)m_pChannelLabels.size()*m_pFrequencyAbscissaLabels.size()];

				m_bFirstBufferReceived = true;
			}

			System::Memory::copy(m_pBuffer, pBuffer, m_pChannelLabels.size()*m_pFrequencyAbscissaLabels.size()*sizeof(float64));

			const float64* pCurChannel = pBuffer;

			m_f64MinDisplayedValue = 0;
			m_f64MaxDisplayedValue = 0;

			//for each channel
			for(uint32_t c=0; c<m_pChannelLabels.size(); c++, pCurChannel += m_pFrequencyAbscissa.size())
			{
				m_oMinMaxDisplayedValues[c].first = 0;
				m_oMinMaxDisplayedValues[c].second = 0;

				//for each displayed frequency band
				for(uint64 i=m_MinDisplayedFrequencyAbscissa; i<m_MaxDisplayedFrequencyAbscissa; i++)
				{
					//update channel min/max values
					if(pCurChannel[i] < m_oMinMaxDisplayedValues[c].first)
					{
						m_oMinMaxDisplayedValues[c].first = pCurChannel[i];
					}
					else if(pCurChannel[i] > m_oMinMaxDisplayedValues[c].second)
					{
						m_oMinMaxDisplayedValues[c].second = pCurChannel[i];
					}
				}

				//update global min/max values
				if(m_oMinMaxDisplayedValues[c].first < m_f64MinDisplayedValue)
				{
					m_f64MinDisplayedValue = m_oMinMaxDisplayedValues[c].first;
				}
				if(m_oMinMaxDisplayedValues[c].second > m_f64MaxDisplayedValue)
				{
					m_f64MaxDisplayedValue = m_oMinMaxDisplayedValues[c].second;
				}
			}

			//tells the drawable to redraw himself since the signal information has been updated
			m_pDrawable->redraw();
		}


		bool CPowerSpectrumDatabase::getChannelLabel(uint32_t ui32ChannelIndex, CString& rChannelLabel)
		{
			if(ui32ChannelIndex >= m_pChannelLabels.size())
			{
				rChannelLabel = "";
				return false;
			}
			else
			{
				rChannelLabel = m_pChannelLabels[ui32ChannelIndex].c_str();
				return true;
			}
		}

		bool CPowerSpectrumDatabase::setMinDisplayedFrequency(float64 f64MinDisplayedFrequency)
		{
			if(m_pFrequencyAbscissa.size() == 0)
			{
				return false;
			}

			if(f64MinDisplayedFrequency < m_pFrequencyAbscissa[0])
			{
				m_MinDisplayedFrequencyAbscissa = 0;
			}
			else
			{
				m_MinDisplayedFrequencyAbscissa = std::distance(m_pFrequencyAbscissa.begin(),
					std::upper_bound(m_pFrequencyAbscissa.begin(), m_pFrequencyAbscissa.end(), f64MinDisplayedFrequency));
			}
			return true;
		}

		bool CPowerSpectrumDatabase::setMaxDisplayedFrequency(float64 f64MaxDisplayedFrequency)
		{
			if(m_pFrequencyAbscissa.size() == 0)
			{
				return false;
			}

			if(f64MaxDisplayedFrequency > m_pFrequencyAbscissa.back())
			{
				m_MaxDisplayedFrequencyAbscissa = m_pFrequencyAbscissa.size()-1;
			}
			else
			{
				m_MaxDisplayedFrequencyAbscissa = std::distance(m_pFrequencyAbscissa.begin(),
					std::upper_bound(m_pFrequencyAbscissa.begin(), m_pFrequencyAbscissa.end(), f64MaxDisplayedFrequency, [](OpenViBE::float64 a, OpenViBE::float64 b) -> bool { return a <= b;}));

			}
			return true;
		}

		bool CPowerSpectrumDatabase::getInputFrequencyRange(float64& f64MinInputFrequency, float64& f64MaxInputFrequency)
		{
			if(m_pFrequencyAbscissa.size() == 0)
			{
				return false;
			}
			f64MinInputFrequency = m_pFrequencyAbscissa.front();
			f64MaxInputFrequency = m_pFrequencyAbscissa.back();
			return true;
		}

		bool CPowerSpectrumDatabase::getLastBufferChannelMinMaxValue(uint32_t ui32Channel, float64& f64Min, float64& f64Max)
		{
			if(ui32Channel >= m_oMinMaxDisplayedValues.size())
			{
				return false;
			}
			f64Min = m_oMinMaxDisplayedValues[ui32Channel].first;
			f64Max = m_oMinMaxDisplayedValues[ui32Channel].second;
			return true;
		}

		bool CPowerSpectrumDatabase::getLastBufferMinMaxValue(float64& f64Min, float64& f64Max)
		{
			if(m_bFirstBufferReceived == false)
			{
				return false;
			}
			f64Min = m_f64MinDisplayedValue;
			f64Max = m_f64MaxDisplayedValue;
			return true;
		}

		float64* CPowerSpectrumDatabase::getLastBufferChannelPointer(uint32_t ui32Channel)
		{
			if(m_pBuffer == NULL)
			{
				return NULL;
			}
			else
			{
				return m_pBuffer + ui32Channel * m_pFrequencyAbscissa.size();
			}
		}
/*
		void CPowerSpectrumDatabase::getDisplayedChannelMinMaxValue(uint32 ui32Channel, float64& f64Min, float64& f64Max)
		{
			f64Min = +DBL_MAX;
			f64Max = -DBL_MAX;

			for(uint64 i=0 ; i<m_oLocalMinMaxValue[ui32Channel].size() ; i++)
			{
				if(f64Min > m_oLocalMinMaxValue[ui32Channel][i].first)
				{
					f64Min = m_oLocalMinMaxValue[ui32Channel][i].first;
				}
				if(f64Max < m_oLocalMinMaxValue[ui32Channel][i].second)
				{
					f64Max = m_oLocalMinMaxValue[ui32Channel][i].second;
				}
			}
		}

		void CPowerSpectrumDatabase::getDisplayedGlobalMinMaxValue(float64& f64Min, float64& f64Max)
		{
			for(uint32 c=0 ; c<m_oLocalMinMaxValue.size() ; c++)
			{
				for(uint64 i=0 ; i<m_oLocalMinMaxValue[c].size() ; i++)
				{
					if(f64Min > m_oLocalMinMaxValue[c][i].first)
					{
						f64Min = m_oLocalMinMaxValue[c][i].first;
					}
					if(f64Max < m_oLocalMinMaxValue[c][i].second)
					{
						f64Max = m_oLocalMinMaxValue[c][i].second;
					}
				}
			}
		}
*/
	}
}
