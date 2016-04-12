#ifndef __OpenViBEPlugins_CDLLBridge_H__
#define __OpenViBEPlugins_CDLLBridge_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <string>
#include <vector>
#include <queue>
#include <cstdio>

#if defined(TARGET_OS_Windows)
#include <windows.h>
#endif

namespace OpenViBEPlugins
{
	namespace DLLBridge
	{
		class CDLLBridge : public OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize();
			virtual OpenViBE::boolean uninitialize();

			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			virtual OpenViBE::boolean process();

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm<OpenViBE::Plugins::IBoxAlgorithm>, OVP_ClassId_DLLBridge)

		private:

			OpenViBEToolkit::TDecoder < CDLLBridge >* m_pDecoder;		
			OpenViBEToolkit::TEncoder < CDLLBridge >* m_pEncoder;			

			OpenViBE::CIdentifier m_oInputType;
			OpenViBE::CString m_sDLLFile;
			OpenViBE::CString m_sParameters;

			// These functions are expected from the DLL library
			// @note the inputs are non-const on purpose to ensure maximum compatibility with non-C++ dlls
			typedef void (* INITFUNC)(OpenViBE::int32* paramsLength, const char *params, OpenViBE::int32 *errorCode);
			typedef void (* UNINITFUNC)(OpenViBE::int32 *errorCode);
			typedef void (* PROCESSHEADERFUNC)(
				OpenViBE::int32* rowsIn, OpenViBE::int32* colsIn, OpenViBE::int32* samplingRateIn,
				OpenViBE::int32* rowsOut, OpenViBE::int32* colsOut, OpenViBE::int32* samplingRateOut,
				OpenViBE::int32* errorCode);
			typedef void (* PROCESSFUNC)(OpenViBE::float64* matIn, OpenViBE::float64* matOut, OpenViBE::int32* errorCode);

			INITFUNC m_pInitialize;
			UNINITFUNC m_pUninitialize;
			PROCESSHEADERFUNC m_pProcessHeader;
			PROCESSFUNC m_pProcess;

#if defined(TARGET_OS_Windows)
			HINSTANCE m_pLibrary;
#elif defined(TARGET_OS_Linux)
			void* m_pLibrary;
#endif

		};

		class CDLLBridgeListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:
			virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getInputType(ui32Index, l_oTypeIdentifier);
				rBox.setOutputType(0, l_oTypeIdentifier);
				return true;
			}

			virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getOutputType(ui32Index, l_oTypeIdentifier);
				rBox.setInputType(0, l_oTypeIdentifier);
				return true;
			};
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};

		/**
		* Plugin's description
		*/
		class CDLLBridgeDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("DLL Bridge"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Process a signal or matrix stream with a DLL"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Scripting"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.2"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-convert"); }
			virtual void release(void)                                   { }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_DLLBridge; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::DLLBridge::CDLLBridge(); }
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new OpenViBEPlugins::DLLBridge::CDLLBridgeListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }

			virtual OpenViBE::boolean getBoxPrototype(OpenViBE::Kernel::IBoxProto& rPrototype) const
			{
				rPrototype.addSetting("DLL file", OV_TypeId_Filename, "");
				rPrototype.addSetting("Parameters", OV_TypeId_String, "");

				rPrototype.addInput("Input",OV_TypeId_Signal);
				rPrototype.addOutput("Output",OV_TypeId_Signal);

				rPrototype.addInputSupport(OV_TypeId_StreamedMatrix);
				rPrototype.addInputSupport(OV_TypeId_Signal);
				rPrototype.addOutputSupport(OV_TypeId_StreamedMatrix);
				rPrototype.addOutputSupport(OV_TypeId_Signal);

				rPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
				rPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);

				rPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_DLLBridgeDesc);

		};

	};
};

#endif

