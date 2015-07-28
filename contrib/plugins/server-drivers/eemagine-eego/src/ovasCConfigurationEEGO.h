
#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#if defined TARGET_OS_Windows

#ifndef __OpenViBE_AcquisitionServer_CConfigurationEEGO_H__
#define __OpenViBE_AcquisitionServer_CConfigurationEEGO_H__

#include <toolkit/ovtk_all.h>
#include <ovasIDriver.h>
#include <../ovasCConfigurationBuilder.h>
#include <gtk/gtk.h>

#include "ovasCHeaderEEGO.h"

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationEEGO
	 * \author Steffen Heimes (Eemagine GmbH)
	 * \date Fri May 27 21:48:42 2011
	 * \brief The CConfigurationEEGO handles the configuration dialog for setting specific for EEGO.
	 * \sa CDriverEEGO
	 */
	class CConfigurationEEGO : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		CConfigurationEEGO(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, 
			const char* sGtkBuilderFileName, 
			OpenViBEAcquisitionServer::CHeaderEEGO& rEEGOHeader);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

	// Data
	protected:

		OpenViBEAcquisitionServer::IDriverContext&		m_rDriverContext;
		
	// Methods
	private:

		static void update_channel_num_cb(GtkWidget *widget, CConfigurationEEGO* pThis);

	// Data
	private:

		OpenViBEAcquisitionServer::CHeaderEEGO&	m_rEEGOHeader;

		::GtkComboBox*			m_pEEGRangeComboBox;
		::GtkComboBox*			m_pBIPRangeComboBox;
		::GtkEntry*				m_pEEGEntryMask;
		::GtkEntry*				m_pBIPEntryMask;
		::GtkEntry*				m_pNumChannelEntry;
	};
};

#endif // header guard
#endif // TARGET_OS_Windows

#endif