#ifndef __OpenViBEPlugins_BoxAlgorithm_SharedMemoryWriter_H__
#define __OpenViBEPlugins_BoxAlgorithm_SharedMemoryWriter_H__

//You may have to change this path to match your folder organisation
#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

//#include <pair>
#include <vector>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/containers/string.hpp>

using namespace boost::interprocess;

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.

namespace OpenViBEPlugins
{
	namespace FileReadingAndWriting
	{
		/**
		 * \class CBoxAlgorithmSharedMemoryWriter
		 * \author Dieter Devlaminck (INRIA)
		 * \date Thu Jan 17 13:34:58 2013
		 * \brief The class CBoxAlgorithmSharedMemoryWriter describes the box SharedMemoryWriter.
		 *
		 */
		
		struct SMatrix
		{
			uint32_t rowDimension;
			uint32_t columnDimension;
			offset_ptr<OpenViBE::float64> data;
		};

		typedef allocator<char, managed_shared_memory::segment_manager> CharAllocator;
		typedef basic_string<char, std::char_traits<char>, CharAllocator> ShmString;
		typedef allocator<ShmString, managed_shared_memory::segment_manager> StringAllocator;      
		//typedef vector<ShmString, StringAllocator> MyShmStringVector;		
		typedef allocator< std::pair<const ShmString, OpenViBE::CIdentifier > , managed_shared_memory::segment_manager>  ShmemAllocatorMetaInfo;
		typedef map<ShmString, OpenViBE::CIdentifier, std::less<ShmString>, ShmemAllocatorMetaInfo> MyVectorMetaInfo;
		
		typedef allocator<uint32_t, managed_shared_memory::segment_manager>  ShmemAllocatorStimulation;
		typedef allocator<offset_ptr<SMatrix>, managed_shared_memory::segment_manager>  ShmemAllocatorMatrix;
		typedef vector<uint32_t, ShmemAllocatorStimulation> MyVectorStimulation;
		typedef vector<offset_ptr<SMatrix>, ShmemAllocatorMatrix> MyVectorStreamedMatrix;	

		class CBoxAlgorithmSharedMemoryWriter : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			
			virtual void release(void) { delete this; }

			virtual bool initialize(void);
			virtual bool uninitialize(void);
				
			//Here is the different process callbacks possible
			// - On clock ticks :
			//virtual bool processClock(OpenViBE::CMessageClock& rMessageClock);
			// - On new input received (the most common behaviour for signal processing) :
			virtual bool processInput(uint32_t ui32InputIndex);
			
			// If you want to use processClock, you must provide the clock frequency.
			//virtual OpenViBE::uint64 getClockFrequency(void);
			
			virtual bool process(void);

			// As we do with any class in openvibe, we use the macro below 
			// to associate this box to an unique identifier. 
			// The inheritance information is also made available, 
			// as we provide the superclass OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_SharedMemoryWriter);

		protected:
			// Codec algorithms specified in the skeleton-generator:
			// Stimulation stream decoder
			//OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmSharedMemoryWriter > m_oAlgo0_StimulationDecoder;
			//OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmSharedMemoryWriter > m_oAlgo0_StreamedMatrixDecoder;
			std::vector<OpenViBEToolkit::TDecoder < CBoxAlgorithmSharedMemoryWriter >* > m_vDecoder;


			OpenViBE::CString m_sSharedMemoryName;
			managed_shared_memory m_oSharedMemoryArray;
			//uint32_t * m_pInputStimuliSet;
			//MyVector * m_pInputStimuliSet;
			//uint32_t m_ui32InputCounter;


		private:
			OpenViBE::CIdentifier m_TypeIdentifier;

			OpenViBE::CString m_sMutexName;
			named_mutex* m_oMutex;
			std::vector<MyVectorStimulation *> m_vStimuliSet;
			std::vector<MyVectorStreamedMatrix *> m_vStreamedMatrix;
		};


		// If you need to implement a box Listener, here is a sekeleton for you.
		// Use only the callbacks you need.
		// For example, if your box has a variable number of input, but all of them must be stimulation inputs.
		// The following listener callback will ensure that any newly added input is stimulations :
		/*		
		virtual bool onInputAdded(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index)
		{
			rBox.setInputType(ui32Index, OV_TypeId_Stimulations);
		};
		*/
		
		
		// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
		// Please uncomment below the callbacks you want to use.
		class CBoxAlgorithmSharedMemoryWriterListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			//virtual bool onInitialized(OpenViBE::Kernel::IBox& rBox) { return true; };
			//virtual bool onNameChanged(OpenViBE::Kernel::IBox& rBox) { return true; };
			//virtual bool onInputConnected(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onInputDisconnected(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			virtual bool onInputAdded(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			virtual bool onInputRemoved(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			virtual bool onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index)
			{ 
			
				return true; 
			};
			//virtual bool onInputNameChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onOutputConnected(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onOutputDisconnected(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onOutputAdded(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onOutputRemoved(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onOutputNameChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onSettingAdded(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onSettingRemoved(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onSettingTypeChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onSettingNameChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onSettingDefaultValueChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };
			//virtual bool onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const uint32_t ui32Index) { return true; };

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};
		

		/**
		 * \class CBoxAlgorithmSharedMemoryWriterDesc
		 * \author Dieter Devlaminck (INRIA)
		 * \date Thu Jan 17 13:34:58 2013
		 * \brief Descriptor of the box SharedMemoryWriter.
		 *
		 */
		class CBoxAlgorithmSharedMemoryWriterDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("SharedMemoryWriter"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Stream input to shared memory"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("The box writes input to shared memory so that it can be read by another process. Stimuli and streamed matrices are supported, and transformed into a format that can be written into shared memory. Based on the input types, a metainfo variable will be created in shared memory that will specify which variables have which type. This way the client can know what it will be reading."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("File reading and writing"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(""); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_SharedMemoryWriter; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::FileReadingAndWriting::CBoxAlgorithmSharedMemoryWriter; }
			
			
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmSharedMemoryWriterListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			
			virtual bool getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("prediction1",OV_TypeId_StreamedMatrix);
				//rBoxAlgorithmPrototype.addInput("prediction2",OV_TypeId_Stimulations);


				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanRemoveInput);
				
				rBoxAlgorithmPrototype.addSetting("SharedMemoryName",OV_TypeId_String,"SharedMemory_P300Stimulator");
				
				rBoxAlgorithmPrototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);
				
				rBoxAlgorithmPrototype.addInputSupport(OV_TypeId_StreamedMatrix);
				rBoxAlgorithmPrototype.addInputSupport(OV_TypeId_Stimulations);

				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_SharedMemoryWriterDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_SharedMemoryWriter_H__
