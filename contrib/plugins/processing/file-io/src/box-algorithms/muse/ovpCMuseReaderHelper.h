#ifndef MUSEREADERHELPER_H
#define MUSEREADERHELPER_H

#include <openvibe/ov_all.h>

#include <istream>
#include <stdexcept>
#include <vector>

#include "Muse_v2.pb.h"

namespace Muse
{
	/**
	* \class CMuseReaderHelper
	* \author Karl Semich
	* \brief Muse protobuf format parser and utilities. 
	**/
	class CMuseReaderHelper
	{
	protected:
		std::istream & m_rMuseStream;
		interaxon::muse_data::MuseDataCollection m_oCollection;

		OpenViBE::uint64 m_ui64EEGTime;
		OpenViBE::uint64 m_ui64EEGSampleRate;
		OpenViBE::uint64 m_ui64EEGSamplePeriod;
		OpenViBE::int64 m_i64MaxClockSkew;
		OpenViBE::uint32 m_ui32EEGChannelCount;
		OpenViBE::boolean m_bCorrectState;
		std::vector<OpenViBE::float32> m_oEEGSamples;

		std::vector<OpenViBE::uint64> m_oStimulations;
		std::vector<OpenViBE::uint64> m_oStimulationTimes;

		/**
		* Sizes m_oMsgBuffer
		*/
		void parseMessageHeader();

		/**
		* Populates m_oCollection
		*/
		void parseMessage();

	private:
		std::vector<char> m_oMsgBuffer;
		int m_iCollectionIndex;

		/**
		* Populates m_oStimulations.  Called by parseSamples().
		*/
		bool parseAnnotation(interaxon::muse_data::MuseData const & rAnnotationData, OpenViBE::Kernel::ILogManager & rLogManager);

		/**
		* Populates m_oEEGSamples.  Called by parseSamples().
		*/
		bool parseEEGSample(interaxon::muse_data::MuseData const & rEEGData, OpenViBE::uint32 ui32Count, OpenViBE::Kernel::ILogManager & rLogManager);

	public:
		/**
		* Construct from a .muse file stream.
		* \param oInputStream stream to read from
		**/
		CMuseReaderHelper(std::istream & rInputStream, OpenViBE::uint64 ui64DefaultEEGSampleRate, OpenViBE::int64 i64MaxClockSkew);
		~CMuseReaderHelper();

		/**
		* Read a contiguous block of samples.
		* Data can be fetched via accessor methods.
		*
		* \param ui32Count number of EEG samples to read
		**/
		void parseSamples(OpenViBE::uint32 ui32Count, OpenViBE::Kernel::ILogManager & rLogManager);

		/**
		* EEG Accessors, populated by parseSamples()
		**/
		OpenViBE::uint64 getEEGTime() const { return m_ui64EEGTime; }
		OpenViBE::uint32 getEEGChannelCount() const { return m_ui32EEGChannelCount; }
		OpenViBE::uint32 getEEGSampleCount() const { return m_oEEGSamples.size() / getEEGChannelCount(); }
		OpenViBE::uint64 getEEGSampleRate() const { return m_ui64EEGSampleRate; }
		OpenViBE::uint64 getEEGSamplePeriod() const { return m_ui64EEGSamplePeriod; }
		OpenViBE::float32 const * getEEGSample(OpenViBE::uint32 ui32which) const { return &m_oEEGSamples[ui32which * getEEGChannelCount()]; }
		OpenViBE::float32 const * getEEGSamples() const { return &m_oEEGSamples[0]; }

		/**
		* Stimulation Accessors, populated by parseSamples()
		**/
		OpenViBE::uint32 getStimulationCount() const { return m_oStimulations.size(); }
		OpenViBE::uint64 getStimulation(OpenViBE::uint32 ui32which) const { return m_oStimulations[ui32which]; }
		OpenViBE::float64 getStimulationTime(OpenViBE::uint32 ui32which) const { return m_oStimulationTimes[ui32which]; }
		
		/**
		* Stream is valid, EOF not reached yet.
		**/
		bool messageAvailable() { return static_cast<bool>(m_rMuseStream); }

		/**
		 * This exception is thrown for unreadable data.
		 **/
		class ParseError : public std::runtime_error
		{
		public:
			ParseError(std::string message = "Parse error") : std::runtime_error(message) {}
		};
	};
}

#endif
