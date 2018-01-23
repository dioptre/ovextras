#ifndef __OpenViBE_AcquisitionServer_TCPTagStream_H__
#define __OpenViBE_AcquisitionServer_TCPTagStream_H__

#include <queue>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <mutex>
#include <thread>

// PluginTCPTagging relies on four auxilliary classes: CTagQueue, CTagSession, CTagServer and CTagStream.
// CTagQueue implements a trivial queue to store tags with exclusive locking.
// CTagServer implements a server that simply binds to a port and waits for incoming connections.
// CTagSession represents an individual connection with a client and holds a connection handle (socket)
// and a data buffer to store incoming data.
// The use of shared pointers is instrumental to ensure that instances are still alive when call-backs are
// called and avoid memory corruption.
// The CTagStream class implements a stream to allow to collect tags. Upon instantiation, it creates an instance
// of CTagServer and starts the server in an auxilliary thread.
// The exchange of data between the main tread and the auxilliary thread is performed via a lockfree queue (boost).

namespace OpenViBEAcquisitionServer
{

namespace OpenViBEAcquisitionServerPlugins
{

// A Tag consists of an identifier to inform about the type of event
// and a timestamp corresponding to the time at which the event occurrs.
struct Tag
{
	uint64_t flags, identifier, timestamp;
};

// Note: duplicated in TCP Tagging module in openvibe
enum TCP_Tagging_Flags {
	FLAG_FPTIME               = (1LL << 0),         // The time given is fixed point time.
	FLAG_AUTOSTAMP_CLIENTSIDE = (1LL << 1),         // Ignore given stamp, bake timestamp on client side before sending
	FLAG_AUTOSTAMP_SERVERSIDE = (1LL << 2)          // Ignore given stamp, bake timestamp on server side when receiving
};

class CTagSession; // forward declaration of CTagSession to define SharedSessionPtr
class CTagQueue; // forward declaration of CTagQueue to define SharedQueuePtr
class CTagServer; // forward declaration of CTagServer to define ScopedServerPtr

typedef std::shared_ptr<CTagQueue> SharedQueuePtr;
typedef std::shared_ptr<CTagSession> SharedSessionPtr;
typedef std::unique_ptr<CTagServer> ScopedServerPtr;
typedef std::unique_ptr<std::thread> ScopedThreadPtr;

// A trivial implementation of a queue to store Tags with exclusive locking
class CTagQueue
{
	public:
		CTagQueue()
		{
		}

		void push(const Tag& tag);

		bool pop(Tag& tag);
	private:
		std::queue<Tag> m_queue;
		std::mutex m_mutex;
};

// An instance of CTagSession is associated to every client connecting to the Tagging Server.
// It contains a connection handle and data buffer.
class CTagSession : public std::enable_shared_from_this<CTagSession>
{
	public:
		CTagSession(boost::asio::io_service& io_service, const SharedQueuePtr& queue);

		boost::asio::ip::tcp::socket& socket();

		void start();

		void startRead();

		void handleRead(const boost::system::error_code& error);

	private:
		Tag m_tag;
		boost::asio::ip::tcp::socket m_socket;
		SharedQueuePtr m_queuePtr;
		uint64_t m_errorState;
	};

// CTagServer implements a server that binds to a port and accepts new connections.
class CTagServer
{
	public:
		CTagServer(const SharedQueuePtr& queue, int port = 15361);
		~CTagServer();

		void run();
		void stop();

	private:
		void startAccept();

		void handleAccept(SharedSessionPtr session, const boost::system::error_code& error);

	private:
		boost::asio::io_service m_ioService;
		boost::asio::ip::tcp::acceptor m_acceptor;
		const SharedQueuePtr& m_queuePtr;
};

// CTagStream allows to collect tags received via TCP.
class CTagStream
{
	// Initial memory allocation of lockfree queue.
	enum {ALLOCATE = 128};

	public:
		CTagStream(int port = 15361);
		~CTagStream();

		bool pop(Tag& tag);

	private:
		void startServer();

	private:
		SharedQueuePtr m_queuePtr;
		ScopedServerPtr m_serverPtr;
		ScopedThreadPtr m_threadPtr;
		int m_port;
};

}

}

#endif // __OpenViBE_AcquisitionServer_TCPTagStream_H__
