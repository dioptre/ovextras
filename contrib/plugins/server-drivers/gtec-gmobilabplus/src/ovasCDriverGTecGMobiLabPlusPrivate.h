/*
 * The only purpose of this class is to hide the gtec API
 * data types from the ov driver header. An approach resembling 
 * this is sometimes called a 'd-pointer'.
 *
 * Previous situation:
 *
 * gUsbamp ov driver header depends on gtec gUSBAmp API types
 * gMobilab ov driver header depends on gtec gMobilab API types
 * Acquisition Server includes both these driver headers
 *
 * But the gtec types are declared differently in the two gtec APIs. 
 * Hence we get a conflict if both headers are included by the 
 * same compilation unit.
 *
 * With this class, the gmobilab API is not exposed just by 
 * including ovasCDriverGtecMobiLabPlus.h to the Acquisition Server.
 *
 */
#ifndef __OpenViBE_AcquisitionServer_CDriverGTecGMobiLabPlusPrivate_H__
#define __OpenViBE_AcquisitionServer_CDriverGTecGMobiLabPlusPrivate_H__

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#include <gMOBIlabplus.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \author Jussi T. Lindgren (Inria)
     *
     * The class collects all members that used to be in 
     * CDriverGtecMobiLabPlus header that depend on the gmobilab API.
     *
     */
	class CDriverGTecGMobiLabPlusPrivate 
	{
	public:

		//useful data to communicate with the gTec module
		_BUFFER_ST m_oBuffer;
		HANDLE m_oDevice;
		_AIN m_oAnalogIn;

#if defined(TARGET_OS_Windows)
		OVERLAPPED m_oOverlap;
#endif

		// These functions are defined in the gmobilab library
#if defined(TARGET_OS_Windows)
		typedef HANDLE(__stdcall *OV_GT_OpenDevice)(LPSTR lpPort);
		typedef BOOL(__stdcall *OV_GT_CloseDevice)(HANDLE hDevice);
		typedef BOOL(__stdcall *OV_GT_SetTestmode)(HANDLE hDevice, BOOL Testmode);
		typedef BOOL(__stdcall *OV_GT_StartAcquisition)(HANDLE hDevice);
		typedef BOOL(__stdcall *OV_GT_GetData)(HANDLE hDevice, _BUFFER_ST *buffer, LPOVERLAPPED lpOvl);
		typedef BOOL(__stdcall *OV_GT_InitChannels)(HANDLE hDevice, _AIN analogCh, _DIO digitalCh);
		typedef bool(__stdcall *OV_GT_StopAcquisition)(HANDLE hDevice);
		typedef bool(__stdcall *OV_GT_GetLastError)(UINT * LastError);
		typedef bool(__stdcall *OV_GT_TranslateErrorCode)(_ERRSTR *ErrorString, UINT ErrorCode);
#else
		typedef HANDLE(*OV_GT_OpenDevice)(const char* lpPort);
		typedef bool(*OV_GT_CloseDevice)(HANDLE hDevice);
		typedef bool(*OV_GT_SetTestmode)(HANDLE hDevice, bool Testmode);
		typedef bool(*OV_GT_StartAcquisition)(HANDLE hDevice);
		typedef bool(*OV_GT_GetData)(HANDLE hDevice, _BUFFER_ST *buffer);
		typedef bool(*OV_GT_InitChannels)(HANDLE hDevice, _AIN analogCh, _DIO digitalCh);
		typedef	bool(*OV_GT_StopAcquisition)(HANDLE hDevice);
		typedef bool(*OV_GT_GetLastError)(unsigned int* LastError);
		typedef bool(*OV_GT_TranslateErrorCode)(_ERRSTR *ErrorString, unsigned int ErrorCode);
#endif

		// Function pointers to the dll
		OV_GT_OpenDevice m_fOpenDevice;
		OV_GT_CloseDevice m_fCloseDevice;
		OV_GT_SetTestmode m_fSetTestmode;
		OV_GT_StartAcquisition m_fStartAcquisition;
		OV_GT_GetData m_fGetData;
		OV_GT_InitChannels m_fInitChannels;
		OV_GT_StopAcquisition m_fStopAcquisition;
		OV_GT_GetLastError m_fGetLastError;
		OV_GT_TranslateErrorCode m_fTranslateErrorCode;

	};
};

#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#endif // __OpenViBE_AcquisitionServer_CDriverGTecGMobiLabPlusPrivate_H__
