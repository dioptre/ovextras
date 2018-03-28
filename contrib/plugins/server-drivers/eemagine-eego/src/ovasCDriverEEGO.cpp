#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

// stl includes
#include <algorithm>
#include <bitset>
#include <exception>
#include <memory>

// OV includes
#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>
#include <system/ovCTime.h>

// auxilliary classes for this implementation
#include "ovasCDriverEEGO.h"
#include "ovasCConfigurationEEGO.h"

// The interface to the driver
// We have to setup the binding method and the unicode support first
#define _UNICODE
#if defined(_MBCS) // Only unicode should be supported. Better stay away from MBCS methods!
#undef _MBCS
#endif
#include <eemagine/sdk/factory.h> // Where it all starts

// Namespaces
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace Kernel;
namespace es = eemagine::sdk;

//___________________________________________________________________//
//                                                                   //

CDriverEEGO::CDriverEEGO(IDriverContext& rDriverContext)
	: IDriver(rDriverContext)
	, m_oSettings("AcquisitionServer_Driver_EEGO", m_rDriverContext.getConfigurationManager())
	, m_pCallback(nullptr)
	, m_ui32SampleCountPerSentBlock(0)
	, m_ui32SamplesInBuffer(0)
	, m_i32TriggerChannel(-1) // == Nonexistent
	, m_ui32LastTriggerValue(~0) // Set every bit to 1
	, m_iBIPRange(1500)
	, m_iEEGRange(1000)
	, m_sEEGMask("0xFFFFFFFFFFFFFFFF") // 64 channels
	, m_sBIPMask("0xFFFFFF") // 24 channels
{
	m_oHeader.setSamplingFrequency(500);
	m_oHeader.setChannelCount(88);

	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);

	// To save your custom driver settings, register each variable to the SettingsHelper
	//m_oSettings.add("SettingName", &variable);
	m_oSettings.add("BIPRange", &m_iBIPRange);
	m_oSettings.add("EEGRange", &m_iEEGRange);

	m_oSettings.add("EEGMask", &m_sEEGMask);
	m_oSettings.add("BIPMask", &m_sBIPMask);

	m_oSettings.load();
}

CDriverEEGO::~CDriverEEGO(void)
{
}

const char* CDriverEEGO::getName(void)
{
	return "EEGO";
}

boolean CDriverEEGO::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if (m_rDriverContext.isConnected()
		|| !m_oHeader.isChannelCountSet()
		|| !m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	try
	{
		// Builds up a buffer to store
		// acquired samples. This buffer
		// will be sent to the acquisition
		// server later...
		m_pSample = std::make_unique<float32[]>(m_oHeader.getChannelCount() * ui32SampleCountPerSentBlock);
		m_ui32SamplesInBuffer = 0;

		// Get the amplifier. If none is connected an exception will be thrown
		try
		{
			m_pAmplifier.reset(factory().getAmplifier());
		}
		catch (const std::exception& ex)
		{
			m_rDriverContext.getLogManager() << LogLevel_Warning << "Failure to get an amplifier! Reason: " << ex.what() << "\n";
			throw;
		}

		if (m_rDriverContext.isImpedanceCheckRequested())
		{
			// end streaming first, if started
			m_pStream.reset();

			// After init we are in impedance mode until the recording is started
			m_pStream.reset(m_pAmplifier->OpenImpedanceStream(getRefChannelMask()));
		}
	}
	catch (const std::exception& ex)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Failed to initialize the driver. Exception: " << ex.what() << "\n";

		m_pSample.reset();
		m_pAmplifier.reset();
		m_pStream.reset();

		return false;
	}

	// Save parameters
	m_pCallback = &rCallback;
	m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverEEGO::check_configuration(void)
{
	// get masks from configuration
	const uint64 l_i64MaskBIP = getBipChannelMask();
	const uint64 l_i64MaskEEG = getRefChannelMask();

	const std::bitset<64> l_oBitsetEEG(l_i64MaskEEG);
	const std::bitset<24> l_oBitsetBIP(l_i64MaskBIP);

	const size_t l_iAllChannels = l_oBitsetBIP.count() + l_oBitsetEEG.count() + 2; // trigger and sample count as additional channels
	if (l_iAllChannels < m_oHeader.getChannelCount())
	{
		// Not enough channels, we have to reduce them
		GtkWidget* l_pDialogue = gtk_message_dialog_new(
			nullptr, // parent
			GTK_DIALOG_MODAL, // Behavoir
			GTK_MESSAGE_QUESTION, // Type
			GTK_BUTTONS_OK_CANCEL, // buttons
			"The channels masks are set to only stream %d channels, but %d channels should be streamed.\n"
			"Change the amount of channels to %d?", l_iAllChannels, m_oHeader.getChannelCount(), l_iAllChannels);
		gint l_iResult = gtk_dialog_run(GTK_DIALOG(l_pDialogue));
		gtk_widget_destroy(l_pDialogue);
		l_pDialogue = nullptr;
		switch (l_iResult)
		{
		case GTK_RESPONSE_OK:
			// update the channel count to contain only the selected masks
			m_oHeader.setChannelCount(l_iAllChannels);
			break;
		default:
			// Nothing can be done here
			return false;
		}
	}

	return true;
}

