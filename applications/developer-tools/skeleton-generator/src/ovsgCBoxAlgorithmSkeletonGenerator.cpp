#include "ovsgCBoxAlgorithmSkeletonGenerator.h"


#include <iostream>
#include <sstream>

#include <glib/gstdio.h>
#include <cstdio>

#include <boost/regex.hpp>

#include <ctime>
#include <cmath>

#include <list>

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace OpenViBESkeletonGenerator;

//Modified version of https://stackoverflow.com/questions/36435204/converting-a-cstring-to-camelcase
std::string camelCase(const char* linec)
{
	std::string line(linec);
	bool active = true;

	for(size_t i = 0; i < line.length();)
	{
		if(std::isalpha(line[i]))
		{
			if(active) {
				line[i] = std::toupper(line[i]);
				active = false;
			} else {
				line[i] = std::tolower(line[i]);
			}
			i++;
		}
		else if(line[i] == ' ')
		{
			active = true;
			line.erase(i, 1);
		}
	}
	return line;
}

bool CDummyAlgoProto::addInputParameter(const CIdentifier& rInputParameterIdentifier,const CString& sInputName,const EParameterType eParameterType,const CIdentifier& rSubTypeIdentifier)
{
	m_vInputs[sInputName] = eParameterType;
	return true;
}
bool CDummyAlgoProto::addOutputParameter(const CIdentifier& rOutputParameterIdentifier,const CString& sOutputName,const EParameterType eParameterType,const CIdentifier& rSubTypeIdentifier)
{
	m_vOutputs[sOutputName] = eParameterType;
	return true;
}

bool CDummyAlgoProto::addInputTrigger(const CIdentifier& rInputTriggerIdentifier,const CString& rInputTriggerName)
{
	m_vInputTriggers.push_back(rInputTriggerName);
	return true;
}

bool CDummyAlgoProto::addOutputTrigger(const CIdentifier& rOutputTriggerIdentifier,const CString& rOutputTriggerName)
{
	m_vOutputTriggers.push_back(rOutputTriggerName);
	return true;
}

//-----------------------------------------------------------------------
static void button_check_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonCheckCB();
}
static void button_tooltip_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonTooltipCB(pButton);
}
static void button_ok_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonOkCB();
}

static void button_add_input_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonAddInputCB();
}
static void button_remove_input_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonRemoveInputCB();
}

static void button_add_output_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonAddOutputCB();
}
static void button_remove_output_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonRemoveOutputCB();
}

static void button_add_setting_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonAddSettingCB();
}
static void button_remove_setting_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonRemoveSettingCB();
}

static void button_add_algorithm_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonAddAlgorithmCB();
}
static void button_remove_algorithm_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonRemoveAlgorithmCB();
}
static void algorithm_selected_cb(::GtkComboBox* pCombobox, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->algorithmSelectedCB(gtk_combo_box_get_active(pCombobox));
}

static void button_exit_cb(::GtkButton* pButton, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->buttonExitCB();
	::gtk_exit(0);
}

//

extern "C" G_MODULE_EXPORT void entry_modified_cb(::GtkWidget * pObject, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->forceRecheckCB();
}
extern "C" G_MODULE_EXPORT void listener_checkbutton_toggled_cb(::GtkWidget * pObject, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->toggleListenerCheckbuttonsStateCB((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pObject))>0));
}
extern "C" G_MODULE_EXPORT void processing_method_clock_toggled(::GtkWidget * pObject, void* pUserData)
{
	static_cast<CBoxAlgorithmSkeletonGenerator*>(pUserData)->toggleClockFrequencyStateCB((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pObject))>0));
}
//-----------------------------------------------------------------------
void CBoxAlgorithmSkeletonGenerator::buttonExitCB(void)
{
	getCommonParameters();
	getCurrentParameters();
	cleanConfigurationFile(m_sConfigurationFile);
	saveCommonParameters(m_sConfigurationFile);
	save(m_sConfigurationFile);

	getLogManager() << LogLevel_Info << "All entries saved in ["<< m_sConfigurationFile<<"]. Exiting.\n";
}

void CBoxAlgorithmSkeletonGenerator::forceRecheckCB(void)
{
	::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
	if(l_pButtonOk != NULL) gtk_widget_set_sensitive(l_pButtonOk,false);
}

void CBoxAlgorithmSkeletonGenerator::toggleListenerCheckbuttonsStateCB(bool bNewState)
{
	auto setSensitivity = [&bNewState, this](const char* name)
	{
		::GtkWidget * l_pListenerWidget = GTK_WIDGET(gtk_builder_get_object(this->m_pBuilderInterface, name));
		gtk_widget_set_sensitive(l_pListenerWidget, bNewState);
	};

	setSensitivity("sg-box-listener-input-added-checkbutton");
	setSensitivity("sg-box-listener-input-removed-checkbutton");
	setSensitivity("sg-box-listener-input-type-checkbutton");
	setSensitivity("sg-box-listener-input-name-checkbutton");
	setSensitivity("sg-box-listener-input-connected-checkbutton");
	setSensitivity("sg-box-listener-input-disconnected-checkbutton");

	setSensitivity("sg-box-listener-output-added-checkbutton");
	setSensitivity("sg-box-listener-output-removed-checkbutton");
	setSensitivity("sg-box-listener-output-type-checkbutton");
	setSensitivity("sg-box-listener-output-name-checkbutton");
	setSensitivity("sg-box-listener-output-connected-checkbutton");
	setSensitivity("sg-box-listener-output-disconnected-checkbutton");

	setSensitivity("sg-box-listener-setting-added-checkbutton");
	setSensitivity("sg-box-listener-setting-removed-checkbutton");
	setSensitivity("sg-box-listener-setting-type-checkbutton");
	setSensitivity("sg-box-listener-setting-name-checkbutton");
	setSensitivity("sg-box-listener-setting-connected-checkbutton");
	setSensitivity("sg-box-listener-setting-disconnected-checkbutton");
}

void CBoxAlgorithmSkeletonGenerator::toggleClockFrequencyStateCB(bool bNewState)
{
	::GtkWidget * l_pWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-process-frequency-spinbutton"));
	gtk_widget_set_sensitive(l_pWidget,bNewState);
}

void CBoxAlgorithmSkeletonGenerator::buttonCheckCB(void)
{
	getLogManager() << LogLevel_Info << "Extracting values... \n";
	//Author and Company
	getCommonParameters();
	//Box generator entries
	getCurrentParameters();

	getLogManager() << LogLevel_Info << "Checking values... \n";

	bool l_bSuccess = true;

	stringstream l_ssTextBuffer;
	l_ssTextBuffer << "----- STATUS -----\n";

	//-------------------------------------------------------------------------------------------------------------------------------------------//
	// Box Description
	//-------------------------------------------------------------------------------------------------------------------------------------------//

	if(!isStringValid(m_sName))
	{
		OV_WARNING_K("-- box name: INVALID (" << (const char *)m_sName << ")");
		l_ssTextBuffer << "[FAILED] No name found. Please provide a name for the box (all characters allowed).\n";
		l_bSuccess = false;
	}
	else
	{
		//m_sName = ensureSedCompliancy(m_sName);
		getLogManager() << LogLevel_Info << "-- box name: VALID (" << (const char *)m_sName << ")\n";
		l_ssTextBuffer << "[   OK   ] Valid box name.\n";
	}

	auto checkRegex = [&l_bSuccess, &l_ssTextBuffer, this](CString& valToCompare, const boost::regex& regex, const std::string& successText, const std::string& errorText)
	{
		if(boost::regex_match(string(valToCompare),regex))
		{
			getLogManager() << LogLevel_Info << "-- " << successText.c_str();
			l_ssTextBuffer << "[   OK   ] " << successText.c_str();
		}
		else
		{
			OV_WARNING_K("-- " << successText.c_str());
			l_ssTextBuffer << "[FAILED] " << successText.c_str();
			l_bSuccess = false;
		}
	};

	checkRegex(m_sClassName,
		  boost::regex("([a-z]|[A-Z])+([a-z]|[A-Z]|[0-9]|[_])*",boost::regex::perl),
		  std::string("Valid class name (") + (const char *)m_sClassName + ").\n",
		  std::string("Invalid class name (") + (const char *)m_sClassName + "). Please provide a class name using lower/upper case letters, numbers or underscores.\n");

	checkRegex(m_sCategory,
		  boost::regex("([a-z]|[A-Z])+([a-z]|[A-Z]|[ ]|[/])*",boost::regex::perl),
		  std::string("Valid category (") + (const char *)m_sCategory + ").\n",
		  std::string("Invalid category (") + (const char *)m_sCategory + "). Please provide a category using only letters and spaces (for sub-category, use '/' separator).\n");

	checkRegex(m_sVersion,
		  boost::regex("([0-9])+([a-z]|[A-Z]|[0-9]|[\\.])*",boost::regex::perl),
		  std::string("Valid box version (") + (const char *)m_sVersion + ").\n",
		  std::string("Invalid box version (") + (const char *)m_sVersion + "). Please use a number followed by either numbers, letters or '.'.\n");

	{
		//m_sShortDescription = ensureSedCompliancy(m_sShortDescription);
		getLogManager() << LogLevel_Info << "-- short description: VALID (" << (const char *)m_sShortDescription << ")\n";
		l_ssTextBuffer << "[   OK   ] Valid short description.\n";
	}

	if(((string)m_sDetailedDescription).length()<500)
	{
		//m_sDetailedDescription = ensureSedCompliancy(m_sDetailedDescription);
		getLogManager() << LogLevel_Info << "-- detailed description: VALID (" << (const char *)m_sDetailedDescription << ")\n";
		l_ssTextBuffer << "[   OK   ] Valid detailed description.\n";
	}

	//-------------------------------------------------------------------------------------------------------------------------------------------//
	// Box INPUTS OUTPUTS and SETTINGS
	//-------------------------------------------------------------------------------------------------------------------------------------------//

	auto setLogHeader = [&l_ssTextBuffer, this](size_t collectionSize, const char* typeText)
	{
		if(collectionSize != 0)
		{
			l_ssTextBuffer << "Checking " << typeText << "... \n";
			getLogManager() << LogLevel_Info << "-- checking " << typeText << "s...\n";
		}
		else
		{
			l_ssTextBuffer << "[----//----] No " << typeText << " specified.\n";
			getLogManager() << LogLevel_Info << "No " << typeText << " specified.\n";
		}
	};

	auto checkCollection = [&l_bSuccess, &setLogHeader, &l_ssTextBuffer, this](std::vector<IOSStruct>& collection, const char* typeText, const char* problemSolution)
	{
		std::string typeTextCapitalized(typeText);
		typeTextCapitalized[0] = std::toupper(typeTextCapitalized[0]);
		setLogHeader(collection.size(), typeText);
		for(uint32_t i = 0; i < collection.size(); i++)
		{
			if(isStringValid(collection[i]._name) && isStringValid(collection[i]._type))
			{
				collection[i]._name = ensureSedCompliancy(collection[i]._name);
				getLogManager() << LogLevel_Info << "  -- " << typeTextCapitalized.c_str() << " " << i <<": [" << (const char *)collection[i]._name<<"],["<< (const char *)collection[i]._type << "] VALID.\n";
				l_ssTextBuffer << ">>[   OK   ] Valid " << typeText << " " << i << " [" << (const char *)collection[i]._name<<"]\n";
			}
			else
			{
				OV_WARNING_K("  -- " << typeTextCapitalized.c_str() << " " << i << ": [" << (const char *)collection[i]._name<<"],["<< (const char *)collection[i]._type << "] INVALID.");
				l_ssTextBuffer << ">>[FAILED] Invalid " << typeText << " " << i<< ". " << problemSolution << "\n";
				l_bSuccess = false;
			}
		}
	};

	checkCollection(m_vInputs, "input", "Please provide a name and a type for each input.");
	checkCollection(m_vOutputs, "output", "Please provide a name and a type for each output.");
	checkCollection(m_vSettings, "setting", "Please provide a name, a type and a default value for each setting.");

	//checking the algorithms...
	setLogHeader(m_vAlgorithms.size(), "algorithm");
	for(uint32_t i = 0; i < m_vAlgorithms.size(); i++)
	{
		getLogManager() << LogLevel_Info << "  -- Algorithm "<<i<<": [" << (const char *)m_vAlgorithms[i]<<"] VALID.\n";
		l_ssTextBuffer << ">>[   OK   ] Valid algorithm "<<i<<" [" << (const char *)m_vAlgorithms[i]<<"]\n";
	}

	//-------------------------------------------------------------------------------------------------------------------------------------------//
	::GtkWidget * l_pTooltipTextview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-tooltips-textview"));
	::GtkTextBuffer * l_pTextBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pTooltipTextview));
	if(l_bSuccess)
	{
		l_ssTextBuffer << "----- SUCCESS -----\nPress 'Generate!' to generate the files. If you want to modify your choice(s), please press the \"Check\" button again.";
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,true);

	}
	else
	{
		l_ssTextBuffer << "----- PROCESS FAILED -----\nModify your choices and press the \"Check\" button again.";
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}

	gtk_text_buffer_set_text (l_pTextBuffer,l_ssTextBuffer.str().c_str(), -1);

}

