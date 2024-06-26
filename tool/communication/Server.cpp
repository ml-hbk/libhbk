#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <string>

#include <hbk/sys/eventloop.h>
#include <hbk/communication/tcpserver.h>
#include <hbk/communication/socketnonblocking.h>


static hbk::sys::EventLoop eventloop;


static void SigHandlerStop(int)
{
	eventloop.stop();
}

static void SigHandlerIgnore(int)
{
	// do nothing!
}

static void cb(hbk::communication::clientSocket_t workerSocket)
{
	ssize_t result;
	char buffer;
	// simply block the server until client closes connection to worker
	
	std::cout << "accepted connection!" << std::endl;
	do {
		result = workerSocket->receiveComplete(&buffer, 1);
		if (result == 0) {
			std::cout << "closed connection" << std::endl;
			break;
		} else if (result<0) {
			std::cerr << strerror(errno) << std::endl;
		}
		workerSocket->sendBlock(&buffer, 1, false);
	} while (true);
}


int main(int argc, char* argv[])
{
	signal(SIGTERM, &SigHandlerStop);
	signal(SIGINT, &SigHandlerStop);
#ifndef _WIN32
	signal(SIGPIPE, &SigHandlerIgnore);
#endif

	if (argc != 2) {
		std::cout << "syntax: " << argv[0] << " < server port >" << std::endl;
		std::cout << "syntax: " << argv[0] << " < unix domain socket name >" << std::endl;
		return EXIT_SUCCESS;
	}



	hbk::communication::TcpServer server(eventloop);

	try {
		// If parameter is a number, this is to be a TCP server listening to that port
		uint16_t port = static_cast < uint16_t > (std::stoul(argv[1]));

		server.start(port, 1, &cb);
		std::cout << "Listening to TCP port " << port << "..." << std::endl;
	} catch(...) {
		// If parameter is a string, this is to be a unix domain socket server.
		// Interprete parameter as unix domain socket name
#ifdef _WIN32
		std::cerr << "Unix domain sockets are not supported under windoes" << std::endl;
		return EXIT_FAILURE;
#else
		constexpr bool useAbstractNamespace = false;
		server.start(argv[1], useAbstractNamespace, 1, &cb);
		if (useAbstractNamespace) {
			std::cout << "Listening abstract to unix domain socket path " << argv[1] << "..." << std::endl;
		} else {
			std::cout << "Listening to unix domain socket path " << argv[1] << "..." << std::endl;
		}
#endif
	}

	eventloop.execute();
	return EXIT_SUCCESS;
}
