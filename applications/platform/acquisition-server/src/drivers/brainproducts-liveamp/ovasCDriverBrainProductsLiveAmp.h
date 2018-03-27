#ifndef __OpenViBE_AcquisitionServer_CDriverBrainProductsLiveAmp_H__
#define __OpenViBE_AcquisitionServer_CDriverBrainProductsLiveAmp_H__

#if defined TARGET_HAS_ThirdPartyLiveAmpAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <deque>

#ifndef BYTE 
typedef unsigned char       BYTE;
#endif

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverBrainProductsLiveAmp
	 * \author Ratko Petrovic (Brain Products GmbH)
	 * \date Mon Nov 21 14:57:37 2016
	 * \brief The CDriverBrainProductsLiveAmp allows the acquisition server to acquire data from a Brain Products LiveAmp device.
	 *
	 * TODO: details
	 *
	 * \sa CConfigurationBrainProductsLiveAmp
	 */
	class CDriverBrainProductsLiveAmp : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverBrainProductsLiveAmp(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDriverBrainProductsLiveAmp(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);

		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void) { return &m_oHeader; }
		
		virtual OpenViBE::boolean isFlagSet(
			const OpenViBEAcquisitionServer::EDriverFlag eFlag) const
		{
			if (eFlag == DriverFlag_IsUnstable) 
			{
				return false;  // switch to "Stable" on 3.5.2017 - RP
			}
			return false; // No flags are set
		}

	private:
		
		SettingsHelper m_oSettings;		
		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		void* m_pHandle;
		
		OpenViBEAcquisitionServer::CHeader m_oHeader;
		OpenViBE::CStimulationSet m_oStimulationSet;
		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;		
		OpenViBE::uint8* m_pSampleBuffer;

		OpenViBE::uint32 m_ui32BufferSize;
		OpenViBE::uint32 m_ui32SampleSize;
		
		OpenViBE::uint32 m_ui32PhysicalSampleRate;
		OpenViBE::boolean m_bUseAccChannels;
		OpenViBE::boolean m_bUseBipolarChannels;
		OpenViBE::uint32 m_ui32ImpedanceChannels;
		OpenViBE::uint32 m_ui32UsedChannelsCounter;
		OpenViBE::uint32 m_ui32EnabledChannels;		

		OpenViBE::uint32 m_ui32ChannelCount;
		OpenViBE::uint32 m_ui32CountEEG;
		OpenViBE::uint32 m_ui32CountAux;
		OpenViBE::uint32 m_ui32CountACC;
		OpenViBE::uint32 m_ui32CountBipolar;
		OpenViBE::uint32 m_ui32CountTriggersIn;
		OpenViBE::uint32 m_ui32GoodImpedanceLimit;
		OpenViBE::uint32 m_ui32BadImpedanceLimit;
		
		std::string m_sSerialNumber;
		OpenViBE::int32 m_iRecordingMode;
			
		std::vector <OpenViBE::float32> m_vSample;
		std::deque <std::vector<OpenViBE::float32>> m_vSampleCache;		
		std::vector<std::vector<OpenViBE::float32>> m_vSendBuffer;
		std::vector<OpenViBE::uint32> m_vSamplingRatesArray; 
		std::vector<OpenViBE::uint16> m_vTriggerIndices;
		std::vector<OpenViBE::uint16> m_vLastTriggerStates;
		std::vector<OpenViBE::uint16> m_vDataTypeArray;
		std::vector<OpenViBE::float32> m_vResolutionArray;
		
		OpenViBE::boolean m_bSimulationMode;
		/**
		* \function initializeLiveAmp
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return True if LiveAmp is successful initialized. 
		* \brief Establish BT connection with the LiveAmp amplifier, identified with the serial number.
		*/
		OpenViBE::boolean initializeLiveAmp(void);

		/**
		* \function configureLiveAmp
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return True if LiveAmp is successful configured. 
		* \brief Configure LiveAmp, sampling rate and mode.
		*/
		OpenViBE::boolean configureLiveAmp(void);

		/**
		* \function checkAvailableChannels
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return True if checking is successful. 
		* \brief Check how many available channels has LiveAmp. Get count of EEG,AUX and ACC channels.
			Compares the count with the amplifier/driver settings.
		*/
		OpenViBE::boolean checkAvailableChannels(void);
		
		/**
		* \function disableAllAvailableChannels
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return True if function is successful. 
		* \brief 
		*/
		OpenViBE::boolean disableAllAvailableChannels(void);
		
		/**
		* \function getChannelIndices
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return True if function is successful.  
		* \brief Get indexes of the channel that will be used for acquisition.
		*/
		OpenViBE::boolean getChannelIndices(void);
		
		/**
		* \function impedanceMessureInit
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return True if function is successful.  
		* \brief Configures impedance measurement. 
		*/
		OpenViBE::boolean configureImpedanceMessure(void);
		
		/**
		* \function getLiveAmpSampleSize
		* \param rInputValue[in] : 
		* \param rOutputValue[out] : 
		* \return Size of 'one' acquired sample. 
		* \brief Calcualtes the size of 'one' sample that LiveAmp delivers. It is actually a sum of the one sample per each channel that will be acquired.
		   This information is needed to read data from buffer, in order to access to each sample of each channel.     
		*/
		OpenViBE::uint32 getLiveAmpSampleSize(void);

		/**
		* \function liveAmpExtractData
		* \param samplesRead[in] : count of samples read (obtained) from the amplifier 
		* \param extractData[out] : extract data from the buffer
		* \return 
		* \brief Extracts acquired data from the buffer, and puts them into the double vector 'extractData' , in the appropriate format.
		*/
		void liveAmpExtractData(OpenViBE::int32 samplesRead, std::vector<std::vector<OpenViBE::float32>> &extractData);
		
		/**
		* \function liveAmpExtractImpedanceData
		* \param samplesRead[in] : count of acquired samples from the amplifier 
		* \param extractData[out] : extract data from the buffer
		* \return 
		* \brief Extracts acquired impedance measurements from the buffer, and puts them into the double vector 'extractData', in the appropriate format. 
		*/
		void liveAmpExtractImpedanceData(OpenViBE::int32 samplesRead, std::vector<std::vector<OpenViBE::float32>> &extractData);
	};
};

#endif // TARGET_HAS_ThirdPartyLiveAmpAPI

#endif // __OpenViBE_AcquisitionServer_CDriverBrainProductsLiveAmp_H__
