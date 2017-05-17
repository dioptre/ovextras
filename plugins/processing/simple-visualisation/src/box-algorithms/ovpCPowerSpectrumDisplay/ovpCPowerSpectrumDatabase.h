#ifndef __OpenViBEPlugins_SimpleVisualisation_CPowerSpectrumDatabase_H__
#define __OpenViBEPlugins_SimpleVisualisation_CPowerSpectrumDatabase_H__

#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <deque>
#include <queue>
#include <string>
#include <cfloat>
#include <iostream>

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		class CSignalDisplayDrawable;

		/**
		* This class is used to store information about the incoming spectrum stream. It can request a CSignalDisplayDrawable
		* object to redraw himself in case of some changes in its data.
		*/
		class CPowerSpectrumDatabase
		{
		public:
			/**
			 * \brief Constructor
			 * \param oPlugin Reference to parent plugin
			 */
			CPowerSpectrumDatabase(
				OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>& oPlugin);

			/**
			 * \brief Destructor
			 */
			~CPowerSpectrumDatabase();

			/**
			* \brief Set the drawable object to update.
			* \param pDrawable drawable object to update.
			*/
			void setDrawable(CSignalDisplayDrawable* pDrawable)
			{
				m_pDrawable=pDrawable;
			}

			/**
			 * \name IBoxAlgorithmSpectrumInputReaderCallback::ICallback callbacks implementation
			 */
			//@{

			/**
			 * \brief Set number of channels
			 */
			void setChannelCount(
				const uint32_t ui32ChannelCount);

			/**
			 * \brief Set name of a channel
			 * \param ui32ChannelIndex Index of channel
			 * \param sChannelName Channel name
			 */
			void setChannelName(
				const uint32_t ui32ChannelIndex,
				const char* sChannelName);

			/**
			 * \brief Set frequency band count
			 * \param ui32FrequencyBandCount Number of frequency bands
			 */
			void setFrequencyAbscissaCount(const uint32_t ui32FrequencyAbscissaCount);

			/**
			 * \brief Set name of a frequency band
			 * \param ui32FrequencyBandIndex Index of frequency band
			 * \param sFrequencyBandName Frequency band name
			 */
			void setFrequencyAbscissaName(
				const uint32_t ui32FrequencyAbscissaIndex,
				const char* sFrequencyAbscissaName);

			/**
			 * \brief Set frequency band start frequency
			 * \param ui32FrequencyBandIndex Index of frequency band
			 * \param f64FrequencyBandStart Start frequency
			 */
			void setFrequencyAbscissaValue(const uint32_t ui32FrequencyAbscissaIndex,
				const OpenViBE::float64 f64FrequencyAbscissaValue);

			/**
			 * \brief Set data buffer
			 * \param pbuffer Pointer to data buffer
			 */
			void setBuffer(const OpenViBE::float64* pBuffer);

			//@}

			/**
			 * \name Buffer data management
			 */
			//@{

			/**
			 * \brief Get min/max values of last buffer for a given channel
			 * \param [in] ui32Channel Index of channel
			 * \param [out] f64Min Minimum value for a given channel
			 * \param [out] f64Min Maximum value for a given channel
			 * \return True if values could be retrieved, false otherwise
			 */
			bool getLastBufferChannelMinMaxValue(
				uint32_t ui32Channel,
				OpenViBE::float64& f64Min,
				OpenViBE::float64& f64Max);

			/**
			 * \brief Get min/max values of last buffer for all channels
			 * \param [out] f64Min Minimum value found in last buffer
			 * \param [out] f64Max Maximum value found in last buffer
			 * \return True if values could be retrieved, false otherwise
			 */
			bool getLastBufferMinMaxValue(
				OpenViBE::float64& f64Min,
				OpenViBE::float64& f64Max);

			/**
			 * \brief Get pointer to last buffer data for a given channel
			 * \param [in] ui32Channel Index of channel which data is to be retrieved
			 * \return Pointer to buffer data if it exists, NULL otherwise
			 */
			OpenViBE::float64* getLastBufferChannelPointer(
				uint32_t ui32Channel);

			//@}

			/**
			 * \name Channels and frequency bands management
			 */
			//@{

			/**
			 * \brief Get number of channels
			 * \return Number of channels
			 */
			uint32_t getChannelCount()
			{
				return m_pChannelLabels.size();
			}

			/**
			 * \brief Get the label of a channel
			 * \param [in] ui32ChannelIndex index of channel which label is to be retrieved
			 * \param [out] rChannelLabel channel label to be retrieved
			 * \return True if channel label could be retrieved, false otherwise
			 */
			bool getChannelLabel(
				uint32_t ui32ChannelIndex,
				OpenViBE::CString& rChannelLabel);

			/**
			 * \brief Set the minimum frequency displayed by the power spectrum plugin
			 * \remarks This frequency should lie in the range of frequencies received from data
			 * buffers, and it is taken into account when computing min/max amplitude values. Also,
			 * this function should be called after header information has been received, e.g. upon
			 * first buffer reception.
			 * \param [in] f64MinDisplayedFrequency Minimum displayed frequency
			 * \return True if frequency was successfully set, false otherwise
			 */
			bool setMinDisplayedFrequency(
				OpenViBE::float64 f64MinDisplayedFrequency);

			/**
			 * \brief Set the maximum frequency displayed by the power spectrum plugin
			 * \remarks This frequency should lie in the range of frequencies received from data
			 * buffers, and it is taken into account when computing min/max amplitude values. Also,
			 * this function should be called after header information has been received, e.g. upon
			 * first buffer reception.
			 * \param [in] f64MaxDisplayedFrequency Maximum displayed frequency
			 * \return True if frequency was successfully set, false otherwise
			 */
			bool setMaxDisplayedFrequency(
				OpenViBE::float64 f64MaxDisplayedFrequency);

			/**
			 * \brief Get range of frequencies received from incoming buffers
			 * \param [out] f64MinInputFrequency Minimum input frequency
			 * \param [out] f64MaxInputFrequency Maximum input frequency
			 * \return True if input frequency range, false otherwise
			 */
			bool getInputFrequencyRange(
				OpenViBE::float64& f64MinInputFrequency,
				OpenViBE::float64& f64MaxInputFrequency);

			/**
			 * \brief Get number of frequency bands displayed by power spectrum plugin
			 * \return number of frequency bands displayed
			 */
			uint32_t getDisplayedFrequencyAbscissaCount()
			{
				if(m_MaxDisplayedFrequencyAbscissa < m_MinDisplayedFrequencyAbscissa)
				{
					return 0;
				}
				return m_MaxDisplayedFrequencyAbscissa - m_MinDisplayedFrequencyAbscissa + 1;
			}

			/**
			 * \brief Get index of minimum frequency band displayed by power spectrum plugin
			 * \return Index of minimum frequency band displayed by power spectrum plugin
			 */
			uint32_t getMinDisplayedFrequencyAbscissaIndex()
			{
				return m_MinDisplayedFrequencyAbscissa;
			}

			/**
			 * \brief Get index of maximum frequency band displayed by power spectrum plugin
			 * \return Index of maximum frequency band displayed by power spectrum plugin
			 */
			uint32_t getMaxDisplayedFrequencyAbscissaIndex()
			{
				return m_MaxDisplayedFrequencyAbscissa;
			}
			//@}

		private:
			// Parent plugin
			OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>& m_oParentPlugin;

			// Pointer to the drawable object to update
			CSignalDisplayDrawable* m_pDrawable;

			/**
			 * \name Buffer data
			 */
			//@{

			// Pointer to last buffer received
			OpenViBE::float64* m_pBuffer;
			// Flag set to true upon first buffer reception
			bool m_bFirstBufferReceived;

			//@}

			/**
			 * \name Channels and frequency bands data
			 */
			//@{

			// Vector of channel labels
			std::vector<std::string> m_pChannelLabels;
			// Vector of frequency abscissa labels
			std::vector<std::string> m_pFrequencyAbscissaLabels;
			// Vector of frequency abscissa values
			std::vector<OpenViBE::float64> m_pFrequencyAbscissa;
			// Index of minimum frequency abscissa displayed in power spectrum plugin
			uint32_t m_MinDisplayedFrequencyAbscissa;
			// Index of maximum frequency abscissa displayed in power spectrum plugin
			uint32_t m_MaxDisplayedFrequencyAbscissa;
			// Min/max displayed values per channel for last buffer
			std::vector<std::pair<OpenViBE::float64, OpenViBE::float64> > m_oMinMaxDisplayedValues;
			// Minimum displayed value for last buffer
			OpenViBE::float64 m_f64MinDisplayedValue;
			// Maximum displayed value for last buffer
			OpenViBE::float64 m_f64MaxDisplayedValue;

			//@}
		};
	}

}

#endif
