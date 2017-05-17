#ifndef __OpenViBEPlugins_SimpleVisualisation_CSimple3DDatabase_H__
#define __OpenViBEPlugins_SimpleVisualisation_CSimple3DDatabase_H__

#include "../../ovp_defines.h"

#include "../../ovpCBufferDatabase.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <map>
#include <vector>
#include <deque>
#include <queue>
#include <string>
#include <cfloat>
#include <iostream>

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		class CSimple3DDrawable : public CSignalDisplayDrawable
		{
			public:
				virtual ~CSimple3DDrawable(){}
		};

		/**
		* This class is used to store information about the incoming signal stream. It can request a CSignalDisplayDrawable
		* object to redraw himself in case of some changes in its data.
		*/
		class CSimple3DDatabase : public CBufferDatabase
		{
		public:
			CSimple3DDatabase(
				OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>& oPlugin);

			virtual ~CSimple3DDatabase();

			bool set3DWidgetIdentifier(
				OpenViBE::CIdentifier o3DWidgetIdentifier);

			bool createVisualObject(
				OpenViBE::CString sObjectName,
				OpenViBE::Kernel::EStandard3DObject eStandard3DObject);

			bool createVisualObject(
				OpenViBE::CString sObjectName,
				OpenViBE::CString sObjectFileName);

			bool deleteVisualObject(
				OpenViBE::CString sObjectName);

			bool setVisualObjectPosition(
				OpenViBE::CString sObjectName,
				OpenViBE::float32 pPosition[3]);
/*
			bool setVisualObjectOrientation(
				OpenViBE::CString sObjectName,
				???);*/

			bool setVisualObjectScale(
				OpenViBE::CString sObjectName,
				OpenViBE::float32 f32ScaleX,
				OpenViBE::float32 f32ScaleY,
				OpenViBE::float32 f32ScaleZ);

			bool setVisualObjectMaterialColor(
				OpenViBE::CString sObjectName,
				OpenViBE::float32 pColor[4]);

			bool setVisualObjectVertexColor(
				OpenViBE::CString sObjectName,
				OpenViBE::float32 pColor[4]);

			bool repositionCamera();

			//called by boxalgorithm's process func
			bool process3D();

		private:
			std::map<std::string, OpenViBE::CIdentifier> m_mSimulatedObjects;

			OpenViBE::CIdentifier m_o3DWidgetIdentifier;

			//create object
			bool m_bCreateObject;
			OpenViBE::CString m_sCreateObjectName;
			OpenViBE::CString m_sCreateObjectFileName;
			OpenViBE::Kernel::EStandard3DObject m_eStandard3DObject;

			//delete object
			bool m_bDeleteObject;
			OpenViBE::CString m_sDeleteObjectName;

			//set position
			bool m_bSetObjectPosition;
			OpenViBE::CString m_sPositionObjectName;
			OpenViBE::float32 m_pPosition[3];

			//set orientation
			bool m_bSetObjectOrientation;
			OpenViBE::CString m_sOrientationObjectName;
			OpenViBE::float32 m_pOrientation[4]; //quaternion

			//set scale
			bool m_bSetObjectScale;
			OpenViBE::CString m_sScaleObjectName;
			OpenViBE::float32 m_f32ScaleX, m_f32ScaleY, m_f32ScaleZ;

			//set color
			bool m_bSetObjectMaterialColor;
			bool m_bSetObjectVertexColor;
			OpenViBE::CString m_sColorObjectName;
			OpenViBE::float32 m_pColor[4];

			//reposition camera
			bool m_bRepositionCamera;
		};
	};
};

#endif // __OpenViBEPlugins_SimpleVisualisation_CElectrodePotentialsDatabase_H__
