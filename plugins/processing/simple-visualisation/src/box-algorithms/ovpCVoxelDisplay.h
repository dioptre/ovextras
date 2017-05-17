#ifndef __SamplePlugin_CVoxelDisplay_H__
#define __SamplePlugin_CVoxelDisplay_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>

#include "../ovpCStreamedMatrixDatabase.h"

#include <visualization-toolkit/ovviz_all.h>

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		class CVoxelView;

		class CVoxel
		{
		public:
			CVoxel();

			bool setObjectIdentifiers(
				OpenViBE::CIdentifier oCubeIdentifier,
				OpenViBE::CIdentifier oSphereIdentifier);

			bool setPosition(
				OpenViBE::float32 f32X,
				OpenViBE::float32 f32Y,
				OpenViBE::float32 f32Z);

			//object identifiers
			OpenViBE::CIdentifier m_oCubeIdentifier;
			OpenViBE::CIdentifier m_oSphereIdentifier;
			//current visibility state of active object
			bool m_bVisible;
			//coordinates of active object
			OpenViBE::float32 m_f32X, m_f32Y, m_f32Z;
		};

		class CVoxelDisplay : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			CVoxelDisplay(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::uint64 getClockFrequency(void);

			virtual bool initialize(void);

			virtual bool uninitialize(void);

			virtual bool processInput(
				OpenViBE::uint32 ui32InputIndex);

			virtual bool processClock(
				OpenViBE::Kernel::IMessageClock& rMessageClock);

			virtual bool process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_VoxelDisplay)

			/** \name CVoxelView callbacks */
			//@{

			bool setVoxelObject(
				OpenViBE::Kernel::EStandard3DObject eStandard3DObject);

			bool toggleColorModification(
				bool bModifyColor);

			bool toggleTransparencyModification(
				bool bModifyTransparency);

			bool toggleSizeModification(
				bool bModifySize);

			bool setMinScaleFactor(
				OpenViBE::float64 f64MinScaleFactor);

			bool setMaxScaleFactor(
				OpenViBE::float64 f64MaxScaleFactor);

			bool setMinDisplayThreshold(
				OpenViBE::float64 f64MinDisplayThreshold);

			bool setMaxDisplayThreshold(
				OpenViBE::float64 f64MaxDisplayThreshold);

			bool setDisplayThresholdBoundaryType(
				bool bInclusiveBoundary);

			bool setSkullOpacity(
				OpenViBE::float64 f64Opacity);

			bool enableAutoCameraMovement(
				bool bEnable);

			bool repositionCamera();

			//@}

		private:
			OpenViBE::CIdentifier getActiveShapeIdentifier(CVoxel& rVoxel);

			/**
			 * \brief Handle 3D scene (voxels loading, creation and update)
			 * \remarks When this method returns false, the plugin is disabled
			 * \return True if processing was successful, false otherwise
			 */
			bool process3D();

			/**
			 * \brief Voxel grid loading
			 * \return True if loading was successful, false otherwise
			 */
			bool loadVoxels();

			/**
			 * \brief Voxel 3D objects creation
			 * \return True if creation was successful, false otherwise
			 */
			bool createVoxels();

			/**
			 * \brief Voxels updating
			 * \return True if updating was successful, false otherwise
			 */
			bool updateVoxels();

			/**
			 * \brief Voxel activation levels computation
			 * \return True if computation was successful, false otherwise
			 */
			bool computeActivationLevels();

			/**
			 * \brief Voxel grid update based on latest activation levels
			 * \return True if update was successful, false otherwise
			 */
			bool processActivationLevels();

			/**
			 * \brief To enhance parallax feeling, camera may be automatically moved
			 */
			bool updateCameraPosition();

		private:
			//Voxel coordinates file reader
			OpenViBE::Kernel::IAlgorithmProxy* m_pOVMatrixFileReader;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::CString* > ip_sFilename;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > op_pVoxelsMatrix;
			bool m_bVoxelsMatrixLoaded;
			//Streamed matrix database
			CStreamedMatrixDatabase* m_pStreamedMatrixDatabase;
			//GUI management
			CVoxelView* m_pVoxelView;

			OpenViBE::CIdentifier m_o3DWidgetIdentifier;
			OpenViBE::CIdentifier m_oResourceGroupIdentifier;

			bool m_bCameraPositioned;
			bool m_bAutoCameraMovementEnabled;
			//time when auto camera movement was last turned on. Theta/Phi offsets are computed from that time step.
			OpenViBE::float64 m_f64AutoCameraMovementStartTime;
			//total theta offset since auto camera movement was last turned on. Reset to 0 when movement is stopped.
			OpenViBE::float32 m_f32ThetaOffset;
			//total phi offset since auto camera movement was last turned on. Reset to 0 when movement is stopped.
			OpenViBE::float32 m_f32PhiOffset;

			OpenViBE::CMatrix m_oPotentialMatrix;
			OpenViBE::float64 m_f64MinPotential;
			OpenViBE::float64 m_f64MaxPotential;

			OpenViBE::uint32 m_ui32NbColors; //number of predefined colors
			OpenViBE::float32* m_pColorScale; //scale of predefined colors potentials are converted to

			std::vector<OpenViBE::CIdentifier> m_oElectrodeIds; //ids of electrode objects
			std::vector<OpenViBEPlugins::SimpleVisualisation::CVoxel> m_oVoxels; //voxels vector
			OpenViBE::CIdentifier m_oScalpId;
			OpenViBE::CIdentifier m_oFaceId;

			/** \name Members modified by CVoxelView requests */
			//@{
			//set voxel object
			bool m_bSetVoxelObject;
			OpenViBE::Kernel::EStandard3DObject m_eVoxelObject;

			//toggle color modification
			bool m_bToggleColorModification;
			bool m_bColorModificationToggled;

			//toggle transparency modification
			bool m_bToggleTransparencyModification;
			bool m_bTransparencyModificationToggled;

			//toggle size modification
			bool m_bToggleSizeModification;
			bool m_bSizeModificationToggled;

			//scale factors
			OpenViBE::float64 m_f64MinScaleFactor;
			OpenViBE::float64 m_f64MaxScaleFactor;

			//display threshold boundary type
			bool m_bInclusiveDisplayThresholdBoundary;

			//voxel display thresholds
			OpenViBE::float64 m_f64MinDisplayThreshold;
			OpenViBE::float64 m_f64MaxDisplayThreshold;

			//set skull opacity
			bool m_bSetSkullOpacity;
			OpenViBE::float64 m_f64SkullOpacity;

			//reposition camera
			bool m_bRepositionCamera;
			//@}

			OpenViBEVisualizationToolkit::IVisualizationContext* m_visualizationContext;

		};

		class CVoxelDisplayDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Voxel display"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Vincent Delannoy"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Displays brain activity as voxels"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Visualisation/Volume"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(GTK_STOCK_EXECUTE); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_VoxelDisplay; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SimpleVisualisation::CVoxelDisplay(); }

			virtual bool hasFunctionality(OpenViBE::CIdentifier functionalityIdentifier) const
			{
				return functionalityIdentifier == OVD_Functionality_Visualization;
			}

			virtual bool getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addSetting("Voxels filename", OV_TypeId_Filename, "");
				rPrototype.addInput("Voxel activity levels", OV_TypeId_StreamedMatrix);
				rPrototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_VoxelDisplayDesc);
		};
	};
};

#endif // __SamplePlugin_CVoxelDisplay_H__