uint64 CDriverEEGO::getRefChannelMask() const
{
	uint64 l_i64MaskEEG(0);
	if (!CHeaderEEGO::convertMask(m_sEEGMask, l_i64MaskEEG))
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Error converting mask: m_sEEGMask: " << m_sEEGMask;
	}
	return l_i64MaskEEG;
}

uint64 CDriverEEGO::getBipChannelMask() const
{
	uint64 l_i64MaskBIP(0);
	if (!CHeaderEEGO::convertMask(m_sBIPMask, l_i64MaskBIP))
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Error converting mask: l_i64MaskBIP: " << l_i64MaskBIP << "\n";
	}
	return l_i64MaskBIP;
}

eemagine::sdk::factory& CDriverEEGO::factory()
{
	if (m_pFactory == nullptr)
	{
		// create the amplifier factory
		// To initialize we need to locate the path of the DLL
		// Create path to the dll
#ifdef _WIN32
		const CString l_oLibDir = Directories::getBinDir() + "\\eego-SDK.dll";
		const std::string l_sPath(l_oLibDir.toASCIIString());
#else
		const std::string l_sPath("libeego-SDK.so");
#endif // _WIN32

		m_rDriverContext.getLogManager() << LogLevel_Debug << "SDK dll/so path: " << l_sPath.c_str() << "\n";
		m_pFactory = std::make_unique<es::factory>(l_sPath);

		// to check what is going on case of error; Log version
		const auto version = m_pFactory->getVersion();
		m_rDriverContext.getLogManager() << LogLevel_Info << "EEGO RT: Version: " << version.major << "." << version.minor << "." << version.micro << "." << version.build << "\n";
	}

	return *m_pFactory;
}

boolean CDriverEEGO::start(void)
{
	if (!m_rDriverContext.isConnected()) return false;
	if (m_rDriverContext.isStarted()) return false;
	if (!m_pAmplifier) return false;

	// Check configuration
	if (!check_configuration()) return false;

	// ...
	// request hardware to start
	// sending data
	// ..
	const double l_fBIPRange = m_iBIPRange / 1000.;
	const double l_fEEGRange = m_iEEGRange / 1000.;

	try
	{
		// stop old streams, if existing
		m_pStream.reset();
		// Create the eeg stream
		m_pStream.reset(
			m_pAmplifier->OpenEegStream(
				m_oHeader.getSamplingFrequency(),
				l_fEEGRange,
				l_fBIPRange,
				getRefChannelMask(),
				getBipChannelMask()));

		// Error check
		if (!m_pStream)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "The stream returned is NULL!";
			return false;
		}

		// find trigger channel
		auto list = m_pStream->getChannelList();
		auto triggerIterator = std::find_if(list.begin(), list.end(), [](eemagine::sdk::channel& chan)
		{
			return chan.getType() == eemagine::sdk::channel::trigger;
		});

		if (triggerIterator == list.end())
		{
			m_i32TriggerChannel = -1; // Unkown
		}
		else
		{
			m_i32TriggerChannel = (*triggerIterator).getIndex();
		}

		// Wait till we are really getting data.
		while (m_pStream->getData().getSampleCount() == 0)
		{
			System::Time::sleep(5); // Do Nothing
		}
	}
	catch (const std::exception& ex)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not open EEG stream: " << ex.what();
		return false;
	}

	return true;
}

