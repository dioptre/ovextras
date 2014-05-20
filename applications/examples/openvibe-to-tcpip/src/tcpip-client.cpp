/*
 * Receives data from two OpenViBE TCPWriter boxes, a signal writer and a stimulus writer.
 *
 * The code reads the two corresponding sockets asynchronously. With signal, the header contents
 * are printed first. Next, every time a stimulus is received, the code will print out the stimulus
 * and how many bytes of signal have been read and discarded the meanwhile. In order not to make this 
 * code more complicated by mutexes, the signal thread does not try to print anything to cout but just
 * discards the data.
 *
 * This code is meant to be used with the corresponding scenario 'tcpwriter.xml'
 *
 * \author Jussi T. Lindgren (Inria)
 * \date 20 May 11 12:55:22 2014
 *
 */

#ifdef TARGET_HAS_Boost

#include <iostream>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

/*
 * \class TCPWriterClient
 * \brief Basic illustratation of how to read from TCPWriter using boost::asio
 */
class TCPWriterClient {
public:
	TCPWriterClient(boost::asio::io_service& ioService) :
		stimulusSocket(ioService),
		signalSocket(ioService),
		signalBytesRead(0) 
	{

		tcp::resolver resolver(ioService);

		// Signal port
		tcp::resolver::query query = tcp::resolver::query(tcp::v4(), "localhost", "5678"); 
		boost::asio::connect(signalSocket, resolver.resolve(query));

		// Stimulus port
		query = tcp::resolver::query(tcp::v4(), "localhost", "5679");
		boost::asio::connect(stimulusSocket, resolver.resolve(query));

		// Tell ASIO to read a stimulus
		boost::asio::async_read_until(stimulusSocket, stimulusStream, "\n",
			  boost::bind(&TCPWriterClient::stimulusHandler, this,
				boost::asio::placeholders::error));

		// Tell ASIO to read the signal header
		boost::asio::async_read(signalSocket, boost::asio::buffer(signalBuffer.data(), bufferSize),
			boost::asio::transfer_exactly(headerSize),
			boost::bind(&TCPWriterClient::signalHeaderHandler, this,
			boost::asio::placeholders::error));

	}
	
	tcp::socket stimulusSocket;
	tcp::socket signalSocket;

	static const int headerSize = 32;						// signal header size
	static const int bufferSize = 1024;						// signal buffer size

	boost::array<char, headerSize> signalHeaderBuffer;
	boost::array<char, bufferSize> signalBuffer;

	unsigned long long signalBytesRead;						// how many bytes of signal read so far

	boost::asio::streambuf stimulusStream;

	void stimulusHandler(const boost::system::error_code& error)
	{
		if (error == boost::asio::error::eof)
			return; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		// std::cout.write(stimulusBuffer.data(), strlen(stimulusBuffer.data()));
		std::cout << "(" << signalBytesRead << " bytes of signal read), stimulus: " << &stimulusStream;

		// Tell ASIO to read again
		boost::asio::async_read_until(stimulusSocket, stimulusStream, "\n",
			  boost::bind(&TCPWriterClient::stimulusHandler, this,
				boost::asio::placeholders::error));
	}

	void signalHeaderHandler(const boost::system::error_code& error) {
		if (error == boost::asio::error::eof)
			return; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.
	
		std::cout << "Received signal header\n";

		const char *buf = signalBuffer.data();

		// the print assumes the stream is little endian

		std::cout << "  Version:    " << ntohl( *(unsigned int *)&buf[0] ) << "\n";
		std::cout << "  Endianness: " << ntohl( *(unsigned int *)&buf[4] ) << "\n";
		std::cout << "  Hz:         " << *(unsigned int *)&buf[8]  << "\n";
		std::cout << "  Channels:   " << *(unsigned int *)&buf[12] << "\n";
		std::cout << "  nSamples:   " << *(unsigned int *)&buf[16] << "\n";

		std::cout << "Will now read signal in background.\n";
		std::cout << "Stimulations will be printed when received...\n";

		// Tell ASIO to read the actual signal
		boost::asio::async_read(signalSocket, boost::asio::buffer(signalBuffer.data(), bufferSize),
			boost::bind(&TCPWriterClient::signalHandler, this,
			boost::asio::placeholders::error));	
	}

	void signalHandler(const boost::system::error_code& error) {
		if (error == boost::asio::error::eof)
			return; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		// signalBuffer will now contain bufferSize/sizeof(float64) amount of samples in float64s. Here we just discard the data.
		const char *buf = signalBuffer.data();
		const double* signalData = reinterpret_cast<const double*>(buf);

		// Use this to fill your data matrix with the signal...
		signalData; 

		signalBytesRead += bufferSize;

		// Tell ASIO to read more
		boost::asio::async_read(signalSocket, boost::asio::buffer(signalBuffer.data(), bufferSize),
			boost::bind(&TCPWriterClient::signalHandler, this,
			boost::asio::placeholders::error));	
	}



};

int main(int argc, char** argv)
{
	try
	{
		boost::asio::io_service ioService;
		TCPWriterClient client(ioService);
		ioService.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

    return 0;
}

#endif
