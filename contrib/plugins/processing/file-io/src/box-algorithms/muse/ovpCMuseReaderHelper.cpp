#ifdef TARGET_HAS_Protobuf

#include "ovpCMuseReaderHelper.h"

#include <openvibe/ovITimeArithmetics.h>
#include <toolkit/ovtk_stimulations.h>

#include <iostream>
#include <sstream>
#include <vector>

#include <boost/regex.hpp>

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace Muse;
using namespace interaxon::muse_data;

/**
* Parse the header information for the next message.  This is called prior to the request
* for the next message, so that EOF and stream validity may be detected in advance.
**/
void CMuseReaderHelper::parseMessageHeader()
{
	// message length
	int32 l_i32MsgLength;
	m_rMuseStream.read(reinterpret_cast<char*>(&l_i32MsgLength), 4);
	
	// message type
	int16 l_i16MsgType;
	m_rMuseStream.read(reinterpret_cast<char*>(&l_i16MsgType), 2);

	// validate
	if (!messageAvailable()) {
		// read failed, EOF
		m_oMsgBuffer.clear();
		return;
	}

	if (l_i16MsgType != 2)
		throw ParseError("Only message type 2 supported.");

	// store
	m_oMsgBuffer.resize(l_i32MsgLength);
}

void CMuseReaderHelper::parseMessage()
{
	if (!messageAvailable())
		throw ParseError("No more messages.");

	m_rMuseStream.read(m_oMsgBuffer.data(), m_oMsgBuffer.size());

	bool l_bStatus = m_oCollection.ParseFromArray(m_oMsgBuffer.data(), m_oMsgBuffer.size());
	if (l_bStatus == false)
		throw ParseError("Failed to parse message body");

	parseMessageHeader();
}

CMuseReaderHelper::CMuseReaderHelper(istream & rInputStream, uint64 ui64DefaultEEGSampleRate, int64 i64MaxClockSkew)
: m_rMuseStream(rInputStream),
	m_ui64EEGTime(0),
	m_ui64EEGSampleRate(ui64DefaultEEGSampleRate),
	m_ui64EEGSamplePeriod(ITimeArithmetics::sampleCountToTime(ui64DefaultEEGSampleRate, 1)),
	m_i64MaxClockSkew(i64MaxClockSkew),
	m_ui32EEGChannelCount(0),
	m_bCorrectState(true),
	m_iCollectionIndex(-1)
{
	parseMessageHeader();
}

CMuseReaderHelper::~CMuseReaderHelper()
{
}

bool CMuseReaderHelper::parseAnnotation(MuseData const & rAnnotationData, ILogManager & rLogManager)
{
	Annotation l_oAnnotation = rAnnotationData.GetExtension(Annotation::museData);
	string l_sEventData = l_oAnnotation.event_data();
	istringstream l_oEventDataStream(l_sEventData);
	string l_sEventName;

	l_oEventDataStream >> l_sEventName;

	// ignore hardware-generated metrics
	string l_sIgnored[] =
	{
		"/muse/elements/alpha_absolute",
		"/muse/elements/beta_absolute",
		"/muse/elements/gamma_absolute",
		"/muse/elements/delta_absolute",
		"/muse/elements/theta_absolute",
		"/muse/elements/mellow",
		"/muse/elements/concentration"
	};
	for (size_t l_i = 0; l_i < sizeof(l_sIgnored) / sizeof(l_sIgnored[0]); ++ l_i)
		if (l_sEventName == l_sIgnored[l_i])
			return true;

	// capture markers
	string l_sEventNamePrefix = l_sEventName;
	l_sEventNamePrefix.resize(8);
	if (l_sEventNamePrefix == "/Marker/") {
		uint64 stimulation = l_sEventName[8] - '0' + OVTK_StimulationId_Label_00;
		m_oStimulations.push_back(stimulation);
		m_oStimulationTimes.push_back(ITimeArithmetics::secondsToTime(rAnnotationData.timestamp()));
	} else {
		rLogManager << LogLevel_ImportantWarning << "Unrecognized .muse event: " << l_sEventData.c_str() << "\n";
	}

	return true;
}

