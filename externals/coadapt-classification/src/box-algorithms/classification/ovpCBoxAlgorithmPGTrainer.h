#ifndef __OpenViBEPlugins_BoxAlgorithm_PGTrainer_H__
#define __OpenViBEPlugins_BoxAlgorithm_PGTrainer_H__

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "ovpCBoxAlgorithmCommonClassifierListener.inl"

#include <map>
#include <vector>
#include <iostream>
#include <stack>
#include <set>
#include "ovp_global_defines.h"

#include <xml/IWriter.h>
#include <xml/IReader.h>
#include <itpp/itbase.h>

#define OVP_ClassId_BoxAlgorithm_PGTrainer                                    OpenViBE::CIdentifier(0x64641646, 0x378102DC)
#define OVP_ClassId_BoxAlgorithm_PGTrainerDesc                                OpenViBE::CIdentifier(0x5895213D, 0x564304E2)
/*
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x5FDD6A98, 0x46D00E9D)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x6CC410D6, 0x0B803F96)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x6D0525D3, 0x1D8A5FB5)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x2E3410E1, 0x16C871F9)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x75A4571B, 0x28BF5F6C)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x4F883DF6, 0x04DA4E57)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x59587C19, 0x19F679B2)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x0B776C6F, 0x4B231CDE)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x4612777A, 0x03A94797)
#define OV_ClassId_                                        OpenViBE::CIdentifier(0x254B26EA, 0x4C4A2E24)
//*/
#define OVP_BoxAlgorithm_PGTrainer_CommonSettingsCount 4

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CBoxAlgorithmPGTrainer : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, public XML::IWriterCallback, public XML::IReaderCallback
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);

			//virtual OpenViBE::boolean classify(const OpenViBEToolkit::IFeatureVector& rFeatureVector, OpenViBE::float64& rf64Class, OpenViBEToolkit::IVector& rClassificationValues);

			virtual OpenViBE::boolean saveConfiguration(OpenViBE::IMemoryBuffer& rMemoryBuffer);
			virtual OpenViBE::boolean loadConfiguration(const OpenViBE::IMemoryBuffer& rMemoryBuffer);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_PGTrainer);

		protected:

			//virtual OpenViBE::boolean train(const size_t uiStartIndex, const size_t uiStopIndex);
			//virtual OpenViBE::float64 getAccuracy(const size_t uiStartIndex, const size_t uiStopIndex);

			//

			virtual void write(const char* sString); // XML IWriterCallback

			virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
			virtual void processChildData(const char* sData); // XML IReaderCallback
			virtual void closeChild(void); // XML ReaderCallback
			virtual OpenViBE::boolean checkInputs();

		protected:
			//feature vector decoder
			std::vector < OpenViBE::uint32 > m_vFeatureCount;
			std::vector < OpenViBE::uint32 > m_vFeatureVectorIndex;
			std::map < OpenViBE::uint32, OpenViBE::Kernel::IAlgorithmProxy*> m_vFeatureVectorsDecoder;
			//stimulations decoder
			OpenViBE::Kernel::IAlgorithmProxy* m_pStimulationsDecoder;

			OpenViBE::Kernel::IAlgorithmProxy* m_pClassifier;
			OpenViBE::uint64 m_ui64TrainStimulation;
			//OpenViBE::uint64 m_ui64PartitionCount;
			// ----------------- errp --------------------------------
			//decoder
			std::vector < OpenViBE::Kernel::IAlgorithmProxy* > m_vStimulationsDecoder;
			std::vector < OpenViBE::Kernel::TParameterHandler<OpenViBE::IStimulationSet*> > op_pStimulationSet;
			std::vector < OpenViBE::Kernel::TParameterHandler<const OpenViBE::IMemoryBuffer*> > ip_pEncodedMemoryBuffer;
			//variables
			OpenViBE::boolean m_bCheckInput;
			OpenViBE::boolean m_bTrainStimulationReceived;
			OpenViBE::boolean m_bIsLineChose;
			OpenViBE::boolean m_bIsColumnChose;
			OpenViBE::boolean m_bIsAnswerCorrect;
			OpenViBE::boolean m_bPreviousResultValidation;
			OpenViBE::uint64 m_ui64ChosenLineIdentifier;
			OpenViBE::uint64 m_ui64ChosenColumnIdentifier;
			OpenViBE::uint64 m_ui64PreviousResultLine;
			OpenViBE::uint64 m_ui64PreviousResultColumn;
			//early stopping management
			OpenViBE::uint32 m_ui32NumberOfRepetitions;
			OpenViBE::uint64 m_ui64NumberOfRows;
			OpenViBE::uint64 m_ui64NumberOfColumns;
			//early stopping, mapping of the feature vector time
			//std::map < OpenViBE::uint32, OpenViBE::float64 > m_mRepetitionTimeMapping;
			// ----------------- errp --------------------------------
			OpenViBE::Kernel::IAlgorithmProxy* m_pStimulationsEncoder;

			typedef struct
			{
				OpenViBE::CMatrix* m_pFeatureVectorMatrix;
				OpenViBE::uint64 m_ui64StartTime;
				OpenViBE::uint64 m_ui64EndTime;
				OpenViBE::uint32 m_ui32InputIndex;
				OpenViBE::uint32 m_ui32RepetitionIndex;
			} SFeatureVector;

			std::vector < CBoxAlgorithmPGTrainer::SFeatureVector > m_vFeatureVector;

			//

			std::stack<OpenViBE::CString> m_vNode;

			OpenViBE::float64 m_f64Class1;
			OpenViBE::float64 m_f64Class2;
			OpenViBE::float64 m_f64Temperature;

			OpenViBE::CMemoryBuffer m_oConfiguration;
			OpenViBE::CMemoryBuffer m_oBoxConfiguration;
			itpp::vec m_oCoefficients;
			//
			itpp::vec m_vMean0;			
			//
			itpp::Array<itpp::vec> m_vMeans;
			itpp::vec m_vSoftmaxCoefficients;
			itpp::vec m_vPiLetterCoefficients;
			itpp::vec m_vPiFlashCoefficients;
			OpenViBE::float64 m_f64Lambda;
			OpenViBE::float64 m_f64Etha;
			OpenViBE::boolean m_bAreCoefficientsDefine;

			//splotch
			std::map <OpenViBE::uint32,std::set<int> > m_mLettersGroups;
			OpenViBE::uint32 m_ui32LetterGroupId;

		public:
			virtual OpenViBE::boolean Algotrain(std::vector < CBoxAlgorithmPGTrainer::SFeatureVector > m_vFeatureVector);//(const OpenViBEToolkit::IFeatureVectorSet& rFeatureVectorSet);
			
		};

		class CBoxAlgorithmPGTrainerDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("PG Trainer"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Lo�c MAHE"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("ECM/INSERM"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Trainer specifically designed for PG in P300 scenario"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Performs multiple training on the feature vector set leaving a single feature vector each time and tests this feature vector on the trained classifier"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Classification"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-apply"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_PGTrainer; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CBoxAlgorithmPGTrainer; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput  ("Stimulations",                         OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput  ("ErrP input",								  OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput  ("Line 1",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Line 2",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Line 3",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Line 4",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Line 5",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Line 6",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Column 1",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Column 2",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Column 3",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Column 4",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Column 5",                 OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput  ("Column 6",                 OV_TypeId_FeatureVector);
				
				

				rBoxAlgorithmPrototype.addOutput ("Train-completed Flag",                 OV_TypeId_Stimulations);

				rBoxAlgorithmPrototype.addSetting("Filename to save configuration to",    OV_TypeId_Filename,                  "");
				rBoxAlgorithmPrototype.addSetting("Train trigger",                        OV_TypeId_Stimulation,               "OVTK_StimulationId_Train");
				rBoxAlgorithmPrototype.addSetting("Eta", OV_TypeId_Float,                   "0.1");
				rBoxAlgorithmPrototype.addSetting("Lambda", OV_TypeId_Float,                   "0.1");
				rBoxAlgorithmPrototype.addSetting("Number of rows", OV_TypeId_Integer,                   "6");
				rBoxAlgorithmPrototype.addSetting("Number of columns", OV_TypeId_Integer,                   "6");
				rBoxAlgorithmPrototype.addSetting("Filename to load group of letters from",    OV_TypeId_Filename,                  "");
				rBoxAlgorithmPrototype.addSetting("Temperature parameter",    OV_TypeId_Float,                  "0.01");

				rBoxAlgorithmPrototype.addFlag   (OpenViBE::Kernel::BoxFlag_CanAddInput);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				return true;
			}

			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const { return new CBoxAlgorithmCommonClassifierListener(OVP_BoxAlgorithm_PGTrainer_CommonSettingsCount); }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) { delete pBoxListener; }

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_PGTrainerDesc);
		};
	};
};
#endif // __OpenViBEPlugins_BoxAlgorithm_PGTrainer_H__