void CBoxAlgorithmSkeletonGenerator::buttonOkCB(void)
{
	getLogManager() << LogLevel_Info << "Generating files, please wait ... \n";
	CString l_sLogMessages = "Generating files...\n";
	::GtkWidget * l_pTooltipTextview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-tooltips-textview"));
	::GtkTextBuffer * l_pTextBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pTooltipTextview));
	
	bool l_bSuccess = true;

	// we ask for a target directory
	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
			"Select the destination folder",
			NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			NULL);
	
	CString l_sTargetDirectory;
	// if the user specified a target directory, it has full priority
	l_sTargetDirectory = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_TargetDirectory}");
	bool l_bNeedFilePrefix = false;
	if((string)l_sTargetDirectory != string(""))
	{
		getLogManager() << LogLevel_Debug << "Target dir user  [" << l_sTargetDirectory << "]\n";
		l_bNeedFilePrefix = true;
	}
	else
	{
		//previous entry
		l_sTargetDirectory = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_TargetDirectory}");
		if((string)l_sTargetDirectory != string(""))
		{
			getLogManager() << LogLevel_Debug << "Target previous  [" << l_sTargetDirectory << "]\n";
			l_bNeedFilePrefix = true;
		}
		else
		{
			//default path = dist
			getLogManager() << LogLevel_Debug << "Target default  [dist]\n";
#ifdef TARGET_OS_Linux
			l_sTargetDirectory = CString(gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(l_pWidgetDialogOpen)));
			l_sTargetDirectory = l_sTargetDirectory + "/..";
#elif defined TARGET_OS_Windows
			l_sTargetDirectory = "..";
#endif
		}
	}
#ifdef TARGET_OS_Linux
	if(l_bNeedFilePrefix) l_sTargetDirectory = "file://"+l_sTargetDirectory;
	gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(l_pWidgetDialogOpen),(const char *)l_sTargetDirectory);
#elif defined TARGET_OS_Windows
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen),(const char *)l_sTargetDirectory);
#endif


	if(gtk_dialog_run(GTK_DIALOG(l_pWidgetDialogOpen))==GTK_RESPONSE_ACCEPT)
	{
		//char* l_sFileName=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		char * l_pTargetDirectory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		m_sTargetDirectory = CString(l_pTargetDirectory);
	}
	else
	{
		getLogManager() << LogLevel_Info << "User cancel. Aborting generation.\n";
		l_sLogMessages += "User cancel. Aborting generation.\n";
		gtk_text_buffer_set_text(l_pTextBuffer,l_sLogMessages,-1);
		gtk_widget_destroy(l_pWidgetDialogOpen);
		return;
	}
	gtk_widget_destroy(l_pWidgetDialogOpen);

	CString l_sDate = getDate();

	// construction of the namespace name from category
	string l_sNamespace(camelCase(m_sCategory));

	// generating some random identifiers
	CString l_sClassIdentifier = getRandomIdentifierString();
	CString l_sDescriptorIdentifier = getRandomIdentifierString();

	// replace tags in the allgorithm description
	if(m_bUseCodecToolkit)
	{
		for(uint32_t i = 0; i < m_vAlgorithms.size(); i++)
		{

			string l_sAlgo = string((const char *)m_mAlgorithmHeaderDeclaration[m_vAlgorithms[i]]);
			size_t it = l_sAlgo.find("@@ClassName@@");
			if(it != string::npos)
			{
				string l_sClass(m_sClassName);
				l_sClass = "CBoxAlgorithm" + l_sClass;
				l_sAlgo.replace(it,13, l_sClass);
				m_mAlgorithmHeaderDeclaration[m_vAlgorithms[i]] = CString(l_sAlgo.c_str());
			}
		}
	}

	// we construct the map of substitutions
	map<CString,CString> l_mSubstitutions;
	l_mSubstitutions[CString("@@Author@@")] = m_sAuthor;
	l_mSubstitutions[CString("@@Date@@")] = l_sDate;
	l_mSubstitutions[CString("@@Company@@")] = m_sCompany;
	l_mSubstitutions[CString("@@Date@@")] = l_sDate;
	l_mSubstitutions[CString("@@BoxName@@")] = m_sName;
	l_mSubstitutions[CString("@@ClassName@@")] = m_sClassName;
	l_mSubstitutions[CString("@@RandomIdentifierClass@@")] = l_sClassIdentifier;
	l_mSubstitutions[CString("@@RandomIdentifierDescriptor@@")] = l_sDescriptorIdentifier;
	l_mSubstitutions[CString("@@ShortDescription@@")] = m_sShortDescription;
	l_mSubstitutions[CString("@@DetailedDescription@@")] = m_sDetailedDescription;
	l_mSubstitutions[CString("@@Category@@")] = m_sCategory;
	l_mSubstitutions[CString("@@Namespace@@")] = CString(l_sNamespace.c_str());
	l_mSubstitutions[CString("@@Version@@")] = m_sVersion;
	l_mSubstitutions[CString("@@StockItemName@@")] = m_sGtkStockItemName;
	l_mSubstitutions[CString("@@InputFlagCanAdd@@")] = (m_bCanAddInputs ? "rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);" : "//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);");
	l_mSubstitutions[CString("@@InputFlagCanModify@@")] = (m_bCanModifyInputs ? "rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);" : "//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);");
	l_mSubstitutions[CString("@@OutputFlagCanAdd@@")] = (m_bCanAddOutputs ? "rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);" : "//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);");
	l_mSubstitutions[CString("@@OutputFlagCanModify@@")] = (m_bCanModifyOutputs ? "rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);" : "//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);");
	//
	l_mSubstitutions[CString("@@SettingFlagCanAdd@@")] = (m_bCanAddSettings ? "rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);" : "//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);");
	l_mSubstitutions[CString("@@SettingFlagCanModify@@")] = (m_bCanModifySettings ? "rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);" : "//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);");
	l_mSubstitutions[CString("@@BoxListenerCommentIn@@")] = (m_bUseBoxListener ? "" : "/*");
	l_mSubstitutions[CString("@@BoxListenerCommentOut@@")] = (m_bUseBoxListener ? "" : "*/");
	l_mSubstitutions[CString("@@BoxListenerOnInputConnectedComment@@")] = (m_bBoxListenerOnInputConnected ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnInputDisconnectedComment@@")] = (m_bBoxListenerOnInputDisconnected ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnInputAddedComment@@")] = (m_bBoxListenerOnInputAdded ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnInputRemovedComment@@")] = (m_bBoxListenerOnInputRemoved ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnInputTypeChangedComment@@")] = (m_bBoxListenerOnInputTypeChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnInputNameChangedComment@@")] = (m_bBoxListenerOnInputNameChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnOutputConnectedComment@@")] = (m_bBoxListenerOnOutputConnected ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnOutputDisconnectedComment@@")] = (m_bBoxListenerOnOutputDisconnected ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnOutputAddedComment@@")] = (m_bBoxListenerOnOutputAdded ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnOutputRemovedComment@@")] = (m_bBoxListenerOnOutputRemoved ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnOutputTypeChangedComment@@")] = (m_bBoxListenerOnOutputTypeChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnOutputNameChangedComment@@")] = (m_bBoxListenerOnOutputNameChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnSettingAddedComment@@")] = (m_bBoxListenerOnSettingAdded ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnSettingRemovedComment@@")] = (m_bBoxListenerOnSettingRemoved ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnSettingTypeChangedComment@@")] = (m_bBoxListenerOnSettingTypeChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnSettingNameChangedComment@@")] = (m_bBoxListenerOnSettingNameChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnSettingDefaultValueChangedComment@@")] = (m_bBoxListenerOnSettingDefaultValueChanged ? "" : "//");
	l_mSubstitutions[CString("@@BoxListenerOnSettingValueChangedComment@@")] = (m_bBoxListenerOnSettingValueChanged ? "" : "//");
	l_mSubstitutions[CString("@@ProcessClockComment@@")] = (m_bProcessClock ? "" : "//");
	l_mSubstitutions[CString("@@ProcessInputComment@@")] = (m_bProcessInput ? "" : "//");
	l_mSubstitutions[CString("@@ProcessMessageComment@@")] = (m_bProcessMessage ? "" : "//");
	l_mSubstitutions[CString("@@ProcessClockCommentIn@@")] = (m_bProcessClock ? "" : "/*");
	l_mSubstitutions[CString("@@ProcessClockCommentOut@@")] = (m_bProcessClock ? "" : "*/");
	l_mSubstitutions[CString("@@ProcessInputCommentIn@@")] = (m_bProcessInput ? "" : "/*");
	l_mSubstitutions[CString("@@ProcessInputCommentOut@@")] = (m_bProcessInput ? "" : "*/");
	l_mSubstitutions[CString("@@ProcessMessageCommentIn@@")] = (m_bProcessMessage ? "" : "/*");
	l_mSubstitutions[CString("@@ProcessMessageCommentOut@@")] = (m_bProcessMessage ? "" : "*/");
	stringstream ss; ss << m_ui32ClockFrequency << "LL<<32";
	l_mSubstitutions[CString("@@ClockFrequency@@")] = ss.str().c_str();
	
	//-------------------------------------------------------------------------------------------------------------------------------------------//
	// box.h
	CString l_sDest = m_sTargetDirectory + "/ovpCBoxAlgorithm" + m_sClassName + ".h";
	CString l_sTemplate = m_rKernelContext.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/box.h-skeleton");
	
	if(!this->generate(l_sTemplate,l_sDest,l_mSubstitutions,l_sLogMessages))
	{
		gtk_text_buffer_set_text (l_pTextBuffer,
			l_sLogMessages,
			-1);
		l_bSuccess = false;
	}

	CString l_sPattern;
	CString l_sSubstitute;

	//--------------------------------------------------------------------------------------
	//Inputs
	//--------------------------------------------------------------------------------------

	l_sPattern = "\\t\\t\\t\\t@@Inputs@@";
	l_sSubstitute = !m_vInputs.empty() ? "" : "\\/\\/No input specified.To add inputs use :\\n\\/\\/rBoxAlgorithmPrototype.addInput(\\\"Input Name\\\",OV_TypeId_XXXX);\\n";
	for(auto& elem : m_vInputs)
	{
		l_sSubstitute += "\\t\\t\\t\\trBoxAlgorithmPrototype.addInput(\\\""+elem._name+"\\\",OV_TypeId_" + camelCase(elem._type).c_str() +");\\n";
	}
	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);

	//--------------------------------------------------------------------------------------
	//Outputs
	//--------------------------------------------------------------------------------------
	l_sPattern = "\\t\\t\\t\\t@@Outputs@@";
	l_sSubstitute = !m_vOutputs.empty() ? "" : "\\/\\/No output specified.To add outputs use :\\n\\/\\/rBoxAlgorithmPrototype.addOutput(\\\"Output Name\\\",OV_TypeId_XXXX);\\n";
	for(auto& elem : m_vOutputs)
	{
		l_sSubstitute += "\\t\\t\\t\\trBoxAlgorithmPrototype.addOutput(\\\"" + elem._name + "\\\",OV_TypeId_"+ camelCase(elem._type).c_str() +");\\n";
	}
	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);

	//--------------------------------------------------------------------------------------
	//Settings
	//--------------------------------------------------------------------------------------
	l_sPattern = "\\t\\t\\t\\t@@Settings@@";
	l_sSubstitute = !m_vSettings.empty() ? "" : "\\/\\/No setting specified.To add settings use :\\n\\/\\/rBoxAlgorithmPrototype.addSetting(\\\"Setting Name\\\",OV_TypeId_XXXX,\\\"default value\\\");\\n";
	for(auto& elem : m_vSettings)
	{
		l_sSubstitute += "\\t\\t\\t\\trBoxAlgorithmPrototype.addSetting(\\\"" + elem._name + "\\\",OV_TypeId_" + camelCase(elem._type).c_str() + ",\\\""+elem._defaultValue+"\\\");\\n";
	}
	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);

	//--------------------------------------------------------------------------------------
	//Codecs algorithms
	//--------------------------------------------------------------------------------------
	l_sPattern = "@@Algorithms@@";
	l_sSubstitute = "";

	l_sSubstitute += m_vInputs.size() == 0 ? "\\/\\/ No Input decoder.\\n" : "\\/\\/ Input decoder:\\n";
	for(size_t idx = 0 ; m_vInputs.size() ; idx++)
	{
		string l_sTypeName(camelCase((const char *)m_vInputs[idx]._type));
		//The stream type is Stimulations but the decoder is tStimulationDecoder
		if( l_sTypeName == "Stimulations")
		{
			l_sTypeName = "Stimulation";
		}
		l_sSubstitute += CString("\\t\\t\\tOpenViBEToolkit::T") + l_sTypeName.c_str() +"Decoder < CBoxAlgorithm"+m_sClassName+" > m_oInput"+ std::to_string(idx).c_str() + "Decoder;\\n";
	}

	l_sSubstitute += m_vOutputs.size() == 0 ? "\\t\\t\\t\\/\\/ No Output decoder.\\n" : "\\t\\t\\t\\/\\/ Output decoder:\\n";
	for(size_t idx = 0 ; m_vOutputs.size() ; idx++)
	{
		string l_sTypeName(camelCase((const char *)m_vOutputs[idx]._type));
		//The stream type is Stimulations but the encoder is tStimulationEncoder
		if( l_sTypeName == "Stimulations")
		{
			l_sTypeName = "Stimulation";
		}
		l_sSubstitute += CString("\\t\\t\\tOpenViBEToolkit::T") + l_sTypeName.c_str() +"Encoder < CBoxAlgorithm"+m_sClassName+"> m_oOutput"+ std::to_string(idx).c_str() + "Encoder;\\n";
	}
	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);

	//-------------------------------------------------------------------------------------------------------------------------------------------//
	// box.cpp
	l_sDest = m_sTargetDirectory + "/ovpCBoxAlgorithm" + m_sClassName + ".cpp";
	if(m_bUseCodecToolkit)
	{
		l_sTemplate= m_rKernelContext.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/box.cpp-codec-toolkit-skeleton");
	}
	else
	{
		l_sTemplate= m_rKernelContext.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/box.cpp-skeleton");
	}
	if(!this->generate(l_sTemplate,l_sDest,l_mSubstitutions,l_sLogMessages))
	{
		gtk_text_buffer_set_text (l_pTextBuffer,
			l_sLogMessages,
			-1);
		l_bSuccess = false;
	}

	// Codec Algorithm stuff. too complicated for the simple SED primitives.
	l_sPattern = "@@AlgorithmInitialisation@@";
	l_sSubstitute = "";
	//We initialize the codec algorithm by give them this, and the index of the input/output
	for(size_t idx = 0 ; idx <  m_vInputs.size() ; idx++)
	{
		l_sSubstitute += CString("\\tm_oInput") + std::to_string(idx).c_str() + "Decoder.initialize(*this, " + std::to_string(idx).c_str() + ");\n";
	}
	for(size_t idx = 0 ; idx <  m_vOutputs.size() ; idx++)
	{
		l_sSubstitute += CString("\\tm_oOutput") + std::to_string(idx).c_str() + "Encoder.initialize(*this, " + std::to_string(idx).c_str() + ");\n";
	}

	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);
		
	l_sPattern = "@@AlgorithmInitialisationReferenceTargets@@";
	l_sSubstitute = "";

	if(!m_bUseCodecToolkit)
	{
		for(uint32 a=0; a<m_vAlgorithms.size(); a++)
		{
			string l_sBlock = string((const char *)m_mAlgorithmInitialisation_ReferenceTargets[m_vAlgorithms[a]]);
			stringstream ss; ss << "Algo" << a << "_";
			string l_sUniqueMarker = ss.str();
			for(uint32 s=0; s<l_sBlock.length(); s++)
			{
				if(l_sBlock[s]=='@')
				{
					l_sBlock.erase(s,1);
					l_sBlock.insert(s,l_sUniqueMarker);
				}
			}
			l_sSubstitute += CString(l_sBlock.c_str());
		}
	}
	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);
		
		
	l_sPattern = "@@AlgorithmUninitialisation@@";
	l_sSubstitute = "";
	//We initialize the codec algorithm by give them this, and the index of the input/output
	for(size_t idx = 0 ; idx <  m_vInputs.size() ; idx++)
	{
		l_sSubstitute += CString("\\tm_oInput") + std::to_string(idx).c_str() + "Decoder.uninitialize();\n";
	}
	for(size_t idx = 0 ; idx <  m_vOutputs.size() ; idx++)
	{
		l_sSubstitute += CString("\\tm_oOutput") + std::to_string(idx).c_str() + "Encoder.uninitialize();\n";
	}
	l_bSuccess &= regexReplace(l_sDest, l_sPattern, l_sSubstitute);

	//-------------------------------------------------------------------------------------------------------------------------------------------//
	// readme-box.cpp
	l_sDest = m_sTargetDirectory + "/README.txt";
	l_sTemplate = m_rKernelContext.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/readme-box.txt-skeleton");
	
	if(!this->generate(l_sTemplate,l_sDest,l_mSubstitutions,l_sLogMessages))
	{
		gtk_text_buffer_set_text (l_pTextBuffer,
			l_sLogMessages,
			-1);
		l_bSuccess = false;
	}
	
	//-------------------------------------------------------------------------------------------------------------------------------------------//

	if(l_bSuccess)
	{
		l_bSuccess&=cleanConfigurationFile(m_sConfigurationFile);
		//re-load all entries, the internal variables may have been modified to be sed compliant.
		getCommonParameters();
		getCurrentParameters();
		//save the entries as the user typed them
		l_bSuccess&=saveCommonParameters(m_sConfigurationFile);
		l_bSuccess&=save(m_sConfigurationFile);
	}

	if(!l_bSuccess)
	{
		l_sLogMessages += "Generation process did not completly succeed. Some files may have not been produced.\n";
		OV_WARNING_K("Generation process did not completly succeed. Some files may have not been produced.");
	}
	else
	{
		l_sLogMessages += CString("Generation process successful. All entries saved in [") + m_sConfigurationFile + "]\n";
		l_sLogMessages += "Please read file [README.txt] !\n";
		getLogManager() << LogLevel_Info << "Generation process successful. All entries saved in [" << m_sConfigurationFile << "]\n";
		
		// opening browser to see the produced files
		CString l_sBrowser = m_rKernelContext.getConfigurationManager().expand("${Designer_WebBrowserCommand_${OperatingSystem}}");
		CString l_sBrowserCmd = l_sBrowser + " \"" +  m_sTargetDirectory+"\"";

#ifdef TARGET_OS_Windows
		l_sBrowserCmd =  l_sBrowser + " file:///"+  m_sTargetDirectory; //otherwise the browser does not find the directory (problem with / and \ char)
#endif
		if(system((const char *)l_sBrowserCmd))
		{
		}
	}

	gtk_text_buffer_set_text(l_pTextBuffer,l_sLogMessages,-1);
}

