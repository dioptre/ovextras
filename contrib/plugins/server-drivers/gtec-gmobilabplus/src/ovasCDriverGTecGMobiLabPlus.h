/**
 * The gMobilab driver was contributed
 * by Lucie Daubigney from Supelec Metz
 */

#ifndef __OpenViBE_AcquisitionServer_CDriverGTecGMobiLabPlus_H__
#define __OpenViBE_AcquisitionServer_CDriverGTecGMobiLabPlus_H__

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"


#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <string>
#ifdef TARGET_OS_Windows
#include <Windows.h>
#endif
#include <gMOBIlabplus.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverGTecGMobiLabPlus
	 * \author Lucie Daubigney (Supelec Metz)
	 */
	class CDriverGTecGMobiLabPlus : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverGTecGMobiLabPlus(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDriverGTecGMobiLabPlus(void);
		virtual void release(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean isFlagSet(
			const OpenViBEAcquisitionServer::EDriverFlag eFlag) const
		{
			return eFlag==DriverFlag_IsUnstable;
		}

		//configuration
		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		//initialisation
		virtual OpenViBE::boolean initialize(const OpenViBE::uint32 ui32SampleCountPerChannel, OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void);

		//acquisition
		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

	protected:

		SettingsHelper m_oSettings;

		//usefull data to communicate with OpenViBE
		OpenViBEAcquisitionServer::IHeader* m_pHeader;
		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;//number of sample you want to send in a row
		OpenViBE::float32* m_pSample;//array containing the data to sent to OpenViBE once they had been recovered from the gTec module

		//params
		std::string m_oPortName;
		OpenViBE::boolean m_bTestMode;

		//usefull data to communicate with the gTec module
		_BUFFER_ST m_oBuffer;
		HANDLE m_oDevice;
		_AIN m_oAnalogIn;

#if defined(TARGET_OS_Windows)
		OVERLAPPED m_oOverlap;
#endif

	private:

		void allowAnalogInputs(OpenViBE::uint32 ui32ChannelIndex);

		// Register the function pointers from the dll. (The dll approach
		// is used with gMobilab to avoid conflicts with the gUSBAmp lib)
		OpenViBE::boolean registerLibraryFunctions(void);
		
		// These gtec function calls are found from the dll library
		typedef HANDLE(__stdcall *OV_GT_OpenDevice)(LPSTR lpPort);
		typedef BOOL(__stdcall *OV_GT_CloseDevice)(HANDLE hDevice);
		typedef BOOL(__stdcall *OV_GT_SetTestmode)(HANDLE hDevice, BOOL Testmode);
		typedef BOOL(__stdcall *OV_GT_StartAcquisition)(HANDLE hDevice);
		typedef BOOL(__stdcall *OV_GT_GetData)(HANDLE hDevice, _BUFFER_ST *buffer, LPOVERLAPPED lpOvl);
		typedef BOOL(__stdcall *OV_GT_InitChannels)(HANDLE hDevice, _AIN analogCh, _DIO digitalCh);
		typedef	BOOL(__stdcall *OV_GT_StopAcquisition)(HANDLE hDevice);
		typedef BOOL(__stdcall *OV_GT_GetLastError)(UINT * LastError);
		typedef BOOL(__stdcall *OV_GT_TranslateErrorCode)(_ERRSTR *ErrorString, UINT ErrorCode);

		OV_GT_OpenDevice m_fOpenDevice;
		OV_GT_CloseDevice m_fCloseDevice;
		OV_GT_SetTestmode m_fSetTestmode;
		OV_GT_StartAcquisition m_fStartAcquisition;
		OV_GT_GetData m_fGetData;
		OV_GT_InitChannels m_fInitChannels;
		OV_GT_StopAcquisition m_fStopAcquisition;
		OV_GT_GetLastError m_fGetLastError;
		OV_GT_TranslateErrorCode m_fTranslateErrorCode;

#if defined(TARGET_OS_Windows)
		HINSTANCE m_pLibrary;
#elif defined(TARGET_OS_Linux)
		void* m_pLibrary;
#endif
	};
};

#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#endif // __OpenViBE_AcquisitionServer_CDriverGTecGMobiLabPlus_H__
