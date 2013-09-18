#ifndef __OpenViBEPlugins_BoxAlgorithm_TCPWriter_H__
#define __OpenViBEPlugins_BoxAlgorithm_TCPWriter_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

// The unique identifiers for the box and its descriptor.
#define OVP_ClassId_BoxAlgorithm_TCPWriter OpenViBE::CIdentifier(0x02F24947, 0x17FA0477)
#define OVP_ClassId_BoxAlgorithm_TCPWriterDesc OpenViBE::CIdentifier(0x3C32489D, 0x46F565D3)

#define OVP_TypeID_TCPWriter_OutputStyle OpenViBE::CIdentifier(0x6D7E53DD, 0x6A0A4753)

enum { TCPWRITER_RAW, TCPWRITER_HEX, TCPWRITER_STRING }; // stimulation output types

namespace OpenViBEPlugins
{
	namespace NetworkIO
	{
		/**
		 * \class CBoxAlgorithmTCPWriter
		 * \author Jussi T. Lindgren (Inria)
		 * \date Wed Sep 11 12:55:22 2013
		 * \brief The class CBoxAlgorithmTCPWriter describes the box TCP Writer.
		 *
		 */
		class CBoxAlgorithmTCPWriter : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
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
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_TCPWriter);

		protected:

			OpenViBE::boolean sendToClients(void *pBuffer, OpenViBE::uint32 ui32BufferLength);

			// Stream decoder
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmTCPWriter > m_StimulationDecoder;
			OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmTCPWriter > m_MatrixDecoder;
			OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmTCPWriter > m_SignalDecoder;
			OpenViBEToolkit::TDecoder < CBoxAlgorithmTCPWriter > *m_pActiveDecoder;

			boost::asio::io_service m_oIOService;
			boost::asio::ip::tcp::acceptor* m_pAcceptor;
			std::vector<boost::asio::ip::tcp::socket*> m_vSockets;

			OpenViBE::uint64 m_ui64OutputStyle;

			OpenViBE::CIdentifier m_oInputType;

			// Data written as global output header, 8*4 = 32 bytes. Padding allows dumb readers to step with float64 (==8 bytes).
			OpenViBE::uint32 m_ui32RawVersion;					// in network byte order, version of the raw stream
			OpenViBE::uint32 m_ui32Endianness;					// in network byte order, 0==unknown, 1==little, 2==big, 3==pdp
			OpenViBE::uint32 m_ui32Frequency;					// this and the rest are in host byte order
			OpenViBE::uint32 m_ui32NumberOfChannels;
			OpenViBE::uint32 m_ui32NumberOfSamplesPerChunk;
			OpenViBE::uint32 m_ui32Reserved0;
			OpenViBE::uint32 m_ui32Reserved1;
			OpenViBE::uint32 m_ui32Reserved2;

			void startAccept();
			void handleAccept(const boost::system::error_code& ec, boost::asio::ip::tcp::socket* pSocket);
		};

		class CBoxAlgorithmTCPWriterListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
			public:

			virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) {
				OpenViBE::CIdentifier l_oNewInputType;
				rBox.getInputType(ui32Index, l_oNewInputType);
				if(l_oNewInputType != OV_TypeId_StreamedMatrix && 
				   l_oNewInputType != OV_TypeId_Signal &&
				   l_oNewInputType != OV_TypeId_Stimulations) 
				{
					this->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Only StreamedMatrix, Signal and Stimulation are supported\n";
					rBox.setInputType(ui32Index, OV_TypeId_StreamedMatrix);
					return false;
				}
				// Set output type to RAW if the new input is not stimulations
				if(l_oNewInputType != OV_TypeId_Stimulations) 
				{
					rBox.setSettingValue(1, "Raw");
				}
				return true;
			}

			virtual OpenViBE::boolean onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) {
				// Test that output type is RAW if the input are not stimulations
				if(ui32Index == 1) 
				{
					OpenViBE::CIdentifier l_oCurrentInputType;
					OpenViBE::CString l_sNewValue;

					rBox.getInputType(0, l_oCurrentInputType);
					rBox.getSettingValue(1, l_sNewValue);
					if(l_oCurrentInputType != OV_TypeId_Stimulations && l_sNewValue != OpenViBE::CString("Raw"))
					{
						rBox.setSettingValue(1, "Raw");
						this->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Only RAW output accepted for this input type\n";
						return false;
					}
				}
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);

		};

		/**
		 * \class CBoxAlgorithmTCPWriterDesc
		 * \author Jussi T. Lindgren (Inria)
		 * \date Wed Sep 11 12:55:22 2013
		 * \brief Descriptor of the box TCP Writer.
		 *
		 */
		class CBoxAlgorithmTCPWriterDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("TCP Writer"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Send input stream out via a TCP socket"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("\n"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Network-io"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-jump-to"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_TCPWriter; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::NetworkIO::CBoxAlgorithmTCPWriter; }
			
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmTCPWriterListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Input 1",OV_TypeId_StreamedMatrix);
				
				rBoxAlgorithmPrototype.addSetting("Port",OV_TypeId_Integer,"5678");
				rBoxAlgorithmPrototype.addSetting("Stimulus output", OVP_TypeID_TCPWriter_OutputStyle, "Raw");

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_TCPWriterDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_TCPWriter_H__
