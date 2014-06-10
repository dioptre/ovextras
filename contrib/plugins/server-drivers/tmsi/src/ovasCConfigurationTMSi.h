/*
 * ovasCConfigurationTMSi.h
 *
 * Copyright (c) 2014, Mensia Technologies SA. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
 */

#ifndef __OpenViBE_AcquisitionServer_CConfigurationTMSi_H__
#define __OpenViBE_AcquisitionServer_CConfigurationTMSi_H__

#include "../ovasCConfigurationBuilder.h"
#include <gtk/gtk.h>
#include <string>
#include <iostream>

namespace OpenViBEAcquisitionServer
{
	class CDriverTMSi;

	class CConfigurationTMSi : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:
		CConfigurationTMSi(const char* sGtkBuilderFileName, CDriverTMSi* pDriver);
		virtual ~CConfigurationTMSi();

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

		void fillDeviceCombobox();
		OpenViBE::boolean fillSamplingFrequencyCombobox();
		void fillAdditionalChannelsTable();
		void clearAdditionalChannelsTable();
		OpenViBE::CString getActiveAdditionalChannels();

		void showWaitWindow();
		void hideWaitWindow();

	public:
		CDriverTMSi* m_pDriver;

		GtkSpinButton* m_pSpinButtonChannelCount;
		GtkComboBox* m_pComboBoxConnectionProtocol;
		GtkComboBox* m_pComboBoxDeviceIdentifier;
		GtkComboBox* m_pComboBoxSamplingFrequency;
		GtkComboBox* m_pComboBoxImpedanceLimit;
//		GtkToggleButton* m_pToggleButtonCommonAverageReference;
		GtkLabel* m_pLabelAdditionalChannels;
		GtkTable* m_pTableAdditionalChannels;
		std::vector<GtkCheckButton*> m_vAdditionalChannelCheckButtons;
	private:
		std::vector<OpenViBE::CString> m_vAdditionalChannelNames;


		GtkWidget* m_pWaitWindow;
	};

}

#endif // __OpenViBE_AcquisitionServer_CConfigurationTMSi_H__