void CBoxAlgorithmSkeletonGenerator::buttonTooltipCB(::GtkButton* pButton)
{
	CString l_sTooltip = m_vTooltips[pButton];

	::GtkWidget * l_pTooltipTextview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-tooltips-textview"));
	::GtkTextBuffer * l_pTextBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pTooltipTextview));
	gtk_text_buffer_set_text (l_pTextBuffer, (const char *) l_sTooltip, -1);
}

void CBoxAlgorithmSkeletonGenerator::buttonAddInputCB(void)
{
	::GtkWidget * l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-dialog"));
	::GtkWidget * l_pNameEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-name-entry"));
	::GtkWidget * l_pTypeCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-type-combobox"));
		
	gtk_entry_set_text(GTK_ENTRY(l_pNameEntry),"");
		
	gint resp = gtk_dialog_run(GTK_DIALOG(l_pDialog));

	if(resp== GTK_RESPONSE_APPLY)
	{
		const gchar * l_sName = gtk_entry_get_text(GTK_ENTRY(l_pNameEntry));
		//we get the two types (user/ov)
		GtkTreeIter l_iterType;
		GtkTreeModel * l_treeModelType = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pTypeCombobox));
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(l_pTypeCombobox),&l_iterType);
		gchar* l_dataTypeUser;
		gchar* l_dataTypeOv;
		gtk_tree_model_get(l_treeModelType,&l_iterType,0,&l_dataTypeUser,1,&l_dataTypeOv,-1);
		//const gchar * l_sType = gtk_combo_box_get_active_text(GTK_COMBO_BOX(l_pTypeCombobox));

		::GtkWidget * l_pInputTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-treeview"));
		::GtkTreeModel * l_pInputListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pInputTreeView));
		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pInputListStore),&l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pInputListStore), &l_iter, 0, l_sName,1,l_dataTypeUser,2,l_dataTypeOv,-1);
		gtk_widget_hide(l_pDialog);

		g_free(l_dataTypeUser);g_free(l_dataTypeOv);

		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}
	else
	{
		gtk_widget_hide(l_pDialog);
	}

}
void CBoxAlgorithmSkeletonGenerator::buttonRemoveInputCB(void)
{
	::GtkWidget * l_pInputTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-treeview"));
	::GtkTreeModel * l_pInputListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pInputTreeView));
	GtkTreeIter l_iter;
	GtkTreeSelection *l_select;
	l_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(l_pInputTreeView));
	if(gtk_tree_selection_get_selected (l_select, &l_pInputListStore, &l_iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(l_pInputListStore),&l_iter);
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}

}

void CBoxAlgorithmSkeletonGenerator::buttonAddOutputCB(void)
{
	::GtkWidget * l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-dialog"));
	::GtkWidget * l_pNameEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-name-entry"));
	::GtkWidget * l_pTypeCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-type-combobox"));
		
	gtk_entry_set_text(GTK_ENTRY(l_pNameEntry),"");
		
	gint resp = gtk_dialog_run(GTK_DIALOG(l_pDialog));

	if(resp== GTK_RESPONSE_APPLY)
	{
		const gchar * l_sName = gtk_entry_get_text(GTK_ENTRY(l_pNameEntry));
		//we get the two types (user/ov)
		GtkTreeIter l_iterType;
		GtkTreeModel * l_treeModelType = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pTypeCombobox));
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(l_pTypeCombobox),&l_iterType);
		gchar* l_dataTypeUser;
		gchar* l_dataTypeOv;
		gtk_tree_model_get(l_treeModelType,&l_iterType,0,&l_dataTypeUser,1,&l_dataTypeOv,-1);
		//const gchar * l_sType = gtk_combo_box_get_active_text(GTK_COMBO_BOX(l_pTypeCombobox));

		::GtkWidget * l_pOutputTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-treeview"));
		::GtkTreeModel * l_pOutputListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pOutputTreeView));
		GtkTreeIter l_iter;
	
		gtk_list_store_append(GTK_LIST_STORE(l_pOutputListStore),&l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pOutputListStore), &l_iter, 0, l_sName,1,l_dataTypeUser,2,l_dataTypeOv,-1);
		gtk_widget_hide(l_pDialog);

		g_free(l_dataTypeUser);g_free(l_dataTypeOv);

		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}
	else
	{
		gtk_widget_hide(l_pDialog);
	}
}
void CBoxAlgorithmSkeletonGenerator::buttonRemoveOutputCB(void)
{
	::GtkWidget * l_pOutputTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-treeview"));
	::GtkTreeModel * l_pOutputListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pOutputTreeView));
	GtkTreeIter l_iter;
	GtkTreeSelection *l_select;
	l_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(l_pOutputTreeView));
	if(gtk_tree_selection_get_selected (l_select, &l_pOutputListStore, &l_iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(l_pOutputListStore),&l_iter);
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}
}

void CBoxAlgorithmSkeletonGenerator::buttonAddSettingCB(void)
{
	::GtkWidget * l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-dialog"));
	::GtkWidget * l_pNameEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-name-entry"));
	::GtkWidget * l_pTypeCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-type-combobox"));
	::GtkWidget * l_pValueEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-default-value-entry"));
	
	gtk_entry_set_text(GTK_ENTRY(l_pNameEntry),"");
	gtk_entry_set_text(GTK_ENTRY(l_pValueEntry),"");
	
	gint resp = gtk_dialog_run(GTK_DIALOG(l_pDialog));
		
	if(resp== GTK_RESPONSE_APPLY)
	{
		const gchar * l_sName = gtk_entry_get_text(GTK_ENTRY(l_pNameEntry));
		const gchar * l_sValue = gtk_entry_get_text(GTK_ENTRY(l_pValueEntry));
		//we get the two types (user/ov)
		GtkTreeIter l_iterType;
		GtkTreeModel * l_treeModelType = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pTypeCombobox));
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(l_pTypeCombobox),&l_iterType);
		gchar* l_dataTypeUser;
		gchar* l_dataTypeOv;
		gtk_tree_model_get(l_treeModelType,&l_iterType,0,&l_dataTypeUser,1,&l_dataTypeOv,-1);
		//const gchar * l_sType = gtk_combo_box_get_active_text(GTK_COMBO_BOX(l_pTypeCombobox));

		::GtkWidget * l_pOutputTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-treeview"));
		::GtkTreeModel * l_pOutputListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pOutputTreeView));
		GtkTreeIter l_iter;
	
		gtk_list_store_append(GTK_LIST_STORE(l_pOutputListStore),&l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pOutputListStore), &l_iter, 0, l_sName,1,l_dataTypeUser,2,l_sValue,3,l_dataTypeOv,-1);
		gtk_widget_hide(l_pDialog);

		g_free(l_dataTypeUser);g_free(l_dataTypeOv);
		
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}
	else
	{
		gtk_widget_hide(l_pDialog);
	}
}
void CBoxAlgorithmSkeletonGenerator::buttonRemoveSettingCB(void)
{
	::GtkWidget * l_pSettingTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-treeview"));
	::GtkTreeModel * l_pSettingListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pSettingTreeView));
	GtkTreeIter l_iter;
	GtkTreeSelection *l_select;
	l_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(l_pSettingTreeView));
	if(gtk_tree_selection_get_selected (l_select, &l_pSettingListStore, &l_iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(l_pSettingListStore),&l_iter);
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}
}

