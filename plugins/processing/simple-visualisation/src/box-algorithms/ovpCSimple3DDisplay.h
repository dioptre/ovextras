#ifndef __SamplePlugin_CSimple3DDisplay_H__
#define __SamplePlugin_CSimple3DDisplay_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <visualization-toolkit/ovviz_all.h>

#include "ovpCSimple3DDisplay/ovpCSimple3DDatabase.h"
#include "ovpCSimple3DDisplay/ovpCSimple3DView.h"

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		class CSimple3DDisplay : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			CSimple3DDisplay(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::uint64 getClockFrequency(void);
			virtual bool initialize(void);
			virtual bool uninitialize(void);
			virtual bool processInput(
				OpenViBE::uint32 ui32InputIndex);
			virtual bool processClock(
				OpenViBE::Kernel::IMessageClock& rMessageClock);
			virtual bool process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_Simple3DDisplay)

		protected:
			CSimple3DDatabase* m_pSimple3DDatabase;
			CSignalDisplayDrawable* m_pSimple3DView; //main object used for the display (contains all the GUI code)
			OpenViBE::CIdentifier m_o3DWidgetIdentifier;
		private:
			OpenViBEVisualizationToolkit::IVisualizationContext* m_visualizationContext;

		};

		class CSimple3DDisplayDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Simple 3D viewer"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Vincent Delannoy"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("3D object handler/viewer"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Examples/Visualisation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(GTK_STOCK_EXECUTE); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Simple3DDisplay; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CSimple3DDisplay(); }

			virtual bool hasFunctionality(OpenViBE::CIdentifier functionalityIdentifier) const
			{
				return functionalityIdentifier == OVD_Functionality_Visualization;
			}

			virtual bool getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_Simple3DDisplayDesc);
		};
	};
};

#endif // __SamplePlugin_CSimple3DDisplay_H__
