#include "ovasCTagStream.h"

#include <sys/timeb.h>

using namespace boost::asio::ip;
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;

void CTagQueue::push(const Tag &tag)
{
	boost::lock_guard<boost::mutex> guard(m_mutex);
	m_queue.push(tag);
}

bool CTagQueue::pop(Tag &tag)
{
	boost::lock_guard<boost::mutex> guard(m_mutex);
	if (m_queue.size()==0) return false;
	else {
		tag = m_queue.front();
		m_queue.pop();
		return true;
	}
}


CTagSession::CTagSession(boost::asio::io_service &io_service, const SharedQueuePtr &queue)
	: m_socket(io_service), m_queuePtr(queue)
{
}

tcp::socket &CTagSession::socket()
{
	return m_socket;
}

void CTagSession::start()
{
	startRead();
}

void CTagSession::startRead()
{
	// Caveat: a shared pointer is used (instead of simply using this) to ensure that this instance of TagSession is still alive when the callback is called.
	boost::asio::async_read(m_socket, boost::asio::buffer((void *) &m_tag, sizeof(Tag)), boost::bind(&CTagSession::handleRead, shared_from_this(), _1));
}

void CTagSession::handleRead(const boost::system::error_code &error)
{
	if (!error) {
		// If the timestamp is 0, set timestamp to current posix time.
		if (m_tag.timestamp==0) {
			// Get POSIX time (number of milliseconds since epoch)
			timeb time_buffer;
			ftime(&time_buffer);
			unsigned long long posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

			// edit timestamp
			m_tag.timestamp=posixTime;
		}

		// Push tag to the queue.
		m_queuePtr->push(m_tag);

		// Continue reading.
		startRead();
	}
}


CTagServer::CTagServer(const SharedQueuePtr &queue, int port)
	: m_ioService(), m_acceptor(m_ioService, tcp::endpoint(tcp::v4(), port)), m_queuePtr(queue)
{
}


void CTagServer::run()
{
	try {
		startAccept();
		m_ioService.run();
	}
	catch(std::exception& e) {
		// TODO: log error message
	}
}


void CTagServer::startAccept()
{
	SharedSessionPtr newSession (new CTagSession(m_ioService, m_queuePtr));
	// Note: if this instance of TaggingSever is destroyed then the associated io_service is destroyed as well.
	// Therefore the call-back will never be called if this instance is destroyed and it is safe to use this instead of shared pointer.
	m_acceptor.async_accept(newSession->socket(), boost::bind(&CTagServer::handleAccept, this, newSession, _1));
}


void CTagServer::handleAccept(SharedSessionPtr session, const boost::system::error_code &error)
{
	if (!error) {
		session->start();
	}

	startAccept();
}


CTagStream::CTagStream(int port)
	: m_queuePtr(new CTagQueue), m_port(port)
{
	boost::thread thread (&CTagStream::startServer, this);
}


bool CTagStream::pop(Tag &tag)
{
	return m_queuePtr->pop(tag);
}


void CTagStream::startServer()
{
	CTagServer server(m_queuePtr, m_port);
	server.run();
}