//

void CBoxAlgorithmSkeletonGenerator::buttonAddAlgorithmCB(void)
{
	::GtkWidget * l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-dialog"));
	::GtkWidget * l_pAlgoCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-combobox"));
	
	::GtkWidget * l_pAlgoInputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-inputs-treeview"));
	::GtkTreeModel * l_pAlgoInputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoInputsTreeView));
	::GtkWidget * l_pAlgoOutputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-outputs-treeview"));
	::GtkTreeModel * l_pAlgoOutputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoOutputsTreeView));
	::GtkWidget * l_pAlgoInputTriggersTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-input-triggers-treeview"));
	::GtkTreeModel * l_pAlgoInputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoInputTriggersTreeView));
	::GtkWidget * l_pAlgoOutputTriggersTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-output-triggers-treeview"));
	::GtkTreeModel * l_pAlgoOutputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoOutputTriggersTreeView));
	
	::GtkWidget * l_pAlgoCategoryEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-category-entry"));
	gtk_entry_set_text(GTK_ENTRY(l_pAlgoCategoryEntry),"");
	::GtkWidget * l_pAlgoShortEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-short-description-entry"));
	gtk_entry_set_text(GTK_ENTRY(l_pAlgoShortEntry),"");
	::GtkWidget * l_pAlgoDetailedTextview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-detailed-description-textview"));
	::GtkTextBuffer * l_pTextBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pAlgoDetailedTextview));
	gtk_text_buffer_set_text(l_pTextBuffer,"",-1);


	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoInputsListStore));
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoOutputsListStore));
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoInputTriggersListStore));
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoOutputTriggersListStore));
	gtk_combo_box_set_active(GTK_COMBO_BOX(l_pAlgoCombobox),-1);

	gint resp = gtk_dialog_run(GTK_DIALOG(l_pDialog));
		
	if(resp== GTK_RESPONSE_APPLY)
	{
		const gchar * l_sAlgo = gtk_combo_box_get_active_text(GTK_COMBO_BOX(l_pAlgoCombobox));		

		if(l_sAlgo) {
			::GtkWidget * l_pAlgoTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-treeview"));
			::GtkTreeModel * l_pAlgoListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoTreeView));
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pAlgoListStore),&l_iter);
			gtk_list_store_set (GTK_LIST_STORE(l_pAlgoListStore), &l_iter, 0, l_sAlgo,-1);
			::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
			gtk_widget_set_sensitive(l_pButtonOk,false);
		} 
		else 
		{	
			getLogManager() << LogLevel_Error << "Please select an algorithm.\n";
		}
		gtk_widget_hide(l_pDialog);
	}
	else
	{
		gtk_widget_hide(l_pDialog);
	}

}
void CBoxAlgorithmSkeletonGenerator::buttonRemoveAlgorithmCB(void)
{
	::GtkWidget * l_pAlgoTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-treeview"));
	::GtkTreeModel * l_pAlgoListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoTreeView));
	GtkTreeIter l_iter;
	GtkTreeSelection *l_select;
	l_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(l_pAlgoTreeView));
	if(gtk_tree_selection_get_selected (l_select, &l_pAlgoListStore, &l_iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(l_pAlgoListStore),&l_iter);
		::GtkWidget * l_pButtonOk = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
		gtk_widget_set_sensitive(l_pButtonOk,false);
	}
}
void CBoxAlgorithmSkeletonGenerator::algorithmSelectedCB(int32 i32IndexSelected)
{
	::GtkWidget * l_pAlgoInputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-inputs-treeview"));
	::GtkTreeModel * l_pAlgoInputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoInputsTreeView));
	::GtkWidget * l_pAlgoOutputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-outputs-treeview"));
	::GtkTreeModel * l_pAlgoOutputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoOutputsTreeView));
	::GtkWidget * l_pAlgoInputTriggersTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-input-triggers-treeview"));
	::GtkTreeModel * l_pAlgoInputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoInputTriggersTreeView));
	::GtkWidget * l_pAlgoOutputTriggersTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-output-triggers-treeview"));
	::GtkTreeModel * l_pAlgoOutputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoOutputTriggersTreeView));
	
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoInputsListStore));
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoOutputsListStore));
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoInputTriggersListStore));
	gtk_list_store_clear(GTK_LIST_STORE(l_pAlgoOutputTriggersListStore));

	::GtkWidget * l_pAlgoCategoryEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-category-entry"));
	gtk_entry_set_text(GTK_ENTRY(l_pAlgoCategoryEntry),"");
	::GtkWidget * l_pAlgoShortEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-short-description-entry"));
	gtk_entry_set_text(GTK_ENTRY(l_pAlgoShortEntry),"");
	::GtkWidget * l_pAlgoDetailedTextview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-detailed-description-textview"));
	::GtkTextBuffer * l_pTextBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pAlgoDetailedTextview));
	gtk_text_buffer_set_text(l_pTextBuffer,"",-1);

	if(i32IndexSelected != -1)
	{
		::GtkWidget * l_pAlgoCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-combobox"));
		::GtkTreeModel * l_pAlgoListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pAlgoCombobox));
		::GtkTreeIter l_iter;
		gchar *l_sIdentifier;
		gtk_tree_model_iter_nth_child(l_pAlgoListStore,&l_iter,NULL,i32IndexSelected);
		gtk_tree_model_get(l_pAlgoListStore,&l_iter,1,&l_sIdentifier,-1);
		CIdentifier l_identifier;
		l_identifier.fromString(CString(l_sIdentifier));
		
		//we need to create a dummy instance of the algorithm proto to know its input/output/triggers
		const IPluginObjectDesc * l_pDesc = m_rKernelContext.getPluginManager().getPluginObjectDesc(l_identifier);
		CDummyAlgoProto l_oDummyProto;
		((IAlgorithmDesc *)l_pDesc)->getAlgorithmPrototype(l_oDummyProto);

		//inputs of the algorithm
		::GtkWidget * l_pAlgoInputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-inputs-treeview"));
		::GtkTreeModel * l_pAlgoInputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoInputsTreeView));
		for(auto& elem : l_oDummyProto.m_vInputs)
		{
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pAlgoInputsListStore),&l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pAlgoInputsListStore), &l_iter, 0, (const char *)elem.first,-1);
		}

		//outputs of the algorithm
		::GtkWidget * l_pAlgoOutputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-outputs-treeview"));
		::GtkTreeModel * l_pAlgoOutputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoOutputsTreeView));
		for(auto& elem : l_oDummyProto.m_vOutputs)
		{
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pAlgoOutputsListStore),&l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pAlgoOutputsListStore), &l_iter, 0, (const char *)elem.first,-1);
		}

		//Input triggers of the algorithm
		::GtkWidget * l_pAlgoInputTriggersTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-input-triggers-treeview"));
		::GtkTreeModel * l_pAlgoInputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoInputTriggersTreeView));
		for(auto& elem : l_oDummyProto.m_vInputTriggers)
		{
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pAlgoInputTriggersListStore),&l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pAlgoInputTriggersListStore), &l_iter, 0, (const char *)elem,-1);
		}
		//Output triggers of the algorithm
		::GtkWidget * l_pAlgoOutputTriggersTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-output-triggers-treeview"));
		::GtkTreeModel * l_pAlgoOutputTriggersListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoOutputTriggersTreeView));
		for(auto& elem : l_oDummyProto.m_vOutputTriggers)
		{
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pAlgoOutputTriggersListStore),&l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pAlgoOutputTriggersListStore), &l_iter, 0, (const char *)elem,-1);
		}

		::GtkWidget * l_pAlgoCategoryEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-category-entry"));
		gtk_entry_set_text(GTK_ENTRY(l_pAlgoCategoryEntry),(const char*)l_pDesc->getCategory());
		::GtkWidget * l_pAlgoShortEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-short-description-entry"));
		gtk_entry_set_text(GTK_ENTRY(l_pAlgoShortEntry),(const char*)l_pDesc->getShortDescription());
		::GtkWidget * l_pAlgoDetailedTextview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-detailed-description-textview"));
		::GtkTextBuffer * l_pTextBuffer  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pAlgoDetailedTextview));
		gtk_text_buffer_set_text(l_pTextBuffer,(const char*)l_pDesc->getDetailedDescription(),-1);

	}
}
//--------------------------------------------------------------------------
CBoxAlgorithmSkeletonGenerator::CBoxAlgorithmSkeletonGenerator(IKernelContext & rKernelContext, ::GtkBuilder * pBuilderInterface)
	:CSkeletonGenerator(rKernelContext, pBuilderInterface)
{
}
CBoxAlgorithmSkeletonGenerator::~CBoxAlgorithmSkeletonGenerator(void)
{
}