bool CMuseReaderHelper::parseEEGSample(MuseData const & rEEGData, uint32 ui32Count, ILogManager & rLogManager)
{
	EEG l_oEEG = rEEGData.GetExtension(EEG::museData);
	uint32 l_ui32nValues = l_oEEG.values_size();
	uint64 l_ui64PacketTime = ITimeArithmetics::secondsToTime(rEEGData.timestamp()); 

	if (l_oEEG.has_drl())
		++ l_ui32nValues;
	if (l_oEEG.has_ref())
		++ l_ui32nValues;

	// Stored as a packed array, so this means this is the first sample in this buffer
	if (m_oEEGSamples.empty())
	{
		m_ui32EEGChannelCount = l_ui32nValues;
		if (m_ui64EEGTime == 0)
		{
			m_ui64EEGTime = l_ui64PacketTime - m_i64MaxClockSkew / 2;
		}
	}
	else if (l_ui32nValues != m_ui32EEGChannelCount)
	{
		// channel count changes, close buffer
		return false;
	}

	uint64 l_ui64PredictedTime = m_ui64EEGTime + getEEGSampleCount() * getEEGSamplePeriod();
	int64 l_i64DeltaTime;
	if (l_ui64PacketTime < l_ui64PredictedTime)
	{
		l_i64DeltaTime = - static_cast<int64>(l_ui64PredictedTime - l_ui64PacketTime);
	}
	else
	{
		l_i64DeltaTime = l_ui64PacketTime - l_ui64PredictedTime;
	}

	if (l_i64DeltaTime > m_i64MaxClockSkew)
	{
		// dropped packets, insert 0's

		l_i64DeltaTime -= m_i64MaxClockSkew - getEEGSamplePeriod();
		uint64 l_ui64DroppedSamples = l_i64DeltaTime / getEEGSamplePeriod();
	
		if (m_bCorrectState) {
			rLogManager << LogLevel_Warning << "Clock skew: missing " << l_ui64DroppedSamples << " sample(s)\n";

			m_oStimulations.push_back(OVTK_GDF_Incorrect);
			m_oStimulationTimes.push_back(l_ui64PredictedTime);
			m_bCorrectState = false;
		}
		
		for (; l_ui64DroppedSamples > 0; -- l_ui64DroppedSamples)
		{
			for (uint32 l_ui32Channel = 0; l_ui32Channel < m_ui32EEGChannelCount; ++ l_ui32Channel)
			{
				m_oEEGSamples.push_back(0);
			}
			
			if (m_oEEGSamples.size() >= m_ui32EEGChannelCount * ui32Count)
			{
				return false;
			}
		}
		
		l_ui64PredictedTime = m_ui64EEGTime + getEEGSampleCount() * getEEGSamplePeriod();
	}
	else if (l_i64DeltaTime < -m_i64MaxClockSkew)
	{
		// extra packets, drop them

		l_i64DeltaTime += m_i64MaxClockSkew - getEEGSamplePeriod();
		if (m_bCorrectState)
		{
			uint64 l_ui64ExtraSamples = -l_i64DeltaTime / getEEGSamplePeriod();
			rLogManager << LogLevel_Warning << "Clock skew: dropping " << l_ui64ExtraSamples << " sample(s)\n";
	
			m_oStimulations.push_back(OVTK_GDF_Incorrect);
			m_oStimulationTimes.push_back(l_ui64PacketTime);
			m_bCorrectState = false;
		}
		return true;
	}

	if (!m_bCorrectState)
	{
		m_oStimulations.push_back(OVTK_GDF_Correct);
		m_oStimulationTimes.push_back(l_ui64PredictedTime);
		m_bCorrectState = true;
	}
	 
	for (int l_i = 0; l_i < l_oEEG.values_size(); ++ l_i)
	{
		m_oEEGSamples.push_back(l_oEEG.values(l_i));
	}

	if (l_oEEG.has_drl())
	{
		m_oEEGSamples.push_back(l_oEEG.drl());
	}
	if (l_oEEG.has_ref())
	{
		m_oEEGSamples.push_back(l_oEEG.ref());
	}
	
	return true;
}

