#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

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
	class CConfigurationEEGO : public CConfigurationBuilder
	{
	public:

		CConfigurationEEGO(IDriverContext& rDriverContext,
		                   const char* sGtkBuilderFileName,
		                   CHeaderEEGO& rEEGOHeader);

		OpenViBE::boolean preConfigure(void) override;
		OpenViBE::boolean postConfigure(void) override;

		// Data
	protected:

		IDriverContext& m_rDriverContext;

		// Methods
	private:

		static void update_channel_num_cb(GtkWidget* widget, CConfigurationEEGO* pThis);

		// Data
	private:

		CHeaderEEGO& m_rEEGOHeader;

		GtkComboBox* m_pEEGRangeComboBox;
		GtkComboBox* m_pBIPRangeComboBox;
		GtkEntry* m_pEEGEntryMask;
		GtkEntry* m_pBIPEntryMask;
		GtkEntry* m_pNumChannelEntry;
	};
};

#endif // header guard

#endif