bool CBoxAlgorithmSkeletonGenerator::initialize( void )
{
	//random seed
	srand((unsigned int)time(NULL));
	
	::GtkWidget * l_pBox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-window"));
	
	// Main Buttons and signals
	::GtkWidget * l_pButtonCheck  = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-check-button"));
	::GtkWidget * l_pButtonOk     = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-ok-button"));
	gtk_widget_set_sensitive(l_pButtonOk,false);

	g_signal_connect(l_pButtonCheck, "pressed",G_CALLBACK(button_check_cb), this);
	g_signal_connect(l_pButtonOk,    "pressed",G_CALLBACK(button_ok_cb), this);

	//connect all the signals in the .ui file (entry_modified_cb)
	gtk_builder_connect_signals(m_pBuilderInterface, this);

	// Tooltips buttons and signal
	::GtkButton * l_pTooltipButton_nameVersion         = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-name-version-tooltip-button"));
	::GtkButton * l_pTooltipButton_category            = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-category-tooltip-button"));
	::GtkButton * l_pTooltipButton_description         = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-description-tooltip-button"));
	::GtkButton * l_pTooltipButton_icon                = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-icon-tooltip-button"));
	::GtkButton * l_pTooltipButton_inputs              = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-list-tooltip-button"));
	::GtkButton * l_pTooltipButton_inputs_modify       = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-modify-tooltip-button"));
	::GtkButton * l_pTooltipButton_inputs_addRemove    = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-add-tooltip-button"));
	::GtkButton * l_pTooltipButton_outputs             = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-list-tooltip-button"));
	::GtkButton * l_pTooltipButton_outputs_modify      = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-modify-tooltip-button"));
	::GtkButton * l_pTooltipButton_outputs_addRemove   = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-add-tooltip-button"));
	::GtkButton * l_pTooltipButton_settings            = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-list-tooltip-button"));
	::GtkButton * l_pTooltipButton_settings_modify     = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-modify-tooltip-button"));
	::GtkButton * l_pTooltipButton_settings_addRemove  = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-tooltip-button"));
	::GtkButton * l_pTooltipButton_algorithms          = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-tooltip-button"));
	::GtkButton * l_pTooltipButton_className           = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-class-name-tooltip-button"));
//	::GtkButton * l_pTooltipButton_UseCodecToolkit     = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-toolkit-tooltip-button"));
	::GtkButton * l_pTooltipButton_BoxListener         = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-listener-tooltip-button"));
	//

	m_vTooltips[l_pTooltipButton_nameVersion]        = CString("Box Name: \nThis name will be the one displayed in the Designer.\nUsually, the box name reflects its main purpose.\nPlease also enter a version number for your box.\nAuthorized characters: letters (lower and upper case), numbers, special characters '()[]._-'\n------\nExample: Clock Stimulator (tic tac), version 1.2");
	m_vTooltips[l_pTooltipButton_category]           = CString("Category: \nThe category decides where the box will be strored in designer's box panel.\nYou can refer to an existing category, already used in the designer, or choose a new one.\nIf you need to specifiy a subcategory, use the character '/'.\nAuthorized characters: letters (lower and upper case) and spaces.\n------\nExample: Samples/Skeleton Generator\n");
	m_vTooltips[l_pTooltipButton_description]        = CString("Description: \nThe short description will be displayed next to the box in the designer box panel.\nThe detailed description is showed on the 'About Box...' panel.\nAll characters are authorized.\n------\nExample:\nShort Description : Periodic stimulation generator\nDetailed description : This box triggers stimulation at fixed frequency.");
	m_vTooltips[l_pTooltipButton_icon]               = CString("Box Icon: \nThe icon used in the designer box panel for this box.\nThis is an optional field.\n------\nExample: 'gtk-help' will be the corresponding gtk stock item (depending on the gtk theme used)\n\n\n");
	m_vTooltips[l_pTooltipButton_inputs]             = CString("Inputs: \nUse the 'Add' and 'Remove' buttons to set all the inputs your box will have.\nWhen pressing 'Add' a dialog window will appear to know the name and type of the new input.\n------\nExample:\n'Incoming Signal' of type 'Signal'\n\n");
	m_vTooltips[l_pTooltipButton_inputs_modify]      = CString("Modify: \nCheck this option if the input(s) of your box can be modified (type and name) in the Designer by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanModifyInput'.\n\n\n\n\n");
	m_vTooltips[l_pTooltipButton_inputs_addRemove]   = CString("Add/Remove: \nCheck this option if the user must be able to add (or remove) inputs, by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanAddInput'.\n\n\n\n");
	m_vTooltips[l_pTooltipButton_outputs]            = CString("Outputs: \nUse the 'Add' and 'Remove' buttons to set all the outputs your box will have.\nWhen pressing 'Add' a dialog window will appear to know the name and type of the new output.\n------\nExample:\n'Filtered Signal' of type 'Signal'\n\n");
	m_vTooltips[l_pTooltipButton_outputs_modify]     = CString("Modify: \nCheck this option if the output(s) of your box can be modified (type and name) in the Designer by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanModifyOutput'.\n\n\n\n\n");
	m_vTooltips[l_pTooltipButton_outputs_addRemove]  = CString("Add/Remove: \nCheck this option if the user must be able to add (or remove) outputs, by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanAddOutput'.\n\n\n\n");
	m_vTooltips[l_pTooltipButton_settings]           = CString("Settings: \nUse the 'Add' and 'Remove' buttons to set all the settings your box will have.\nWhen pressing 'Add' a dialog window will appear to know the name,type and default value of the new output.\n------\nExample:\n'Filter order' of type 'int' with default value '4'\n\n");
	m_vTooltips[l_pTooltipButton_settings_modify]    = CString("Modify: \nCheck this option if the setting(s) of your box can be modified (type and name) in the Designer by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanModifySetting'.\n\n\n\n\n");
	m_vTooltips[l_pTooltipButton_settings_addRemove] = CString("Add/Remove: \nCheck this option if the user must be able to add (or remove) settings, by right-clicking the box.\nIn the implementation, this option decides whether or not the box will have the flag 'BoxFlag_CanAddSetting'.\n\n\n\n");
	m_vTooltips[l_pTooltipButton_algorithms]         = CString("Codec Algorithms: \nChoose the decoder(s) and encoder(s) used by the box. \nYou can choose between all the different stream codecs currently in OpenViBE.\nWhen choosing a codec, the dialog window will display the algorithm inputs and outputs that can be retrieve through getter methods. \n------\nExample: Signal Decoder, that outputs a Streamed Matrix and a Sampling Frequency value from a Memory Buffer.\n\n");
	m_vTooltips[l_pTooltipButton_className]          = CString("Class Name: \nThis name will be used in the code to build the class name.\nUsually, the class name is close to the box name, just without any blank.\nAuthorized characters: letters (lower and upper case), numbers, NO special characters, NO blank.\n------\nExample: ClockStimulator\n");
//	m_vTooltips[l_pTooltipButton_UseCodecToolkit]    = CString("Codec Toolkit: \nTells the generator to use or not the Codec Toolkit in the box implementation. \nThe Codec Toolkit makes the decoding and encoding process much more simpler.\nCurrently the Skeleton Generator only creates templates using the codec toolkit.\n\n\n\n\n");
	m_vTooltips[l_pTooltipButton_BoxListener]        = CString("Box Listener: \nImplement or not a box listener class in the header.\nA box listener has various callbacks that you can overwrite, related to any modification of the box structure.\n------\nExample:\nThe Identity box uses a box listener with 2 callbacks: 'onInputAdded' and 'onOutputAdded'.\nWhenever an input (output) is added, the listener automatically add an output (input) of the same type.\n");
	//

	g_signal_connect(l_pTooltipButton_nameVersion,        "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_category,           "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_description,        "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_icon,               "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_inputs,             "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_inputs_modify,      "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_inputs_addRemove,   "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_outputs,            "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_outputs_modify,     "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_outputs_addRemove,  "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_settings,           "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_settings_modify,    "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_settings_addRemove, "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_algorithms,         "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_className,          "pressed",G_CALLBACK(button_tooltip_cb), this);
//	g_signal_connect(l_pTooltipButton_UseCodecToolkit,    "pressed",G_CALLBACK(button_tooltip_cb), this);
	g_signal_connect(l_pTooltipButton_BoxListener,        "pressed",G_CALLBACK(button_tooltip_cb), this);
	//
	
	//'Inputs' buttons
	::GtkButton * l_pInputsButton_add            = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-add-button"));
	::GtkButton * l_pInputsButton_remove         = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-remove-button"));
	
	g_signal_connect(l_pInputsButton_add,    "pressed",G_CALLBACK(button_add_input_cb), this);
	g_signal_connect(l_pInputsButton_remove, "pressed",G_CALLBACK(button_remove_input_cb), this);
	
	//'outputs' buttons
	::GtkButton * l_pOutputsButton_add            = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-add-button"));
	::GtkButton * l_pOutputsButton_remove         = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-remove-button"));
	
	g_signal_connect(l_pOutputsButton_add,    "pressed",G_CALLBACK(button_add_output_cb), this);
	g_signal_connect(l_pOutputsButton_remove, "pressed",G_CALLBACK(button_remove_output_cb), this);
	
	//'settings' buttons
	::GtkButton * l_pSettingsButton_add            = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-button"));
	::GtkButton * l_pSettingsButton_remove         = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-remove-button"));
	
	g_signal_connect(l_pSettingsButton_add,    "pressed",G_CALLBACK(button_add_setting_cb), this);
	g_signal_connect(l_pSettingsButton_remove, "pressed",G_CALLBACK(button_remove_setting_cb), this);

	//'algos' buttons
	::GtkButton * l_pAlgorithmsButton_add          = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-button"));
	::GtkButton * l_pAlgorithmsButton_remove       = GTK_BUTTON(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-remove-button"));
	
	g_signal_connect(l_pAlgorithmsButton_add,    "pressed",G_CALLBACK(button_add_algorithm_cb), this);
	g_signal_connect(l_pAlgorithmsButton_remove, "pressed",G_CALLBACK(button_remove_algorithm_cb), this);

	//Add IO dialog buttons 
	::GtkWidget * l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-dialog"));
	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_APPLY,
		GTK_RESPONSE_APPLY);

	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL);

	//Add Message IO dialog buttons
	l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-messageIO-add-dialog"));
	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_APPLY,
		GTK_RESPONSE_APPLY);

	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL);

	//Add Setting dialog buttons 
	l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-dialog"));
	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_APPLY,
		GTK_RESPONSE_APPLY);

	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL);

	//Add Algo dialog buttons 
	l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-dialog"));
	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_APPLY,
		GTK_RESPONSE_APPLY);

	gtk_dialog_add_button (GTK_DIALOG (l_pDialog),
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL);

	//initialize the icon combo box with gtk stock items
	::GtkWidget * l_pIconCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-icon-combobox"));
	::GtkTreeModel * l_pIconListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pIconCombobox));
	::GSList * l_StockIdList = gtk_stock_list_ids();
	while(l_StockIdList->next!=NULL)
	{
		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pIconListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pIconListStore), &l_iter, 0, (char *)l_StockIdList->data,1,(char *)l_StockIdList->data,-1);
		l_StockIdList = g_slist_next(l_StockIdList);
	}
	g_slist_free(l_StockIdList);

	//types when adding IOS
	::GtkWidget * l_pTypeCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-IO-add-type-combobox"));
	::GtkTreeModel * l_pTypeListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pTypeCombobox));
	::GtkWidget * l_pSettingTypeCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-add-type-combobox"));
	::GtkTreeModel * l_pSettingTypeListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pSettingTypeCombobox));
	//we iterate over all identifiers
	CIdentifier l_Identifier = m_rKernelContext.getTypeManager().getNextTypeIdentifier(OV_UndefinedIdentifier);
	while(l_Identifier != OV_UndefinedIdentifier)
	{
		CString l_sType = m_rKernelContext.getTypeManager().getTypeName(l_Identifier);
		CString l_sTypeId = l_Identifier.toString();
		//Streams are possible inputs and outputs
		if(m_rKernelContext.getTypeManager().isStream(l_Identifier))
		{
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pTypeListStore), &l_iter);
			gtk_list_store_set (GTK_LIST_STORE(l_pTypeListStore), &l_iter, 0, (const char *)l_sType,1,(const char *)l_sTypeId,-1);
		}
		else // other types are possible settings
		{
			GtkTreeIter l_iter;
			gtk_list_store_append(GTK_LIST_STORE(l_pSettingTypeListStore), &l_iter);
			gtk_list_store_set (GTK_LIST_STORE(l_pSettingTypeListStore), &l_iter, 0, (const char *)l_sType,1,(const char *)l_sTypeId,-1);
		}
		l_Identifier = m_rKernelContext.getTypeManager().getNextTypeIdentifier(l_Identifier);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(l_pTypeCombobox),0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(l_pSettingTypeCombobox),0);
	//types when adding Algorithms
	m_vParameterType_EnumTypeCorrespondance.resize(ParameterType_Pointer+1);
	m_vParameterType_EnumTypeCorrespondance[ParameterType_None]           = "TYPE-NOT-AVAILABLE";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Integer]        = "OpenViBE::int64";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_UInteger]       = "OpenViBE::uint64";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Enumeration]    = "ENUMERATION-NOT-AVAILABLE";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Boolean]        = "bool";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Float]          = "OpenViBE::float64";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_String]         = "OpenViBE::CString";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Identifier]     = "OpenViBE::CIdentifier";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Matrix]         = "OpenViBE::IMatrix *";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_StimulationSet] = "OpenViBE::IStimulationSet *";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_MemoryBuffer]   = "OpenViBE::IMemoryBuffer *";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Object]         = "OpenViBE::IObject *";
	m_vParameterType_EnumTypeCorrespondance[ParameterType_Pointer]        = "OpenViBE::uint8*";


	// CODECS INITIALISATION:
	// ::GtkWidget * l_pCodecCheckbutton = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-toolkit-checkbutton"));
	// m_bUseCodecToolkit = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCodecCheckbutton))>0);

	::GtkWidget * l_pAlgoCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-add-combobox"));
	::GtkTreeModel * l_pAlgoListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pAlgoCombobox));
	//we iterate over all plugin descriptor identifiers
	l_Identifier = m_rKernelContext.getPluginManager().getNextPluginObjectDescIdentifier(OV_UndefinedIdentifier);
	while(l_Identifier != OV_UndefinedIdentifier)
	{
		const IPluginObjectDesc * l_pDesc =  m_rKernelContext.getPluginManager().getPluginObjectDesc(l_Identifier);
		if(l_pDesc != NULL && l_pDesc->isDerivedFromClass(OV_ClassId_Plugins_AlgorithmDesc)) // we select only algorithm descriptors
		{
			CString l_sAlgo = l_pDesc->getName();
			string l_sTest((const char *) l_sAlgo);

			// we only keep decoders and encoders
			// and reject the master acquisition stream
			// and reject acquisition stream encoder as toolkit doesn't support it
			if((l_sTest.find("encoder")!=string::npos || l_sTest.find("decoder")!=string::npos) 
				&& l_sTest.find("Master")==string::npos
				&& l_sTest.find("Acquisition stream encoder")==string::npos)
			{
				CString l_sAlgoID = l_Identifier.toString();
				GtkTreeIter l_iter;
				gtk_list_store_append(GTK_LIST_STORE(l_pAlgoListStore), &l_iter);
				gtk_list_store_set (GTK_LIST_STORE(l_pAlgoListStore), &l_iter, 0, (const char *)l_sAlgo,1,(const char *)l_sAlgoID,-1);
			
				// now we map every decoder/encoder to its string description that will be added in the skeleton (algorithmProxy + parameter handlers for I/O)
				CString l_sHeaderDeclaration ="\\t\\t\\t\\/\\/ "+ l_sAlgo +"\\n";
				CString l_sInitialisation ="\\t\\/\\/ "+ l_sAlgo +"\\n";
				CString l_sInitialisation_ReferenceTargets;
				CString l_sUninitialisation;

			
				//we need to create a dummy instance of the algorithm proto to know its input/output/triggers
				const IPluginObjectDesc * l_pDesc = m_rKernelContext.getPluginManager().getPluginObjectDesc(l_Identifier);
				CDummyAlgoProto l_oDummyProto;
				((IAlgorithmDesc *)l_pDesc)->getAlgorithmPrototype(l_oDummyProto);
				//algorithm proxy
				string l_sAlgoNameStdSTr(camelCase((const char *)l_sAlgo));
				string l_sCodecTypeStdStr = l_sAlgoNameStdSTr;
				string l_sStream("Stream");
				l_sCodecTypeStdStr.erase(l_sCodecTypeStdStr.rfind(l_sStream),6);

				CString l_sAlgorithmProxy = "m_p@" + CString(l_sAlgoNameStdSTr.c_str());
				CString l_sCodec = "m_o@" + CString(l_sCodecTypeStdStr.c_str());
				CString l_sCodecType = CString(l_sCodecTypeStdStr.c_str());
			
				if(! m_bUseCodecToolkit)
				{
					l_sInitialisation_ReferenceTargets = "\\t\\/\\/"+ l_sAlgo +" Reference Targets :\\n";
					l_sHeaderDeclaration = l_sHeaderDeclaration + "\\t\\t\\tOpenViBE::Kernel::IAlgorithmProxy* "+ l_sAlgorithmProxy +";\\n";
					l_sInitialisation = l_sInitialisation + "\\t" +
						l_sAlgorithmProxy +" = \\&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_"+CString(l_sAlgoNameStdSTr.c_str())+"));\\n" +
						"\\t"+ l_sAlgorithmProxy +"->initialize();\\n";
					l_sUninitialisation = "\\tthis->getAlgorithmManager().releaseAlgorithm(*"+l_sAlgorithmProxy+");\\n" + l_sUninitialisation;
					//inputs of the algorithm
					for(map<CString,EParameterType>::iterator it = l_oDummyProto.m_vInputs.begin(); it!=l_oDummyProto.m_vInputs.end(); it++)
					{
						string l_sInputNameStdSTr(camelCase((const char *)(*it).first));
						CString l_sInputHandler ="ip_p@"+ CString(l_sInputNameStdSTr.c_str());
						// input handlers for pointer must be "const"
						CString l_sConst = "";
						if(l_oDummyProto.m_vInputs[(*it).first] >= ParameterType_Matrix)
						{
							l_sConst = "const ";
						}
						l_sHeaderDeclaration = l_sHeaderDeclaration + "\\t\\t\\tOpenViBE::Kernel::TParameterHandler < " + l_sConst + (const char *)m_vParameterType_EnumTypeCorrespondance[l_oDummyProto.m_vInputs[(*it).first]] + "> "+ l_sInputHandler +";\\n";
						l_sInitialisation = l_sInitialisation + "\\t" +
							 l_sInputHandler + ".initialize("+ l_sAlgorithmProxy +"->getInputParameter(OVP_GD_Algorithm_"+CString(l_sAlgoNameStdSTr.c_str())+"_InputParameterId_"+CString(l_sInputNameStdSTr.c_str())+"));\\n";
						l_sInitialisation_ReferenceTargets = l_sInitialisation_ReferenceTargets + "\\t\\/\\/"+l_sInputHandler +".setReferenceTarget( ... )\\n";
						l_sUninitialisation = "\\t"+l_sInputHandler+ ".uninitialize();\\n" + l_sUninitialisation;
					}
					//outputs of the algorithm
					for(map<CString,EParameterType>::iterator it = l_oDummyProto.m_vOutputs.begin(); it!=l_oDummyProto.m_vOutputs.end(); it++)
					{
						string l_sOutputNameStdSTr(camelCase((const char *)(*it).first));
						CString l_sOutputHandler ="op_p@"+ CString(l_sOutputNameStdSTr.c_str());
						l_sHeaderDeclaration = l_sHeaderDeclaration + "\\t\\t\\tOpenViBE::Kernel::TParameterHandler < " + (const char *)m_vParameterType_EnumTypeCorrespondance[l_oDummyProto.m_vOutputs[(*it).first]] + "> "+ l_sOutputHandler +";\\n";
						l_sInitialisation = l_sInitialisation + "\\t" +
							l_sOutputHandler+ ".initialize("+ l_sAlgorithmProxy +"->getOutputParameter(OVP_GD_Algorithm_"+CString(l_sAlgoNameStdSTr.c_str())+"_OutputParameterId_"+CString(l_sOutputNameStdSTr.c_str())+"));\\n";
						l_sUninitialisation = "\\t"+ l_sOutputHandler+ ".uninitialize();\\n" + l_sUninitialisation;
					}
					l_sHeaderDeclaration = l_sHeaderDeclaration + "\\n";
					l_sInitialisation = l_sInitialisation + "\\n";
					l_sInitialisation_ReferenceTargets = l_sInitialisation_ReferenceTargets + "\\n";
					l_sUninitialisation ="\\t\\/\\/ "+ l_sAlgo +"\\n" + l_sUninitialisation + "\\t"+l_sAlgorithmProxy+" = NULL;\\n\\n";
				}
				else // use the Codec Toolkit
				{
					l_sHeaderDeclaration = l_sHeaderDeclaration + "\\t\\t\\tOpenViBEToolkit::T"+ l_sCodecType + " < @@ClassName@@ > " + l_sCodec +";\\n";
					l_sInitialisation = l_sInitialisation + "\\t" + l_sCodec + ".initialize(*this);\\n";
					l_sUninitialisation = "\\t" + l_sCodec + ".uninitialize();\\n" + l_sUninitialisation;
					l_sInitialisation_ReferenceTargets = "";
					//l_sInitialisation_ReferenceTargets = l_sInitialisation_ReferenceTargets + "\\t\\/\\/ If you need to, you can manually set the reference targets to link the codecs input and output. To do so, you can use :\n";
					//l_sInitialisation_ReferenceTargets = l_sInitialisation_ReferenceTargets + "\\t\\/\\/"+l_sCodec + ".getInputX().setReferenceTarget( ... )\\n";
				}
				m_mAlgorithmHeaderDeclaration[l_sAlgo]               = l_sHeaderDeclaration;
				m_mAlgorithmInitialisation[l_sAlgo]                  = l_sInitialisation;
				m_mAlgorithmUninitialisation[l_sAlgo]                = l_sUninitialisation;
				m_mAlgorithmInitialisation_ReferenceTargets[l_sAlgo] = l_sInitialisation_ReferenceTargets;
				m_rKernelContext.getLogManager() << LogLevel_Debug << "The algorithm [" << l_sAlgo << "] has description [" << l_sHeaderDeclaration << "\n";
			}
		}
		l_Identifier =m_rKernelContext.getPluginManager().getNextPluginObjectDescIdentifier(l_Identifier);
	}

	

	gtk_combo_box_set_active(GTK_COMBO_BOX(l_pAlgoCombobox),-1);
	//callback to update algo description
	g_signal_connect(G_OBJECT(l_pAlgoCombobox),"changed", G_CALLBACK(algorithm_selected_cb),this);
	
	//Close with X and "cancel" button
	g_signal_connect (G_OBJECT(l_pBox),"delete_event",G_CALLBACK(::gtk_exit),0);
	::GtkWidget * l_pButtonCancel = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-exit-button"));
	g_signal_connect(l_pButtonCancel,"pressed", G_CALLBACK(button_exit_cb), this);

	//load everything from config file
	load(m_sConfigurationFile);
	::GtkWidget * l_pListenerWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-listener-checkbutton"));
	toggleListenerCheckbuttonsStateCB((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pListenerWidget))>0));

	gtk_widget_show_all(l_pBox);

	return true;
}

