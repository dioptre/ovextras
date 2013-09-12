#ifndef __OpenViBEPlugins_BoxAlgorithm_StimulusTCPWriter_H__
#define __OpenViBEPlugins_BoxAlgorithm_StimulusTCPWriter_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

// The unique identifiers for the box and its descriptor.
#define OVP_ClassId_BoxAlgorithm_StimulusTCPWriter OpenViBE::CIdentifier(0x6F661883, 0x5EDB3AF6)
#define OVP_ClassId_BoxAlgorithm_StimulusTCPWriterDesc OpenViBE::CIdentifier(0x7CAF4B16, 0x24190F9B)

#define OVP_TypeID_StimulusTCPWriter_OutputStyle OpenViBE::CIdentifier(0x45A451E3, 0x41CF1ABE)

enum { TCPWRITER_RAW, TCPWRITER_HEX, TCPWRITER_STRING }; // output types

namespace OpenViBEPlugins
{
	namespace NetworkIO
	{
		/**
		 * \class CBoxAlgorithmStimulusTCPWriter
		 * \author Jussi T. Lindgren (Inria)
		 * \date Wed Sep 11 12:55:22 2013
		 * \brief The class CBoxAlgorithmStimulusTCPWriter describes the box TCP Writer.
		 *
		 */
		class CBoxAlgorithmStimulusTCPWriter : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			//Here is the different process callbacks possible
			// - On clock ticks :
			//virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			// - On new input received (the most common behaviour for signal processing) :
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			// If you want to use processClock, you must provide the clock frequency.
			//virtual OpenViBE::uint64 getClockFrequency(void);
			
			virtual OpenViBE::boolean process(void);

			// As we do with any class in openvibe, we use the macro below 
			// to associate this box to an unique identifier. 
			// The inheritance information is also made available, 
			// as we provide the superclass OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_StimulusTCPWriter);

		protected:
			// Stimulation stream decoder
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmStimulusTCPWriter > m_oAlgo0_StimulationDecoder;

			boost::asio::io_service m_oIOService;
			boost::asio::ip::tcp::acceptor* m_pAcceptor;
			std::vector<boost::asio::ip::tcp::socket*> m_vSockets;

			OpenViBE::uint64 m_ui64OutputStyle;

			void startAccept();
			void handleAccept(const boost::system::error_code& ec);
		};

		/**
		 * \class CBoxAlgorithmStimulusTCPWriterDesc
		 * \author Jussi T. Lindgren (Inria)
		 * \date Wed Sep 11 12:55:22 2013
		 * \brief Descriptor of the box TCP Writer.
		 *
		 */
		class CBoxAlgorithmStimulusTCPWriterDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Stimulus TCP Writer"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Stream stimuli out to TCP"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("\n"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Network-io"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-jump-to"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_StimulusTCPWriter; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::NetworkIO::CBoxAlgorithmStimulusTCPWriter; }
			
			/*
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmStimulusTCPWriterListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			*/
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Stimulations",OV_TypeId_Stimulations);
				
				rBoxAlgorithmPrototype.addSetting("Port",OV_TypeId_Integer,"5678");
				rBoxAlgorithmPrototype.addSetting("Output type", OVP_TypeID_StimulusTCPWriter_OutputStyle, "String");

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_StimulusTCPWriterDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_StimulusTCPWriter_H__
