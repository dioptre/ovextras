#ifndef __OpenViBE_AcquisitionServer_CConfigurationBrainProductsLiveAmp_H__
#define __OpenViBE_AcquisitionServer_CConfigurationBrainProductsLiveAmp_H__

#ifdef TARGET_HAS_ThirdPartyLiveAmpAPI

#include "../ovasCConfigurationBuilder.h"
#include "ovasCDriverBrainProductsLiveAmp.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationBrainProductsLiveAmp
	 * \author Ratko Petrovic (Brain Products GmbH)
	 * \date Mon Nov 21 14:57:37 2016
	 * \brief The CConfigurationBrainProductsLiveAmp handles the configuration dialog specific to the Brain Products LiveAmp device.
	 *
	 * TODO: details
	 *
	 * \sa CDriverBrainProductsLiveAmp
	 */
	class CConfigurationBrainProductsLiveAmp : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		// you may have to add to your constructor some reference parameters
		// for example, a connection ID:
		CConfigurationBrainProductsLiveAmp(
			CDriverBrainProductsLiveAmp& rDriverContext, 
			const char* sGtkBuilderFileName,			
			OpenViBE::uint32& rPhysicalSampleRate,
			OpenViBE::uint32& rCountEEG,
			OpenViBE::uint32& rCountBip,
			OpenViBE::uint32& rCountAUX,
			OpenViBE::uint32& rCountACC,
			OpenViBE::boolean& rUseAccChannels,
			OpenViBE::uint32& rGoodImpedanceLimit,
			OpenViBE::uint32& rBadImpedanceLimit,
			std::string& sSerialNumber,
			OpenViBE::boolean& rUseBipolarChannels);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

	protected:

		CDriverBrainProductsLiveAmp& m_rDriverContext;

	private:

		/*
		 * Insert here all specific attributes, such as a connection ID.
		 * use references to directly modify the corresponding attribute of the driver
		 * Example:
		 */

	protected:

		OpenViBE::uint32& m_rPhysicalSampleRate;
		OpenViBE::uint32& m_rCountEEG;
		OpenViBE::uint32& m_rCountBip;
		OpenViBE::uint32& m_rCountAUX;
		OpenViBE::uint32& m_rCountACC;
		OpenViBE::boolean& m_rUseAccChannels;
		OpenViBE::boolean& m_rUseBipolarChannels;

		OpenViBE::uint32& m_rGoodImpedanceLimit;
		OpenViBE::uint32& m_rBadImpedanceLimit;
		OpenViBE::uint32& m_rNumberEEGChannels;
		OpenViBE::uint32& m_rNumberAUXChannels;
		OpenViBE::uint32& m_rTriggers;
		std::string& m_sSerialNumber; 

		::GtkWidget* m_pImageErrorIcon;
		::GtkLabel* m_pLabelErrorMessage;
		::GtkButton* m_pButtonErrorDLLink;
		::GtkSpinButton* m_pSpinButtonDeviceId;
		::GtkComboBox* m_pComboBoxMode;
		::GtkComboBox* m_pComboBoxPhysicalSampleRate;
		::GtkComboBox* m_pComboBoxADCDataFilter;
		::GtkComboBox* m_pComboBoxADCDataDecimation;
		::GtkSpinButton* m_pSpinButtonActiveShieldGain;
		::GtkToggleButton* m_pCheckButtonUseAuxChannels;
		::GtkSpinButton* m_pSpinButtonGoodImpedanceLimit;
		::GtkSpinButton* m_pSpinButtonBadImpedanceLimit;

		::GtkSpinButton* m_pNumberEEGChannels;
		::GtkSpinButton* m_pNumberBipolar;
		::GtkSpinButton* m_pNumberAUXChannels;
		::GtkToggleButton* m_pEnableACCChannels;
		::GtkToggleButton* m_pUseBipolarChannels;
		::GtkEntry* m_tSerialNumber;
		
	};
};

#endif // TARGET_HAS_ThirdPartyLiveAmpAPI

#endif // __OpenViBE_AcquisitionServer_CConfigurationBrainProductsLiveAmp_H__
