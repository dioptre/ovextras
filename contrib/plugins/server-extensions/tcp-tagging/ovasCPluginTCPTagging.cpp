#include "ovasCPluginTCPTagging.h"

#include <set>
#include <queue>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <sys/timeb.h>

// Implementation of PluginTCPTagging.
// The plugin relies on four auxilliary classes: TagQueue, TagSession, TagServer and TagStream.
// TagQueue implements a trivial queue to store tags with exclusive locking.
// TagServer implements a server that simply binds to a port and waits for incoming connections.
// TagSession represents an individual connection with a client and holds a connection handle (socket)
// and a data buffer to store incoming data.
// The use of shared pointers is instrumental to ensure that instances are still alive when call-backs are
// called and avoid memory corruption.
// The TagStream class implements a stream to allow to collect tags. Upon instantiation, it creates an instance
// of TagServer and starts the server in an auxilliary thread.
// The exchange of data between the main tread and the auxilliary thread is performed via a lockfree queue (boost).

using boost::asio::ip::tcp;
using namespace OpenViBE;
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;

// A Tag consists of an identifier to inform about the type of event
// and a timestamp corresponding to the time at which the event occurrs.
struct Tag
{
	uint64 padding, identifier, timestamp;
};

class TagSession; // forward declaration of TagSession to define SharedSessionPtr
class TagQueue; // forward declaration of TagQueue to define SharedQueuePtr

typedef boost::shared_ptr<TagQueue> SharedQueuePtr;
typedef boost::shared_ptr<TagSession> SharedSessionPtr;

// A trivial implementation of a queue to store Tags with exclusive locking
class TagQueue
{
public:
	TagQueue()
	{
	}

	void push(const Tag& tag)
	{
		boost::lock_guard<boost::mutex> guard(m_mutex);
		m_queue.push(tag);
	}

	bool pop(Tag& tag)
	{
		boost::lock_guard<boost::mutex> guard(m_mutex);
		if (m_queue.size()==0) return false;
		else {
			tag = m_queue.front();
			m_queue.pop();
			return true;
		}
	}
private:
	std::queue<Tag> m_queue;
	boost::mutex m_mutex;
};

// An instance of TagSession is associated to every client connecting to the Tagging Server.
// It contains a connection handle and data buffer.
class TagSession : public boost::enable_shared_from_this<TagSession>
{
public:
	TagSession(boost::asio::io_service& io_service, const SharedQueuePtr& queue)
		: m_socket(io_service), m_queuePtr(queue)
	{
	}

	tcp::socket& socket()
	{
		return m_socket;
	}

	void start()
	{
		startRead();
	}

	void startRead()
	{
		// Caveat: a shared pointer is used (instead of simply using this) to ensure that this instance of TagSession is still alive when the call-back is called.
		boost::asio::async_read(m_socket, boost::asio::buffer((void *) &m_tag, sizeof(Tag)), boost::bind(&TagSession::handleRead, shared_from_this(), _1));
	}

	void handleRead(const boost::system::error_code& error)
	{
		if (!error) {
			// If the timestamp is 0, set timestamp to current posix time.
			if (m_tag.timestamp==0) {
        			// Get POSIX time (number of milliseconds since epoch)
        			timeb time_buffer;
        			ftime(&time_buffer);
        			uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

				// edit timestamp
				m_tag.timestamp=posixTime;
			} 

			// Push tag to the queue.
			m_queuePtr->push(m_tag);

			// Continue reading.
			startRead();
		}
	}

private:
	Tag m_tag;
	tcp::socket m_socket;
	SharedQueuePtr m_queuePtr;
};

// TagServer implements a server that binds to a port and accepts new connections.
// It also has a field sessionSet that holds shared pointers to all exisiting sessions
class TagServer
{
public:
	// Server port.
	enum {PORT = 15361};

	TagServer(const SharedQueuePtr& queue)
		: m_ioService(), m_acceptor(m_ioService, tcp::endpoint(tcp::v4(), PORT)), m_queuePtr(queue)
	{
	}

