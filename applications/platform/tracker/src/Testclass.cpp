//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include "Testclass.h"

#include "Source.h"

#include "Stream.h"

using namespace OpenViBE;

TestClass::TestClass(OpenViBE::Kernel::IKernelContext& ctx) : m_ctx(ctx) 
{
	std::cout << "Testing\n";

#if 0
	StreamSignal testStream(ctx);
	StreamMatrix testStream2(ctx);

	std::vector<Stream*> list;

	list.push_back(&testStream);
	list.push_back(&testStream2);

	Stream* tmp = list[0];
	
	std::cout << "First is " << ctx.getTypeManager().getTypeName(tmp->getTypeIdentifier()) << "\n";
#endif

	const CString eegFile = OpenViBE::Directories::getDataDir() + CString("/scenarios/signals/bci-motor-imagery.ov");

	Source src;
	src.initialize(eegFile.toASCIIString());

#if 0
	// Test code illustrating how to alter stimulation stream
	for(auto it = m_Streams.begin();it != m_Streams.end(); it++)
	{
		if(it->second->getTypeIdentifier() == OV_TypeId_Stimulations)
		{
			TypeBase::Buffer *ptr = nullptr;
			it->second->peek(OpenViBE::ITimeArithmetics::secondsToTime(5.0), &ptr);
			TypeStimulation::Buffer *ptr2 = reinterpret_cast<TypeStimulation::Buffer*>(ptr);
			// std::cout << "cnt: " << ptr2->m_buffer.getStimulationCount() << "\n";

			// Request early stop
			ptr2->m_buffer.clear();
			ptr2->m_buffer.appendStimulation(OVTK_StimulationId_ExperimentStop, OpenViBE::ITimeArithmetics::secondsToTime(5.0),0);
		}
	}
#endif

#if 0
	StreamHeaderSignal signalHeader;
	signalHeader.m_samplingFrequency = 512;

	Stream<StreamHeaderSignal, StreamDataSignal> testStream(m_ctx);
	testStream.initialize(signalHeader);

	StreamDataSignal* data = new(StreamDataSignal);
	data->data.setDimensionSize(10,2);
	testStream.push(data);

	// Encode a stream
	Encoder<StreamHeaderSignal, StreamDataSignal> encoder(ctx);

	std::vector<OpenViBE::CMemoryBuffer> encoded;

	encoded.push_back( encoder.encodeHeader(testStream.getHeader()) );

	for(size_t i=0;i<testStream.m_vChunks.size(); i++)
	{
		encoded.push_back( encoder.encodeBuffer(*testStream.m_vChunks[i]) );
	}

	Stream* ptr = testStream;
#endif
}