bool CBoxAlgorithmSkeletonGenerator::save(CString sFileName)
{
	FILE* l_pFile=::fopen(sFileName.toASCIIString(), "ab");
	if(!l_pFile)
	{
		OV_WARNING_K("Saving the box entries in [" << sFileName << "] failed !");
		return false;
	}
	::fprintf(l_pFile, "# ----------------------BOX GENERATOR-------------------------\n");
	// we need to replace the \ by / in the path for cross compatibility
	string::iterator it_directory;
	string l_sTempTargetDirectory(m_sTargetDirectory.toASCIIString());
	for(it_directory=l_sTempTargetDirectory.begin(); it_directory<l_sTempTargetDirectory.end(); it_directory++)
	{
		if((*it_directory)=='\\')
		{
			l_sTempTargetDirectory.replace(it_directory, it_directory+1, 1, '/');
		}
	}
	::fprintf(l_pFile, "SkeletonGenerator_Box_TargetDirectory = %s\n", l_sTempTargetDirectory.c_str());

	::fprintf(l_pFile, "SkeletonGenerator_Box_Name = %s\n",(const char *)m_sName);
	::fprintf(l_pFile, "SkeletonGenerator_Box_Version = %s\n",(const char *) m_sVersion);
	::fprintf(l_pFile, "SkeletonGenerator_Box_Category = %s\n",(const char *)m_sCategory);
	::fprintf(l_pFile, "SkeletonGenerator_Box_ClassName = %s\n",(const char *)m_sClassName);
	
	//we need to escape with '\' the special characters of the configuration manager files
	string l_sTempShortDescr((const char *)m_sShortDescription);
	l_sTempShortDescr.reserve(1000); // if we need to insert characters
	string::iterator it_sdescr=l_sTempShortDescr.begin();
	while(it_sdescr<l_sTempShortDescr.end())
	{
		//characters to escape
		if((*it_sdescr)=='\\' || (*it_sdescr)=='=' || (*it_sdescr)=='$' || (*it_sdescr)=='\t')
		{
			l_sTempShortDescr.insert(it_sdescr, '\\');
			it_sdescr++;
		}
		it_sdescr++;
	}
	::fprintf(l_pFile, "SkeletonGenerator_Box_ShortDescription = %s\n", l_sTempShortDescr.c_str());
	
	//we need to escape with '\' the special characters of the configuration manager files
	string l_sTempDetailedDescr((const char *)m_sDetailedDescription);
	m_rKernelContext.getLogManager() << LogLevel_Debug << "SAVE > DESCRIPTION FROM WIDGET: "<<l_sTempDetailedDescr.c_str()<<"\n";
	l_sTempDetailedDescr.reserve(1000); // if we need to insert characters
	string::iterator it_descr=l_sTempDetailedDescr.begin();
	while(it_descr<l_sTempDetailedDescr.end())
	{
		//characters to escape
		if((*it_descr)=='\\' || (*it_descr)=='=' || (*it_descr)=='$' || (*it_descr)=='\t')
		{
			l_sTempDetailedDescr.insert(it_descr, '\\');
			it_descr++;
		}
		//the special character we use for \n must also be escaped when used in the text
		else if((*it_descr)=='@')
		{
			l_sTempDetailedDescr.insert(it_descr, '\\');
			l_sTempDetailedDescr.insert(it_descr, '\\');
			it_descr+=2;
		}
		//we add a special character @ representing a \n for further loading. the \ ensure that the config manager will read the token past the \n
		else if((*it_descr)=='\n')
		{
			l_sTempDetailedDescr.insert(it_descr, '\\');
			l_sTempDetailedDescr.insert(it_descr, '@');
			it_descr+=2;
		}
		it_descr++;
	}
	m_rKernelContext.getLogManager() << LogLevel_Debug << "SAVE > DESCR MODIFIED: "<<l_sTempDetailedDescr.c_str()<<"\n";
	
	::fprintf(l_pFile, "SkeletonGenerator_Box_DetailedDescription = %s\n", (const char *) l_sTempDetailedDescr.c_str());
	
	::fprintf(l_pFile, "SkeletonGenerator_Box_IconIndex = %i\n",m_i32GtkStockItemIndex);
	::fprintf(l_pFile, "SkeletonGenerator_Box_IconName = %s\n",(const char *)m_sGtkStockItemName);
	
	// ADD/MODIFY FLAGS
	::fprintf(l_pFile, "SkeletonGenerator_Box_CanModifyInputs = %s\n",(m_bCanModifyInputs?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_CanAddInputs = %s\n",(m_bCanAddInputs?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_CanModifyOutputs = %s\n",(m_bCanModifyOutputs?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_CanAddOutputs = %s\n",(m_bCanAddOutputs?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_CanModifySettings = %s\n",(m_bCanModifySettings?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_CanAddSettings = %s\n",(m_bCanAddSettings?"TRUE":"FALSE"));

	// Inputs
	::fprintf(l_pFile, "SkeletonGenerator_Box_InputCount = %lu\n",(unsigned long int)m_vInputs.size());
	for(uint32_t i = 0; i < m_vInputs.size(); i++)
	{
		::fprintf(l_pFile, "SkeletonGenerator_Box_Input%i_Name = %s\n",i,(const char *)m_vInputs[i]._name);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Input%i_Type = %s\n",i,(const char *)m_vInputs[i]._type);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Input%i_TypeId = %s\n",i,(const char *)m_vInputs[i]._typeId);
	}
	// Outputs
	::fprintf(l_pFile, "SkeletonGenerator_Box_OutputCount = %lu\n",(unsigned long int)m_vOutputs.size());
	for(uint32_t i = 0; i < m_vOutputs.size(); i++)
	{
		::fprintf(l_pFile, "SkeletonGenerator_Box_Output%i_Name = %s\n",i,(const char *)m_vOutputs[i]._name);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Output%i_Type = %s\n",i,(const char *)m_vOutputs[i]._type);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Output%i_TypeId = %s\n",i,(const char *)m_vOutputs[i]._typeId);
	}
	// Settings
	::fprintf(l_pFile, "SkeletonGenerator_Box_SettingCount = %lu\n",(unsigned long int)m_vSettings.size());
	for(uint32_t i = 0; i < m_vSettings.size(); i++)
	{
		::fprintf(l_pFile, "SkeletonGenerator_Box_Setting%i_Name = %s\n",i,(const char *)m_vSettings[i]._name);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Setting%i_Type = %s\n",i,(const char *)m_vSettings[i]._type);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Setting%i_TypeId = %s\n",i,(const char *)m_vSettings[i]._typeId);
		::fprintf(l_pFile, "SkeletonGenerator_Box_Setting%i_DefaultValue = %s\n",i,(const char *)m_vSettings[i]._defaultValue);
	}
	// Algorithms
	::fprintf(l_pFile, "SkeletonGenerator_Box_AlgorithmCount = %lu\n",(unsigned long int)m_vAlgorithms.size());
	for(uint32_t i = 0; i < m_vAlgorithms.size(); i++)
	{
		::fprintf(l_pFile, "SkeletonGenerator_Box_Algorithm%i_Name = %s\n",i,(const char *)m_vAlgorithms[i]);
	}

	// Listener
	::fprintf(l_pFile, "SkeletonGenerator_Box_UseListener = %s\n",(m_bUseBoxListener?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnInputAdded = %s\n",(m_bBoxListenerOnInputAdded?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnInputRemoved = %s\n",(m_bBoxListenerOnInputRemoved?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnInputTypeChanged = %s\n",(m_bBoxListenerOnInputTypeChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnInputNameChanged = %s\n",(m_bBoxListenerOnInputNameChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnInputConnected = %s\n",(m_bBoxListenerOnInputConnected?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnInputDisconnected = %s\n",(m_bBoxListenerOnInputDisconnected?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnOutputAdded = %s\n",(m_bBoxListenerOnOutputAdded?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnOutputRemoved = %s\n",(m_bBoxListenerOnOutputRemoved?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnOutputTypeChanged = %s\n",(m_bBoxListenerOnOutputTypeChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnOutputNameChanged = %s\n",(m_bBoxListenerOnOutputNameChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnOutputConnected = %s\n",(m_bBoxListenerOnOutputConnected?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnOutputDisconnected = %s\n",(m_bBoxListenerOnOutputDisconnected?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnSettingAdded = %s\n",(m_bBoxListenerOnSettingAdded?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnSettingRemoved = %s\n",(m_bBoxListenerOnSettingRemoved?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnSettingTypeChanged = %s\n",(m_bBoxListenerOnSettingTypeChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnSettingNameChanged = %s\n",(m_bBoxListenerOnSettingNameChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnSettingDefaultValueChanged = %s\n",(m_bBoxListenerOnSettingDefaultValueChanged?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ListenerOnSettingValueChanged = %s\n",(m_bBoxListenerOnSettingValueChanged?"TRUE":"FALSE"));

	::fprintf(l_pFile, "SkeletonGenerator_Box_ProcessInput = %s\n",(m_bProcessInput?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ProcessClock = %s\n",(m_bProcessClock?"TRUE":"FALSE"));
	::fprintf(l_pFile, "SkeletonGenerator_Box_ProcessMessage = %s\n",(m_bProcessMessage?"TRUE":"FALSE"));
	stringstream ssListener; ssListener << m_ui32ClockFrequency;
	::fprintf(l_pFile, "SkeletonGenerator_Box_ClockFrequency = %s\n",ssListener.str().c_str());
	



	::fprintf(l_pFile, "# --------------------------------------------------\n");
	::fclose(l_pFile);
	m_rKernelContext.getLogManager() << LogLevel_Info << "box entries saved in [" << sFileName << "]\n";

	m_bConfigurationFileLoaded = false; 

	return true;
}

bool CBoxAlgorithmSkeletonGenerator::load(CString sFileName)
{
	if(!m_bConfigurationFileLoaded && !m_rKernelContext.getConfigurationManager().addConfigurationFromFile(sFileName))
	{
		OV_WARNING_K("box: Configuration file [" << sFileName << "] could not be loaded.");
		return false;
	}

	::GtkWidget * l_pClassNameEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-class-name-entry"));
	CString l_sClassName = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_ClassName}");
	gtk_entry_set_text(GTK_ENTRY(l_pClassNameEntry),(const char *) l_sClassName);
	::GtkWidget * l_pCategoryEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-category-entry"));
	CString l_sCategory = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_Category}");
	gtk_entry_set_text(GTK_ENTRY(l_pCategoryEntry),(const char *) l_sCategory);
	::GtkWidget * l_pNameEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-box-name-entry"));
	CString l_sName = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_Name}");
	gtk_entry_set_text(GTK_ENTRY(l_pNameEntry),(const char *) l_sName);
	::GtkWidget * l_pVersionEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-version-entry"));
	CString l_sVersion = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_Version}");
	gtk_entry_set_text(GTK_ENTRY(l_pVersionEntry),(const char *) l_sVersion);
	::GtkWidget * l_pSDEntry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-short-description-entry"));
	
	CString l_sShortDescr = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_ShortDescription}");
	//we need to UNescape the special characters of the configuration manager files
	string::iterator it_sdescr;
	string l_sTempShortDescr(l_sShortDescr.toASCIIString());
	for(it_sdescr=l_sTempShortDescr.begin(); it_sdescr<l_sTempShortDescr.end(); it_sdescr++)
	{
		// juste erase the escape character
		if((*it_sdescr)=='\\' && (it_sdescr+1) != l_sTempShortDescr.end())
		{
			if((*(it_sdescr+1))=='\\' || (*(it_sdescr+1))=='=' || (*(it_sdescr+1))=='$' || (*(it_sdescr+1))=='\t' || (*(it_sdescr+1))=='@')
			{
				l_sTempShortDescr.erase(it_sdescr);
			}
		}
		// replace the special character @ by \n in the textview
		else if((*it_sdescr)=='@')
		{
			l_sTempShortDescr.erase(it_sdescr);
			l_sTempShortDescr.insert(it_sdescr,'\n');
		}
	}	
	gtk_entry_set_text(GTK_ENTRY(l_pSDEntry),l_sTempShortDescr.c_str());
	
	::GtkWidget * l_pDetailedDescrTextView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-detailed-description-textview"));
	::GtkTextBuffer * l_pDetailedDescrTextBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pDetailedDescrTextView));
	CString l_sDetailedDescr = m_rKernelContext.getConfigurationManager().expand("${SkeletonGenerator_Box_DetailedDescription}");
	
	//we need to UNescape the special characters of the configuration manager files
	string::iterator it_descr;
	string l_sTempDetailedDescr(l_sDetailedDescr.toASCIIString());
	getLogManager() << LogLevel_Debug << "LOAD > DESCR LOADED: "<<l_sTempDetailedDescr.c_str()<<"\n";
	for(it_descr=l_sTempDetailedDescr.begin(); it_descr<l_sTempDetailedDescr.end(); it_descr++)
	{
		// juste erase the escape character
		if((*it_descr)=='\\' && (it_descr+1) != l_sTempDetailedDescr.end())
		{
			if((*(it_sdescr+1))=='\\' || (*(it_sdescr+1))=='=' || (*(it_sdescr+1))=='$' || (*(it_sdescr+1))=='\t' || (*(it_sdescr+1))=='@')
			{
				l_sTempDetailedDescr.erase(it_descr);
			}
		}
		// replace the special character @ by \n in the textview
		else if((*it_descr)=='@')
		{
			l_sTempDetailedDescr.erase(it_descr);
			l_sTempDetailedDescr.insert(it_descr,'\n');
		}
	}	
	getLogManager() << LogLevel_Debug << "LOAD > DESCR MODIFIED: "<<l_sTempDetailedDescr.c_str()<<"\n";
	gtk_text_buffer_set_text(l_pDetailedDescrTextBuffer,l_sTempDetailedDescr.c_str(),((string)l_sTempDetailedDescr).length());

	GtkWidget * l_pIconCombobox =  GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-icon-combobox"));
	int64_t l_iIconSelected = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_IconIndex}");
	gtk_combo_box_set_active(GTK_COMBO_BOX(l_pIconCombobox),(gint)l_iIconSelected);
	
	auto setActiveFromConf = [this](const char* elementName, const char* confTokenName)
	{
		GtkWidget* element = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, elementName));
		bool tokenValue = m_rKernelContext.getConfigurationManager().expandAsBoolean(confTokenName, false);
		gtk_combo_box_set_active(GTK_COMBO_BOX(element), tokenValue);
	};
	setActiveFromConf("sg-box-inputs-modify-checkbutton", "${SkeletonGenerator_Box_CanModifyInputs}");
	setActiveFromConf("sg-box-inputs-add-checkbutton", "${SkeletonGenerator_Box_CanAddInputs}");
	setActiveFromConf("sg-box-outputs-modify-checkbutton", "${SkeletonGenerator_Box_CanModifyOutputs}");
	setActiveFromConf("sg-box-outputs-add-checkbutton", "${SkeletonGenerator_Box_CanAddOutputs}");
	//
	setActiveFromConf("sg-box-settings-modify-checkbutton", "${SkeletonGenerator_Box_CanModifySettings}");
	setActiveFromConf("sg-box-settings-add-checkbutton", "${SkeletonGenerator_Box_CanAddSettings}");

	::GtkWidget * l_pInputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-inputs-treeview"));
	::GtkTreeModel * l_pInputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pInputsTreeView));
	int64_t l_i32InputCount = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_InputCount}",0);

	auto getValueFromConfMask = [this](const char* mask, const int index) -> CString
	{
		char l_sTokenName [128];
		sprintf(l_sTokenName, mask, index);
		return m_rKernelContext.getConfigurationManager().expand(CString(l_sTokenName));
	};

	for(int32_t i = 0; i < l_i32InputCount; i++)
	{
		CString l_sName = getValueFromConfMask("${SkeletonGenerator_Box_Input%i_Name}", i);
		CString l_sType = getValueFromConfMask("${SkeletonGenerator_Box_Input%i_Type}", i);
		CString l_sTypeId = getValueFromConfMask("${SkeletonGenerator_Box_Input%i_TypeId}", i);
		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pInputsListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pInputsListStore), &l_iter, 0, (const char *)l_sName,1,(const char *)l_sType,2,(const char *)l_sTypeId,-1);
	}

	::GtkWidget * l_pOutputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-outputs-treeview"));
	::GtkTreeModel * l_pOutputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pOutputsTreeView));
	int64_t l_i32OutputCount = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_OutputCount}",0);
	for(int32_t i = 0; i < l_i32OutputCount; i++)
	{
		CString l_sName = getValueFromConfMask("${SkeletonGenerator_Box_Output%i_Name}", i);
		CString l_sType = getValueFromConfMask("${SkeletonGenerator_Box_Output%i_Type}", i);
		CString l_sTypeId = getValueFromConfMask("${SkeletonGenerator_Box_Output%i_TypeId}", i);
		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pOutputsListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pOutputsListStore), &l_iter, 0, (const char *)l_sName,1,(const char *)l_sType,2,(const char *)l_sTypeId,-1);
	}

	//
	::GtkWidget * l_pMessageInputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-message-inputs-treeview"));
	::GtkTreeModel * l_pMessageInputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pMessageInputsTreeView));
	int64_t l_i32MessageInputCount = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_MessageInputCount}",0);
	for(int32_t i = 0; i < l_i32MessageInputCount; i++)
	{
		CString l_sName = getValueFromConfMask("${SkeletonGenerator_Box_MessageInput%i_Name}", i);

		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pMessageInputsListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pMessageInputsListStore), &l_iter, 0, (const char *)l_sName,-1);
	}

	::GtkWidget * l_pMessageOutputsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-message-outputs-treeview"));
	::GtkTreeModel * l_pMessageOutputsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pMessageOutputsTreeView));
	int64_t l_i32MessageOutputCount = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_MessageOutputCount}",0);
	for(int32_t i = 0; i < l_i32MessageOutputCount; i++)
	{
		CString l_sName = getValueFromConfMask("${SkeletonGenerator_Box_MessageOutput%i_Name}",i);

		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pMessageOutputsListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pMessageOutputsListStore), &l_iter, 0, (const char *)l_sName,-1);
	}

	//

	::GtkWidget * l_pSettingsTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-settings-treeview"));
	::GtkTreeModel * l_pSettingsListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pSettingsTreeView));
	int64_t l_i32SettingCount = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_SettingCount}",0);
	for(int32_t i = 0; i < l_i32SettingCount; i++)
	{
		CString l_sName = getValueFromConfMask("${SkeletonGenerator_Box_Setting%i_Name}", i);
		CString l_sType =  getValueFromConfMask("${SkeletonGenerator_Box_Setting%i_Type}", i);
		CString l_sTypeId = getValueFromConfMask("${SkeletonGenerator_Box_Setting%i_TypeId}", i);
		CString l_sDefaultValue = getValueFromConfMask("${SkeletonGenerator_Box_Setting%i_DefaultValue}", i);
		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pSettingsListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pSettingsListStore), &l_iter, 0, (const char *)l_sName,1,(const char *)l_sType,2,(const char *)l_sDefaultValue,3,(const char *)l_sTypeId,-1);
	}
	
	::GtkWidget * l_pAlgoTreeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-treeview"));
	::GtkTreeModel * l_pAlgoListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgoTreeView));
	int64_t l_i32AlgoCount = m_rKernelContext.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Box_AlgorithmCount}",0);
	for(int32_t i = 0; i < l_i32AlgoCount; i++)
	{
		CString l_sName = getValueFromConfMask("${SkeletonGenerator_Box_Algorithm%i_Name}",i);
		GtkTreeIter l_iter;
		gtk_list_store_append(GTK_LIST_STORE(l_pAlgoListStore), &l_iter);
		gtk_list_store_set (GTK_LIST_STORE(l_pAlgoListStore), &l_iter, 0, (const char *)l_sName,-1);
	}

	::GtkWidget * l_pListenerWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-listener-checkbutton"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pListenerWidget),m_rKernelContext.getConfigurationManager().expandAsBoolean("${SkeletonGenerator_Box_UseListener}",false));

	auto setActiveFromConfTog = [this](const char* elementName, const char* confTokenName)
	{
		GtkWidget* element = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, elementName));
		bool tokenValue = m_rKernelContext.getConfigurationManager().expandAsBoolean(confTokenName, false);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(element), tokenValue);
	};
	setActiveFromConfTog("sg-box-listener-input-added-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputAdded}");
	setActiveFromConfTog("sg-box-listener-input-removed-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputRemoved}");
	setActiveFromConfTog("sg-box-listener-input-type-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputTypeChanged}");
	setActiveFromConfTog("sg-box-listener-input-name-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputNameChanged}");
	setActiveFromConfTog("sg-box-listener-input-connected-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputConnected}");
	setActiveFromConfTog("sg-box-listener-input-disconnected-checkbutton", "${SkeletonGenerator_Box_ListenerOnInputDisconnected}");
	setActiveFromConfTog("sg-box-listener-output-added-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputAdded}");
	setActiveFromConfTog("sg-box-listener-output-removed-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputRemoved}");
	setActiveFromConfTog("sg-box-listener-output-type-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputTypeChanged}");
	setActiveFromConfTog("sg-box-listener-output-name-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputNameChanged}");
	setActiveFromConfTog("sg-box-listener-output-connected-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputConnected}");
	setActiveFromConfTog("sg-box-listener-output-disconnected-checkbutton", "${SkeletonGenerator_Box_ListenerOnOutputDisconnected}");
	
	setActiveFromConfTog("sg-box-listener-setting-added-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingAdded}");
	setActiveFromConfTog("sg-box-listener-setting-removed-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingRemoved}");
	setActiveFromConfTog("sg-box-listener-setting-type-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingTypeChanged}");
	setActiveFromConfTog("sg-box-listener-setting-name-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingNameChanged}");
	setActiveFromConfTog("sg-box-listener-setting-default-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingDefaultValueChanged}");
	setActiveFromConfTog("sg-box-listener-setting-value-checkbutton", "${SkeletonGenerator_Box_ListenerOnSettingValueChanged}");

	setActiveFromConfTog("sg-box-process-input-checkbutton", "${SkeletonGenerator_Box_ProcessInput}");
	setActiveFromConfTog("sg-box-process-clock-checkbutton", "${SkeletonGenerator_Box_ProcessClock}");
	::GtkWidget* l_pProcessingMethod = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-process-frequency-spinbutton"));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(l_pProcessingMethod), (gdouble) m_rKernelContext.getConfigurationManager().expandAsUInteger("${SkeletonGenerator_Box_ProcessClock}",1));
	setActiveFromConfTog("sg-box-process-message-checkbutton", "${SkeletonGenerator_Box_ProcessMessage}");

	getLogManager() << LogLevel_Info << "box entries from [" << m_sConfigurationFile << "] loaded.\n";
	
	return true;
}

