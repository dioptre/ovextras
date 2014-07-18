#ifndef __OpenViBEPlugins_BoxAlgorithm_CommonClassifierListener_INL__
#define __OpenViBEPlugins_BoxAlgorithm_CommonClassifierListener_INL__

#include "../ovp_defines.h"
#include "../algorithms/ovpCAlgorithmClassifierOneVsOne.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <iostream>

// #define OV_DEBUG_CLASSIFIER_LISTENER

#ifdef OV_DEBUG_CLASSIFIER_LISTENER
#define DEBUG_PRINT(x) x
#else
#define DEBUG_PRINT(x) 
#endif

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CBoxAlgorithmCommonClassifierListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			CBoxAlgorithmCommonClassifierListener(const OpenViBE::uint32 ui32CustomSettingBase)
				:m_ui32CustomSettingBase(ui32CustomSettingBase)
			{
			}

			virtual OpenViBE::boolean initialize(void)
			{
				m_oClassifierClassIdentifier=OV_UndefinedIdentifier;
				m_pClassifier=NULL;

				m_oStrategyClassIdentifier=OV_UndefinedIdentifier;
				m_pStrategy = NULL;

				m_ui32StrategyAmountSettings = 0;
				return true;
			}

			virtual OpenViBE::boolean uninitialize(void)
			{
				if(m_pClassifier)
				{
					m_pClassifier->uninitialize();
					this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
					m_pClassifier=NULL;
				}
				if(m_pStrategy)
				{
					m_pStrategy->uninitialize();
					this->getAlgorithmManager().releaseAlgorithm(*m_pStrategy);
					m_pStrategy=NULL;
				}
				return true;
			}

			virtual OpenViBE::boolean onInputAddedOrRemoved(OpenViBE::Kernel::IBox& rBox)
			{
				rBox.setInputType(0, OV_TypeId_Stimulations);
				rBox.setInputName(0, "Stimulations");
				for(OpenViBE::uint32 i=1; i<rBox.getInputCount(); i++)
				{
					char l_sBuffer[1024];
					sprintf(l_sBuffer, "Features for class %i", i);
					rBox.setInputName(i, l_sBuffer);
					rBox.setInputType(i, OV_TypeId_FeatureVector);
				}
				return true;
			}

			virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				//ui32Index represent the numero of the class (because of rejected offset)
				char l_sBuffer[64];
				sprintf(l_sBuffer, "Class %d label", ui32Index);
				char l_sStimulation[64];
				sprintf(l_sStimulation, "OVTK_StimulationId_Label_%02X", ui32Index);
				rBox.addSetting(l_sBuffer, OV_TypeId_Stimulation, l_sStimulation, m_ui32CustomSettingBase+ui32Index);

				//Rename input
				return this->onInputAddedOrRemoved(rBox);
			}

			virtual OpenViBE::boolean onInputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				//First remove the removed input from settings
				rBox.removeSetting(m_ui32CustomSettingBase + ui32Index);

				//Then rename the remains inputs in settings
				for(OpenViBE::uint32 i=1 ; i<rBox.getInputCount() ; ++i)
				{
					char l_sBuffer[64];
					sprintf(l_sBuffer, "Class %d label", i);
					rBox.setSettingName(m_ui32CustomSettingBase + i, l_sBuffer);
				}

				//Then rename input
				return this->onInputAddedOrRemoved(rBox);
			}

			virtual OpenViBE::boolean onInitialized(OpenViBE::Kernel::IBox& rBox)
			{
				//First add the rejected label class
				rBox.addSetting("Reject class label", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");

				//Now added Settings for classes
				for(OpenViBE::uint32 i = 1 ; i< rBox.getInputCount() ; ++i)
				{
					char l_sBuffer[64];
					sprintf(l_sBuffer, "Class %d label", i);
					char l_sStimulation[64];
					sprintf(l_sStimulation, "OVTK_StimulationId_Label_%02X", i);
					rBox.addSetting(l_sBuffer, OV_TypeId_Stimulation, l_sStimulation);
					DEBUG_PRINT(std::cout << "Add setting (type D) " << l_sBuffer << " " << l_sStimulation << "\n";)
				}
				return this->onAlgorithmClassifierChanged(rBox);
			}

			virtual OpenViBE::boolean onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				if(ui32Index == 1){
					return this->onAlgorithmClassifierChanged(rBox);
				}
				else if(ui32Index == 0){
					return this->onStrategyChanged(rBox);
				}
				else
					return true;
			}

			virtual OpenViBE::boolean updateDecision(OpenViBE::Kernel::IBox& rBox){
				OpenViBE::uint32 i=m_ui32CustomSettingBase + rBox.getInputCount();
				if(m_oStrategyClassIdentifier == OVP_ClassId_Algorithm_ClassifierOneVsOne){
					OpenViBE::CIdentifier l_oEnum = getAvailableDecisionEnumeration(m_oClassifierClassIdentifier);
					if(l_oEnum == OV_UndefinedIdentifier){
						this->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Unable to find Pairwise Decision for the algorithm" << m_oClassifierClassIdentifier.toString() << "\n";
						return false;
					}
					else
					{
						OpenViBE::Kernel::IParameter* l_pParameter=m_pStrategy->getInputParameter(OVP_Algorithm_OneVsOneStrategy_InputParameterId_DecisionType);
						OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64Parameter(l_pParameter);

						OpenViBE::CString l_sEnumTypeName=this->getTypeManager().getTypeName(l_oEnum);
						OpenViBE::uint64 l_ui64EnumValue = ip_ui64Parameter;
						OpenViBE::CString l_sEnumValue;
						if(l_ui64EnumValue) 
						{
							l_sEnumValue = this->getTypeManager().getEnumerationEntryNameFromValue(l_oEnum, l_ui64EnumValue);
						}
						else
						{
							// @fixme Initialize. This is the second place with this check (search INITDECISIONENUM). 
							// This either may not be the right place to initialize this. However, if the enum value here is 0, the
							// gui will get an empty default value...
							this->getTypeManager().getEnumerationEntry(l_oEnum, 0, l_sEnumValue, l_ui64EnumValue);
							ip_ui64Parameter = l_ui64EnumValue;
						}

						rBox.setSettingType(i, l_oEnum);
						rBox.setSettingName(i, l_sEnumTypeName);
						rBox.setSettingValue(i, l_sEnumValue);
					}
				}
				return true;
			}

			virtual OpenViBE::boolean onStrategyChanged(OpenViBE::Kernel::IBox& rBox)
			{
				OpenViBE::CString l_sStrategyName;
				OpenViBE::CIdentifier l_oStrategyIdentifier;
				OpenViBE::CIdentifier l_oOldStrategyIdentifier=m_oStrategyClassIdentifier;

				rBox.getSettingValue(0, l_sStrategyName);

				l_oStrategyIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationStrategy, l_sStrategyName);

				if(l_oStrategyIdentifier != m_oStrategyClassIdentifier)
				{
					if(m_pStrategy)
					{
						m_pStrategy->uninitialize();
						this->getAlgorithmManager().releaseAlgorithm(*m_pStrategy);
						m_pStrategy=NULL;
						m_oStrategyClassIdentifier=OV_UndefinedIdentifier;
					}
					if(l_oStrategyIdentifier != OV_UndefinedIdentifier)
					{
						m_pStrategy=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(l_oStrategyIdentifier));
						m_pStrategy->initialize();
						m_oStrategyClassIdentifier=l_oStrategyIdentifier;
					}

					if(m_ui32StrategyAmountSettings > 0)
					{
						// std::cout << m_ui32StrategyAmountSettings << std::endl;
						for(OpenViBE::uint32 i = m_ui32CustomSettingBase+rBox.getInputCount() + m_ui32StrategyAmountSettings ; i > m_ui32CustomSettingBase+rBox.getInputCount() ; --i)
						{
							DEBUG_PRINT(std::cout << "Remove pairing strategy setting at idx " << i-1 << "\n";)
							rBox.removeSetting(i-1);
						}
					}
					m_ui32StrategyAmountSettings = 0;
				}
				else//If we don't change the strategy we just have to return
				{
					return true;
				}

				if(m_pStrategy)
				{
					OpenViBE::CString l_sClassifierName;
					rBox.getSettingValue(1, l_sClassifierName);
					OpenViBE::CIdentifier l_oClassifierIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationAlgorithm, l_sClassifierName);

					OpenViBE::uint32 i=m_ui32CustomSettingBase + rBox.getInputCount();
					if(m_oStrategyClassIdentifier == OVP_ClassId_Algorithm_ClassifierOneVsOne){
						OpenViBE::CIdentifier l_oEnum = getAvailableDecisionEnumeration(l_oClassifierIdentifier);
						if(l_oEnum == OV_UndefinedIdentifier){
							this->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Unable to find Pariwise Decision for the algorithm " << m_oClassifierClassIdentifier.toString() << "\n";
							return false;
						}
						else
						{
							OpenViBE::Kernel::IParameter* l_pParameter=m_pStrategy->getInputParameter(OVP_Algorithm_OneVsOneStrategy_InputParameterId_DecisionType);
							OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64Parameter(l_pParameter);
							OpenViBE::uint64 l_ui64EnumValue = ip_ui64Parameter;
							OpenViBE::CString l_sEnumName;
							if(l_ui64EnumValue == 0) // Parameter value has not yet been set?
							{
								// @fixme This may not be the right place to initialize this (search INITDECISIONENUM), but if we don't, the default value
								// in the gui will be "", leading to trouble. We cannot initialize the param except in a place that
								// knows the classifier type and can ask which strategies work for it.
								this->getTypeManager().getEnumerationEntry(l_oEnum, 0, l_sEnumName, l_ui64EnumValue);
								ip_ui64Parameter = l_ui64EnumValue;	// initialize
							}
							else
							{
								l_sEnumName = this->getTypeManager().getEnumerationEntryNameFromValue(l_oEnum, l_ui64EnumValue);
							}

							OpenViBE::CString l_sParameterName=this->getTypeManager().getTypeName(l_oEnum);

							DEBUG_PRINT(std::cout << "Adding setting (case C) " << l_sParameterName << " : '" << l_sEnumName << "' to index " << i << "\n";)
							rBox.addSetting(l_sParameterName, l_oEnum, l_sEnumName, i);

							++m_ui32StrategyAmountSettings;
						}
					}
				}

				return true;
			}

			virtual OpenViBE::boolean onAlgorithmClassifierChanged(OpenViBE::Kernel::IBox& rBox)
			{
				OpenViBE::CString l_sClassifierName;
				OpenViBE::CIdentifier l_oClassifierIdentifier;
				OpenViBE::CIdentifier l_oOldClassifierIdentifier=m_oClassifierClassIdentifier;
				OpenViBE::CIdentifier l_oIdentifier;

				rBox.getSettingValue(1, l_sClassifierName);
				l_oClassifierIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationAlgorithm, l_sClassifierName);
				if(l_oClassifierIdentifier != m_oClassifierClassIdentifier)
				{
					if(m_pClassifier)
					{
						m_pClassifier->uninitialize();
						this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
						m_pClassifier=NULL;
						m_oClassifierClassIdentifier=OV_UndefinedIdentifier;
					}
					if(l_oClassifierIdentifier != OV_UndefinedIdentifier)
					{
						m_pClassifier=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(l_oClassifierIdentifier));
						m_pClassifier->initialize();
						m_oClassifierClassIdentifier=l_oClassifierIdentifier;
					}

					if(l_oOldClassifierIdentifier != OV_UndefinedIdentifier)
					{
						while(rBox.getSettingCount()>m_ui32CustomSettingBase+rBox.getInputCount() + m_ui32StrategyAmountSettings)
						{
							rBox.removeSetting(m_ui32CustomSettingBase + rBox.getInputCount() + m_ui32StrategyAmountSettings);
						}
					}
				}
				else//If we don't change the algorithm we just have to return
				{
					return true;
				}

				if(m_pClassifier)
				{
					OpenViBE::uint32 i=m_ui32CustomSettingBase + rBox.getInputCount() + m_ui32StrategyAmountSettings;
					while((l_oIdentifier=m_pClassifier->getNextInputParameterIdentifier(l_oIdentifier))!=OV_UndefinedIdentifier)
					{
						if((l_oIdentifier!=OVTK_Algorithm_Classifier_InputParameterId_FeatureVector)
						&& (l_oIdentifier!=OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet)
						&& (l_oIdentifier!=OVTK_Algorithm_Classifier_InputParameterId_Configuration)
						&& (l_oIdentifier!=OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter))
						{
							OpenViBE::CIdentifier l_oTypeIdentifier;
							OpenViBE::CString l_sParameterName=m_pClassifier->getInputParameterName(l_oIdentifier);
							OpenViBE::Kernel::IParameter* l_pParameter=m_pClassifier->getInputParameter(l_oIdentifier);
							OpenViBE::Kernel::TParameterHandler < OpenViBE::int64 > ip_i64Parameter(l_pParameter);
							OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64Parameter(l_pParameter);
							OpenViBE::Kernel::TParameterHandler < OpenViBE::float64 > ip_f64Parameter(l_pParameter);
							OpenViBE::Kernel::TParameterHandler < OpenViBE::boolean > ip_bParameter(l_pParameter);
							OpenViBE::Kernel::TParameterHandler < OpenViBE::CString* > ip_sParameter(l_pParameter);
							char l_sBuffer[1024];
							bool l_bValid=true;
							switch(l_pParameter->getType())
							{
								case OpenViBE::Kernel::ParameterType_Enumeration:
									::strcpy(l_sBuffer, this->getTypeManager().getEnumerationEntryNameFromValue(l_pParameter->getSubTypeIdentifier(), ip_ui64Parameter).toASCIIString());
									l_oTypeIdentifier=l_pParameter->getSubTypeIdentifier();
									break;

								case OpenViBE::Kernel::ParameterType_Integer:
								case OpenViBE::Kernel::ParameterType_UInteger:
									::sprintf(l_sBuffer, "%lli", (OpenViBE::int64)ip_i64Parameter);
									l_oTypeIdentifier=OV_TypeId_Integer;
									break;

								case OpenViBE::Kernel::ParameterType_Boolean:
									::sprintf(l_sBuffer, "%s", ((OpenViBE::boolean)ip_bParameter)?"true":"false");
									l_oTypeIdentifier=OV_TypeId_Boolean;
									break;

								case OpenViBE::Kernel::ParameterType_Float:
									::sprintf(l_sBuffer, "%lf", (OpenViBE::float64)ip_f64Parameter);
									l_oTypeIdentifier=OV_TypeId_Float;
									break;
								case OpenViBE::Kernel::ParameterType_String:
									::sprintf(l_sBuffer, "%s", ((OpenViBE::CString*)ip_sParameter)->toASCIIString());
									l_oTypeIdentifier=OV_TypeId_String;
									break;
								default:
									std::cout << "Invalid parameter type " << l_pParameter->getType() << "\n";
									l_bValid=false;
									break;
							}

							if(l_bValid)
							{
								if(i>=rBox.getSettingCount())
								{
									rBox.addSetting(l_sParameterName, l_oTypeIdentifier, l_sBuffer);
									DEBUG_PRINT(std::cout << "Adding setting (case A) " << l_sParameterName << " : " << l_sBuffer << "\n";)
								}
								else
								{
									OpenViBE::CIdentifier l_oOldTypeIdentifier;
									rBox.getSettingType(i, l_oOldTypeIdentifier);
									if(l_oOldTypeIdentifier != l_oTypeIdentifier)
									{
										rBox.setSettingType(i, l_oTypeIdentifier);
									}
									rBox.setSettingValue(i, l_sBuffer);
									rBox.setSettingName(i, l_sParameterName);
									OpenViBE::CString tmpName;  rBox.getSettingName(i, tmpName);
									DEBUG_PRINT(std::cout << "Replacing setting " << i << " : " << tmpName << " with " << l_sParameterName << " : " << l_sBuffer << "\n";)
								}
								i++;
							}
						}
					}

					while(i<rBox.getSettingCount())
					{
						DEBUG_PRINT(OpenViBE::CString tmpName;  rBox.getSettingName(i, tmpName); std::cout << "Removing setting " << i << " : " << tmpName << "\n";)
						rBox.removeSetting(i);
					}
				}

				// This changes the pairwise strategy decision voting type of the box settings allowing
				// designer to list the correct choices in the combo box.
				updateDecision(rBox);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier)

		protected:

			OpenViBE::CIdentifier m_oClassifierClassIdentifier;
			OpenViBE::CIdentifier m_oStrategyClassIdentifier;
			OpenViBE::Kernel::IAlgorithmProxy* m_pClassifier;
			OpenViBE::Kernel::IAlgorithmProxy* m_pStrategy;
			const OpenViBE::uint32 m_ui32CustomSettingBase;
			OpenViBE::uint32 m_ui32StrategyAmountSettings;
		};
	}
}

#endif // __OpenViBEPlugins_BoxAlgorithm_CommonClassifierListener_INL__
