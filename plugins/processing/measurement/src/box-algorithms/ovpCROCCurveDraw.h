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
			virtual ~CROCCurveDraw();
			std::vector < CCoordinate >& getCoordinateVector();

			void generateCurve();

			//Callbak functions, should not been called
			void resizeEvent(::GdkRectangle *pRectangle);
			void exposeEnvent();

			//This function is called when the cruve should be redraw for an external reason
			void forceRedraw(void);

		private:
			OpenViBE::uint32 m_ui32Margin;
			OpenViBE::uint32 m_ui32ClassIndex;
			std::vector <GdkPoint> m_oPointList;
			std::vector < CCoordinate > m_oCoordinateList;
			OpenViBE::uint64 m_ui64PixelsPerLeftRulerLabel;

			::GtkWidget *m_pDrawableArea;
			OpenViBE::boolean m_bHasBeenInit;

			//For a mytical reason, gtk says that the DrawableArea is not a DrawableArea unless it's been exposed at least once...
			OpenViBE::boolean m_bHasBeenExposed;

			void redraw();
			void drawLeftMark(OpenViBE::uint32 ui32W, OpenViBE::uint32 ui32H, const char* sLabel);
			void drawBottomMark(OpenViBE::uint32 ui32W, OpenViBE::uint32 ui32H, const char *sLabel);

		};
	}
}



#endif // OVPCROCCURVEDRAW_H