void CMuseReaderHelper::parseSamples(uint32 ui32Count, ILogManager & rLogManager)
{
	m_oStimulations.clear();
	m_oStimulationTimes.clear();
	if (m_ui64EEGTime != 0)
	{
		m_ui64EEGTime += getEEGSampleCount() * getEEGSamplePeriod();
	}
	m_oEEGSamples.clear();

	// Handling of m_iCollectionIndex:
	// -1 prior to reading any messages
	// if an undesired sample is read, value is unchanged; sample will be re-read later
	// when a sample is consumed, value is incremented
	// when value is invalid (<-1,>size), new message is read, value is set to 0

	while (m_ui32EEGChannelCount == 0 || m_oEEGSamples.size() / m_ui32EEGChannelCount < ui32Count)
	{
		if (m_iCollectionIndex < 0 || m_iCollectionIndex >= m_oCollection.collection_size()) {
			if (!messageAvailable())
				break;

			parseMessage();
			m_iCollectionIndex = 0;
			continue;
		}

		MuseData const & l_rData = m_oCollection.collection(m_iCollectionIndex);
		bool l_bParsed = true;

		switch (l_rData.datatype()) {
		case MuseData::ANNOTATION:
			l_bParsed = parseAnnotation(l_rData, rLogManager);
			break;
		case MuseData::EEG:
			l_bParsed = parseEEGSample(l_rData, ui32Count, rLogManager);
			break;
		case MuseData::QUANT:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'QUANT'\n"; break;
		case MuseData::ACCEL:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'ACCEL'\n"; break;
		case MuseData::BATTERY:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'BATTERY'\n"; break;
		case MuseData::VERSION:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'VERSION'\n"; break;
		case MuseData::CONFIG:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'CONFIG'\n"; break;
		case MuseData::HISTOGRAM:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'HISTOGRAM'\n"; break;
		case MuseData::ALGVALUE:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'ALGVALUE'\n"; break;
		case MuseData::DSP:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'DSP'\n"; break;
		case MuseData::COMPUTING_DEVICE:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'COMPUTING_DEVICE'\n"; break;
		case MuseData::EEG_DROPPED:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'EEG_DROPPED'\n"; break;
		case MuseData::ACC_DROPPED:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'ACC_DROPPED'\n"; break;
		case MuseData::CALM_APP:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'CALM_APP'\n"; break;
		case MuseData::CALM_ALG:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'CALM_ALG'\n"; break;
		case MuseData::MUSE_ELEMENTS:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'MUSE_ELEMENTS'\n"; break;
		case MuseData::GYRO:
			rLogManager << LogLevel_ImportantWarning << "Unhandled Muse datatype 'GYRO'\n"; break;
		default:
			throw ParseError("Unrecognized MuseData type");
		}

		if (!l_bParsed)
			break;

		++ m_iCollectionIndex;
	}

	while (getEEGSampleCount() < ui32Count)
	{
		if (m_bCorrectState)
		{
			m_oStimulations.push_back(OVTK_GDF_Incorrect);
			m_oStimulationTimes.push_back(m_ui64EEGTime + getEEGSampleCount() * getEEGSamplePeriod());
			m_bCorrectState = false;
		}
		for (uint32 l_ui32Channel = 0; l_ui32Channel < getEEGChannelCount(); ++ l_ui32Channel)
		{
			m_oEEGSamples.push_back(0);
		}
	}
}

#endif