boolean CDriverEEGO::loop(void)
{
	if (!m_rDriverContext.isConnected()) return false;

	if (!m_rDriverContext.isStarted()
		&& !m_rDriverContext.isImpedanceCheckRequested())
		return true; // Nothing to be done here!

	if (!m_pStream) return false;
	// Check if we really provide enough channels
	// When doing impedance only the normal EEG channels are tested. This is fine and handled.
	if (m_pStream->getChannelList().size() < m_oHeader.getChannelCount()
		&& m_rDriverContext.isStarted()) // !started -> impedance
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "The amplifier got asked for more channels than it could provide";
		return false;
	}

	if (m_rDriverContext.isStarted()) // Normal operation
	{
		eemagine::sdk::buffer data;

		try
		{
			data = m_pStream->getData();
		}
		catch (const std::exception& ex)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "Error fetching data: " << ex.what();
			return false;
		}

		const uint32 l_ui32SampleCount = data.getSampleCount();

		// For EEGO the with every index increment, the channel is incremented.
		// For OpenVibe it means next sample. Therefor we have to transpose the data
		for (uint32 sample = 0; sample < l_ui32SampleCount; sample++)
		{
			for (uint32 channel = 0; channel < m_oHeader.getChannelCount(); channel++)
			{
				const int ovIdx = m_ui32SamplesInBuffer + channel * m_ui32SampleCountPerSentBlock;

				const double& sampleVal = data.getSample(channel, sample);
				m_pSample[ovIdx] = static_cast<float32>(sampleVal);
			}

			// Add potential triggers to stimulation set
			// check for triggers
			if (m_i32TriggerChannel >= 0) // Only try to find triggers when the channel exists
			{
				// A trigger is detected when the level changes in positive direction, all additional bits are seen as trigger code
				// a change from 1 to 0 is ignored
				const uint32 currentTriggers = (const uint32)data.getSample(m_i32TriggerChannel, sample);
				const uint32 currentNewTriggers = currentTriggers & ~m_ui32LastTriggerValue;
				m_ui32LastTriggerValue = currentTriggers;

				if (currentNewTriggers != 0)
				{
					const uint64 currentTime = ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(),
						m_ui32SamplesInBuffer);
					m_oStimulationSet.appendStimulation(OVTK_StimulationId_Label(currentNewTriggers), currentTime, 0);
				}
			}

			// Send buffer counter
			m_ui32SamplesInBuffer++;

			// Send buffer is full, so send it
			if (m_ui32SamplesInBuffer == m_ui32SampleCountPerSentBlock)
			{
				m_pCallback->setSamples(m_pSample.get());
				m_pCallback->setStimulationSet(m_oStimulationSet);

				// When your sample buffer is fully loaded,
				// it is advised to ask the acquisition server
				// to correct any drift in the acquisition automatically.
				m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

				m_ui32SamplesInBuffer = 0;
				m_oStimulationSet.clear();
			}
		}
	}
	else // Impedance
	{
		// Get the impedance data, here the data is always the most current state.
		// The method can block if impedance still needs to be calculated.
		eemagine::sdk::buffer data;
		try
		{
			data = m_pStream->getData();
		}
		catch (const std::exception& ex)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "Error fetching data: " << ex.what();
			return false;
		}

		// We have to take care not to r/w over any boundary.
		uint32 minChannels = std::min(data.getChannelCount(), m_oHeader.getChannelCount());
		for (uint32 channel = 0; channel < minChannels; channel++)
		{
			m_rDriverContext.updateImpedance(channel, data.getSample(channel, 0));
		}
	}

	return true;
}

boolean CDriverEEGO::stop(void)
{
	if (!m_rDriverContext.isConnected()) return false;
	if (!m_rDriverContext.isStarted()) return false;

	// ...
	// request the hardware to stop
	// sending data
	// ...
	if (m_rDriverContext.isImpedanceCheckRequested())
	{
		try
		{
			m_pStream.reset();
			m_pStream.reset(m_pAmplifier->OpenImpedanceStream(getRefChannelMask())); // And we can stream Impedances once more.
		}
		catch (const std::exception& ex)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "Error restarting impedance: " << ex.what() << "\n";
			return false;
		}
	}

	return true;
}

boolean CDriverEEGO::uninitialize(void)
{
	if (!m_rDriverContext.isConnected()) return false;
	if (m_rDriverContext.isStarted()) return false;

	// ...
	// uninitialize hardware here
	// ...
	m_pStream.reset();
	m_pAmplifier.reset();
	m_pSample.reset();

	m_pCallback = nullptr;
	m_ui32SamplesInBuffer = 0;

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverEEGO::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDriverEEGO::configure(void)
{
	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationEEGO m_oConfiguration(m_rDriverContext,
		Directories::getDataDir() + "/applications/acquisition-server/interface-EEGO.ui",
		m_oHeader);

	m_oHeader.setBIPRange(m_iBIPRange);
	m_oHeader.setEEGRange(m_iEEGRange);
	m_oHeader.setBIPMask(m_sBIPMask);
	m_oHeader.setEEGMask(m_sEEGMask);
	if (!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}

	m_iBIPRange = m_oHeader.getBIPRange();
	m_iEEGRange = m_oHeader.getEEGRange();
	m_sBIPMask = m_oHeader.getBIPMask();
	m_sEEGMask = m_oHeader.getEEGMask();

	m_oSettings.save();

	return true;
}

#endif