	void run()
	{
		try {
			startAccept();
			m_ioService.run();
		}
		catch(std::exception& e) {
			// TODO: log error message
		}
	}

private:
	void startAccept()
	{
		SharedSessionPtr newSession (new TagSession(m_ioService, m_queuePtr));
		// Note: if this instance of TaggingSever is destroyed then the associated io_service is destroyed as well.
		// Therefore the call-back will never be called if this instance is destroyed and it is safe to use this instead of shared pointer.
		m_acceptor.async_accept(newSession->socket(), boost::bind(&TagServer::handleAccept, this, newSession, _1));
	}

	void handleAccept(SharedSessionPtr session, const boost::system::error_code& error)
	{
		if (!error) {
			session->start();
		}

		startAccept();
	}

private:
	boost::asio::io_service m_ioService;
	tcp::acceptor m_acceptor;
	const SharedQueuePtr& m_queuePtr;
};

// TagStream allows to collect tags received via TCP.
class TagStream
{
	// Initial memory allocation of lockfree queue.
	enum {ALLOCATE = 128};

public:	   
	TagStream()
		: m_queuePtr(new TagQueue)
	{
		boost::thread thread (&TagStream::startServer, this);
	}	 

	bool pop(Tag& tag)
	{
		return m_queuePtr->pop(tag);
	}

private:
	void startServer()
	{
		TagServer server(m_queuePtr);
		server.run();
	}			 
				
private:
	SharedQueuePtr m_queuePtr;
};

static TagStream tagStream;

// CPluginTCPTagging implementation

CPluginTCPTagging::CPluginTCPTagging(const OpenViBE::Kernel::IKernelContext& rKernelContext)
	: IAcquisitionServerPlugin(rKernelContext, CString("AcquisitionServer_Plugin_TCPTagging"))
{
	m_rKernelContext.getLogManager() << Kernel::LogLevel_Info << "Loading plugin: TCP Tagging\n";
}

CPluginTCPTagging::~CPluginTCPTagging()
{
}

void CPluginTCPTagging::startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames,
	OpenViBE::uint32 ui32SamplingFrequency, OpenViBE::uint32 ui32ChannelCount, OpenViBE::uint32 ui32SampleCountPerSentBlock)
{
	// Get POSIX time (number of milliseconds since epoch)
	timeb time_buffer;
	ftime(&time_buffer);
	uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

	// Initialize time counters.
	m_previousPosixTime = posixTime;
	m_previousSampleTime = 0;

	// Clear Tag stream
	Tag tag;
	while(tagStream.pop(tag));
}

void CPluginTCPTagging::loopHook(std::vector < std::vector < OpenViBE::float32 > >& /*vPendingBuffer*/,
	OpenViBE::CStimulationSet& stimulationSet, uint64 start, uint64 end, uint64 sampleTime)
{
	// Get POSIX time (number of milliseconds since epoch)
	timeb time_buffer;
	ftime(&time_buffer);
	uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

	Tag tag;

	// Collect tags from the stream until exhaustion.
	while(tagStream.pop(tag)) {
		m_rKernelContext.getLogManager() << Kernel::LogLevel_Info << "New Tag received (" << tag.padding << ", " << tag.identifier << ", " << tag.timestamp << ") at " << posixTime << " (posix time in ms)\n";

		// Check that the timestamp fits the current chunk.
		if (tag.timestamp < m_previousPosixTime) {
			m_rKernelContext.getLogManager() << Kernel::LogLevel_Warning << "The Tag has arrived before the beginning of the current chunk and will be inserted at the beginning of this chunk\n";
			tag.timestamp = m_previousPosixTime;
		}

		// Marker time correction (simple local linear interpolation).
		if (m_previousPosixTime != posixTime) {
			tag.timestamp = m_previousSampleTime + (tag.timestamp - m_previousPosixTime)*((sampleTime - m_previousSampleTime) / (posixTime - m_previousPosixTime));
		}

		// Insert tag into the stimulation set.
		stimulationSet.appendStimulation(tag.identifier, tag.timestamp, 0 /* duration of tag (ms) */);
	}

	// Update time counters.
	m_previousPosixTime = posixTime;
	m_previousSampleTime = sampleTime;
}
