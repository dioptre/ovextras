#ifndef __OpenViBEPlugins_SimpleVisualisation_CPowerSpectrumDisplay_H__
#define __OpenViBEPlugins_SimpleVisualisation_CPowerSpectrumDisplay_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <visualization-toolkit/ovviz_all.h>

#include "ovpCPowerSpectrumDisplay/ovpCPowerSpectrumDatabase.h"
#include "ovpCPowerSpectrumDisplay/ovpCPowerSpectrumDisplayView.h"

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{

		class CPowerSpectrumDisplay : public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
			public:
				CPowerSpectrumDisplay();

				virtual void release(void) { delete this; }
				virtual bool initialize();
				virtual bool uninitialize();
				virtual bool processInput(uint32_t ui32InputIndex);
				virtual bool process();

				_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithm, OVP_ClassId_PowerSpectrumDisplay)

			public:

				OpenViBEToolkit::TSpectrumDecoder<CPowerSpectrumDisplay> m_oSpectrumDecoder;

				//main object used for the display (contains all the GUI code)
				CSignalDisplayDrawable* m_pPowerSpectrumDisplayView;

				//Contains all the data about the incoming signal
				CPowerSpectrumDatabase* m_pPowerSpectrumDisplayDatabase;

				//Start and end time of the last buffer
				OpenViBE::uint64 m_ui64StartTime;
				OpenViBE::uint64 m_ui64EndTime;

		private:
			OpenViBEVisualizationToolkit::IVisualizationContext* m_visualizationContext;
		};

		class CPowerSpectrumDisplayDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
			public:
				virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Power spectrum display"); }
				virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Vincent Delannoy"); }
				virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
				virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Power spectrum in frequency bands"); }
				virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("TODO"); }
				virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualization/Basic"); }
				virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
				virtual void release(void)                                   { }
				virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_PowerSpectrumDisplay; }
				virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-zoom-fit"); }
				virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CPowerSpectrumDisplay(); }

				virtual bool hasFunctionality(OpenViBE::CIdentifier functionalityIdentifier) const
				{
					return functionalityIdentifier == OVD_Functionality_Visualization;
				}

				virtual bool getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
				{
					rPrototype.addSetting("Minimum frequency to display", OV_TypeId_Float, "0");
					rPrototype.addSetting("Maximum frequency to display", OV_TypeId_Float, "40");
					rPrototype.addInput("Spectrum", OV_TypeId_Spectrum);
					return true;
				}

				_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_PowerSpectrumDisplayDesc)
		};
	};
};

#endif // __OpenViBEPlugins_SimpleVisualisation_CPowerSpectrumDisplay_H__
