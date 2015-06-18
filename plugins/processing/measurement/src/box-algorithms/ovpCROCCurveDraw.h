#ifndef __OpenViBEPlugins_BoxAlgorithm_ROCCurveDraw_H__
#define __OpenViBEPlugins_BoxAlgorithm_ROCCurveDraw_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>


#include <gtk/gtk.h>
#include <vector>

namespace OpenViBEPlugins
{
	namespace Measurement
	{

		typedef std::pair < OpenViBE::float64, OpenViBE::float64 > CCoordinate;

		class CROCCurveDraw{
		public:
			CROCCurveDraw(::GtkNotebook* pNotebook, OpenViBE::uint32 ui32ClassIndex, OpenViBE::CString& rClassName);
			::GtkWidget* getWidget(void);
			std::vector < CCoordinate >& getCoordinateVector();


			void generateCurve();
			void exposeEnvent();
			void resizeEvent(::GdkRectangle *pRectangle);


			void forceRedraw(void);

		private:
			OpenViBE::uint32 m_ui32Margin;
			OpenViBE::uint32 m_ui32ClassIndex;
			std::vector <GdkPoint> m_oPointList;
			std::vector < CCoordinate > m_oCoordinateList;
			::GtkWidget *m_pDrawableArea;

			::GtkNotebook* m_pNotebook;
			OpenViBE::boolean m_bHasBeenInit;

			//For a mytical reason, gtk says that the DrawableArea is not a DrawableArea unless it's been exposed at least once...
			OpenViBE::boolean m_bHasBeenExposed;

			void redraw();
		};
	}
}

#endif // OVPCROCCURVEDRAW_H