void CBoxAlgorithmSkeletonGenerator::getCurrentParameters(void){
	auto getText = [this](const char* elementName)
	{
		::GtkWidget* entry = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, elementName));
		return gtk_entry_get_text(GTK_ENTRY(entry));
	};

	m_sName = getText("sg-box-box-name-entry");
	m_sClassName = getText("sg-box-class-name-entry");
	m_sCategory = getText("sg-box-category-entry");
	m_sVersion = getText("sg-box-version-entry");
	m_sShortDescription = getText("sg-box-short-description-entry");
	
	::GtkWidget * l_pEntryDetailedDescr = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-detailed-description-textview"));
	::GtkTextBuffer * l_pTextBufferDetailedDescr = gtk_text_view_get_buffer(GTK_TEXT_VIEW(l_pEntryDetailedDescr));
	::GtkTextIter l_TextIterStart,l_TextIterEnd;
	gtk_text_buffer_get_start_iter(l_pTextBufferDetailedDescr,&l_TextIterStart);
	gtk_text_buffer_get_end_iter(l_pTextBufferDetailedDescr,&l_TextIterEnd);
	m_sDetailedDescription = gtk_text_buffer_get_text(l_pTextBufferDetailedDescr,&l_TextIterStart,&l_TextIterEnd, false);
	
	::GtkWidget * l_pIconCombobox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-icon-combobox"));
	::GtkTreeModel * l_pIconListStore = gtk_combo_box_get_model(GTK_COMBO_BOX(l_pIconCombobox));
	::GtkTreeIter l_iterIcon;
	m_i32GtkStockItemIndex = gtk_combo_box_get_active(GTK_COMBO_BOX(l_pIconCombobox)); // can be -1 if nothing selected
	if(m_i32GtkStockItemIndex != -1)
	{
		gtk_tree_model_iter_nth_child(l_pIconListStore,&l_iterIcon,NULL,m_i32GtkStockItemIndex);
		gchar * l_sData;
		gtk_tree_model_get(l_pIconListStore, &l_iterIcon,0, &l_sData,-1);
		m_sGtkStockItemName = CString((const char *)l_sData);
	}
	else
	{
		m_sGtkStockItemName = "";
	}
	auto isActive = [this](const char* widgetName) -> bool
	{
		::GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, widgetName));
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? true : false;
	};

	m_bCanModifyInputs = isActive("sg-box-inputs-modify-checkbutton");
	m_bCanAddInputs = isActive("sg-box-inputs-add-checkbutton");
	m_bCanModifyOutputs = isActive("sg-box-outputs-modify-checkbutton");
	m_bCanAddOutputs = isActive("sg-box-outputs-add-checkbutton");
	m_bCanModifySettings =isActive("sg-box-settings-modify-checkbutton");
	m_bCanAddSettings = isActive("sg-box-settings-add-checkbutton");

	//

	m_bUseBoxListener = isActive("sg-box-listener-checkbutton");
	m_bBoxListenerOnInputAdded = isActive("sg-box-listener-input-added-checkbutton");
	m_bBoxListenerOnInputRemoved = isActive("sg-box-listener-input-removed-checkbutton");
	m_bBoxListenerOnInputTypeChanged = isActive("sg-box-listener-input-type-checkbutton");
	m_bBoxListenerOnInputNameChanged = isActive("sg-box-listener-input-name-checkbutton");
	m_bBoxListenerOnInputConnected = isActive("sg-box-listener-input-connected-checkbutton");
	m_bBoxListenerOnInputDisconnected = isActive("sg-box-listener-input-disconnected-checkbutton");
	
	m_bBoxListenerOnOutputAdded = isActive("sg-box-listener-output-added-checkbutton");
	m_bBoxListenerOnOutputRemoved = isActive("sg-box-listener-output-removed-checkbutton");
	m_bBoxListenerOnOutputTypeChanged = isActive("sg-box-listener-output-type-checkbutton");
	m_bBoxListenerOnOutputNameChanged = isActive("sg-box-listener-output-name-checkbutton");
	m_bBoxListenerOnOutputConnected = isActive("sg-box-listener-output-connected-checkbutton");
	m_bBoxListenerOnOutputDisconnected = isActive("sg-box-listener-output-disconnected-checkbutton");

	m_bBoxListenerOnSettingAdded = isActive("sg-box-listener-setting-added-checkbutton");
	m_bBoxListenerOnSettingRemoved = isActive("sg-box-listener-setting-removed-checkbutton");
	m_bBoxListenerOnSettingTypeChanged = isActive("sg-box-listener-setting-type-checkbutton");
	m_bBoxListenerOnSettingNameChanged = isActive("sg-box-listener-setting-name-checkbutton");
	m_bBoxListenerOnSettingDefaultValueChanged = isActive("sg-box-listener-setting-default-checkbutton");
	m_bBoxListenerOnSettingValueChanged = isActive("sg-box-listener-setting-value-checkbutton");

	m_bProcessInput = isActive("sg-box-process-input-checkbutton");
	m_bProcessClock = isActive("sg-box-process-clock-checkbutton");
	::GtkWidget* l_pProcessingMethod = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-process-frequency-spinbutton"));
	m_ui32ClockFrequency = (uint32) gtk_spin_button_get_value(GTK_SPIN_BUTTON(l_pProcessingMethod));
	m_bProcessMessage = isActive("sg-box-process-message-checkbutton");

