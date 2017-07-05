#ifndef __OpenViBESkeletonGenerator_CBoxAlgorithmSkeletonGenerator_H__
#define __OpenViBESkeletonGenerator_CBoxAlgorithmSkeletonGenerator_H__

#include "ovsgCSkeletonGenerator.h"
#include <map>
#include <vector>

namespace OpenViBESkeletonGenerator
{
	class CBoxAlgorithmSkeletonGenerator : public CSkeletonGenerator
	{
		public:

			CBoxAlgorithmSkeletonGenerator(OpenViBE::Kernel::IKernelContext & rKernelContext,::GtkBuilder * pBuilderInterface);
			virtual ~CBoxAlgorithmSkeletonGenerator(void);

			bool initialize(void);
			bool save(OpenViBE::CString sFileName);
			bool load(OpenViBE::CString sFileName);
			void getCurrentParameters(void);
		
			// Box Description
			OpenViBE::CString              m_sName;
			OpenViBE::CString              m_sVersion;
			OpenViBE::CString              m_sClassName;
			OpenViBE::CString              m_sCategory;
			OpenViBE::CString              m_sShortDescription;
			OpenViBE::CString              m_sDetailedDescription;
			int32_t               m_i32GtkStockItemIndex;
			OpenViBE::CString              m_sGtkStockItemName;

			struct IOSStruct{
				OpenViBE::CString _name;
				OpenViBE::CString _type;
				OpenViBE::CString _typeId;
				OpenViBE::CString _defaultValue;
			};

			// Inputs
			bool              m_bCanModifyInputs;
			bool              m_bCanAddInputs;
			std::vector<IOSStruct>         m_vInputs;
			// Outputs
			bool              m_bCanModifyOutputs;
			bool              m_bCanAddOutputs;
			std::vector<IOSStruct>         m_vOutputs;
			// Settings
			bool              m_bCanModifySettings;
			bool              m_bCanAddSettings;
			std::vector<IOSStruct>         m_vSettings;

			//Algorithms
			std::vector<OpenViBE::CString> m_vAlgorithms; // the algorithm selected by user
			// Can be made non-const after '= false' produces working code
			static const bool              m_bUseCodecToolkit = true; // use or not the codec toolkit for encoder and decoder algorithms
			std::map <OpenViBE::CString, OpenViBE::CString> m_mAlgorithmHeaderDeclaration; //the map between algorithm and corresponding header declaration (all variables algo/input/output).
			std::map <OpenViBE::CString, OpenViBE::CString> m_mAlgorithmInitialisation;//the map between algorithm and corresponding initialisation
			std::map <OpenViBE::CString, OpenViBE::CString> m_mAlgorithmInitialisation_ReferenceTargets;//the map between algorithm and corresponding initialisation of ref targets
			std::map <OpenViBE::CString, OpenViBE::CString> m_mAlgorithmUninitialisation;//the map between algorithm and corresponding uninitialisation
			
			// Box Listener
			bool              m_bUseBoxListener;
			// input
			bool              m_bBoxListenerOnInputAdded;
			bool              m_bBoxListenerOnInputRemoved;
			bool              m_bBoxListenerOnInputTypeChanged;
			bool              m_bBoxListenerOnInputNameChanged;
			bool              m_bBoxListenerOnInputConnected;
			bool              m_bBoxListenerOnInputDisconnected;
			// output
			bool              m_bBoxListenerOnOutputAdded;
			bool              m_bBoxListenerOnOutputRemoved;
			bool              m_bBoxListenerOnOutputTypeChanged;
			bool              m_bBoxListenerOnOutputNameChanged;
			bool              m_bBoxListenerOnOutputConnected;
			bool              m_bBoxListenerOnOutputDisconnected;
			// setting
			bool              m_bBoxListenerOnSettingAdded;
			bool              m_bBoxListenerOnSettingRemoved;
			bool              m_bBoxListenerOnSettingTypeChanged;
			bool              m_bBoxListenerOnSettingNameChanged;
			bool              m_bBoxListenerOnSettingDefaultValueChanged;
			bool              m_bBoxListenerOnSettingValueChanged;
			
			bool              m_bProcessInput;
			bool              m_bProcessClock;
			uint32_t               m_ui32ClockFrequency;
			bool              m_bProcessMessage;

			void buttonCheckCB(void);
			void buttonOkCB(void);
			void toggleListenerCheckbuttonsStateCB(bool bNewState);
			void buttonTooltipCB(::GtkButton* pButton);
			void buttonExitCB(void);

			void buttonAddInputCB(void);
			void buttonAddOutputCB(void);
			void buttonAddSettingCB(void);
			void buttonAddAlgorithmCB(void);
			void buttonRemoveGeneric(const char* buttonName);

			void algorithmSelectedCB(OpenViBE::int32 i32IndexSelected);
			void setSensitivity(const char* widgetName, bool isActive);

		private:

			virtual OpenViBE::Kernel::ILogManager& getLogManager(void) const { return m_rKernelContext.getLogManager(); }
			virtual OpenViBE::Kernel::IErrorManager& getErrorManager(void) const { return m_rKernelContext.getErrorManager(); }


			std::map < ::GtkButton*, OpenViBE::CString > m_vTooltips;

			OpenViBE::CString getRandomIdentifierString(void);

			std::vector<OpenViBE::CString> m_vParameterType_EnumTypeCorrespondance;

			// Sanity checks that a string is not empty or consist of spaces
			bool isStringValid(const char *string);
	};

	class CDummyAlgoProto : public OpenViBE::Kernel::IAlgorithmProto
	{
	public:
		std::map<OpenViBE::CString, OpenViBE::Kernel::EParameterType> m_vInputs;
		std::map<OpenViBE::CString, OpenViBE::Kernel::EParameterType> m_vOutputs;
		std::vector<OpenViBE::CString> m_vInputTriggers;
		std::vector<OpenViBE::CString> m_vOutputTriggers;
	public:
		bool addInputParameter(
			const OpenViBE::CIdentifier& rInputParameterIdentifier,
			const OpenViBE::CString& sInputName,
			const OpenViBE::Kernel::EParameterType eParameterType,
			const OpenViBE::CIdentifier& rSubTypeIdentifier=OV_UndefinedIdentifier);
			
		bool addOutputParameter(
			const OpenViBE::CIdentifier& rOutputParameterIdentifier,
			const OpenViBE::CString& sOutputName,
			const OpenViBE::Kernel::EParameterType eParameterType,
			const OpenViBE::CIdentifier& rSubTypeIdentifier=OV_UndefinedIdentifier);
			
		bool addInputTrigger(
			const OpenViBE::CIdentifier& rInputTriggerIdentifier,
			const OpenViBE::CString& rInputTriggerName);
			
		bool addOutputTrigger(
			const OpenViBE::CIdentifier& rOutputTriggerIdentifier,
			const OpenViBE::CString& rOutputTriggerName);

		OpenViBE::CIdentifier getClassIdentifier(void) const {return OV_UndefinedIdentifier;}
	};
}

#endif //__OpenViBESkeletonGenerator_CBoxAlgorithmSkeletonGenerator_H__