//	::GtkWidget * l_pUseCodecToolkitCheckbox = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-toolkit-checkbutton"));
//	m_bUseCodecToolkit = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pUseCodecToolkitCheckbox)) ? true : false);


	auto fillCollection = [this](const char* treeModelName, std::vector<IOSStruct>& collection)
	{
		::GtkWidget* treeView = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, treeModelName));
		::GtkTreeModel* treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
		GtkTreeIter iter;
		bool l_bValid = gtk_tree_model_get_iter_first(treeModel, &iter) ? true : false;
		collection.clear();
		while (l_bValid)
		{
			/* Walk through the list, reading each row */
			gchar* l_sName, *l_sType, *l_sTypeOv, *l_sDefaultValue;
			if (gtk_tree_model_get_n_columns (treeModel) == 3)
			{
				gtk_tree_model_get(treeModel, &iter,0, &l_sName,1, &l_sType,2,&l_sTypeOv,-1);
				l_sDefaultValue = nullptr;
			}
			else
			{
				gtk_tree_model_get(treeModel, &iter,0, &l_sName,1, &l_sType, 2, &l_sDefaultValue,3,&l_sTypeOv,-1);
			}

			IOSStruct l_struct;
			l_struct._name = l_sName;
			l_struct._type = l_sType;
			l_struct._typeId = l_sTypeOv;
			l_struct._defaultValue = l_sDefaultValue ? l_sDefaultValue : 0;
			collection.push_back(l_struct);

			g_free(l_sName); g_free (l_sType); g_free(l_sTypeOv); g_free(l_sDefaultValue);
			l_bValid = gtk_tree_model_iter_next (treeModel, &iter) ? true : false;
		}
	};

	fillCollection("sg-box-inputs-treeview", m_vInputs);
	fillCollection("sg-box-outputs-treeview", m_vOutputs);
	fillCollection("sg-box-settings-treeview", m_vSettings);

	::GtkWidget * l_pAlgosTreeview = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "sg-box-algorithms-treeview"));
	::GtkTreeModel * l_pAlgosListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(l_pAlgosTreeview));
	GtkTreeIter l_iterAlgo;
	bool l_bValid = gtk_tree_model_get_iter_first(l_pAlgosListStore,&l_iterAlgo) ? true : false;
	m_vAlgorithms.clear();
	while(l_bValid)
	{
		/* Walk through the list, reading each row */
		gchar * l_sName;
		gtk_tree_model_get(l_pAlgosListStore, &l_iterAlgo,0, &l_sName,-1);

		m_vAlgorithms.push_back(l_sName);

		g_free(l_sName);
		l_bValid = (gtk_tree_model_iter_next (l_pAlgosListStore, &l_iterAlgo) ? true : false);
	}
}

CString  CBoxAlgorithmSkeletonGenerator::getRandomIdentifierString(void)
{
	return CIdentifier().random().toString();
}

bool CBoxAlgorithmSkeletonGenerator::isStringValid(const char* string)
{
	std::string l_sTestString(string);
	return !(l_sTestString == "" || l_sTestString.find_first_not_of(" ")==std::string::npos);
}

