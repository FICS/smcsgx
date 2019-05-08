#include <iostream>
#include <pthread.h>
#include <vector>

#include <netdb.h>
#include <arpa/inet.h>

#include "Env.h"
#include "NetIO.h"

#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("ipserver.cpp"));

//static const size_t PORT = 4096;

static Bytes ips;

pthread_mutex_t mutex;

extern "C"
{
	void print_ips(const Bytes &ips)
	{
		std::vector<Bytes> chunks = ips.split(sizeof(struct in_addr));
		for (size_t ix = 0; ix < chunks.size(); ix++)
			LOG4CXX_INFO(logger, "ip[" << ix << "]: "
				<< inet_ntoa(*(struct in_addr *)&chunks[ix][0]));
	}

	void *foo(void *arg)
	{
		Socket *socket = reinterpret_cast<Socket*>(arg);

		switch((socket->read_bytes())[0])
		{
        case Env::GEN:
			pthread_mutex_lock(&mutex);
				ips = socket->read_bytes();
				LOG4CXX_INFO(logger, "GEN_REQUEST: " << ips.to_hex());
				print_ips(ips);
			pthread_mutex_unlock(&mutex);
			break;

        case Env::EVL:
			pthread_mutex_lock(&mutex);
				LOG4CXX_INFO(logger, "EVL_REQUEST: " << ips.to_hex());
				socket->write_bytes(ips);
			pthread_mutex_unlock(&mutex);
			break;

        default:
			LOG4CXX_INFO(logger, "Unknown request!");
			break;
		}

		delete socket;
		LOG4CXX_INFO(logger, "disconnecting...");
		pthread_exit(0);

		return arg;
	}
}

int main()
{
	log4cxx::PropertyConfigurator::configure("log4cxx.conf");

	pthread_mutex_init(&mutex, NULL);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	const int THREAD_COUNT = 8;
	int thread_ix = 0;
	pthread_t thread[THREAD_COUNT];

	ServerSocket server(Env::IP_SERVER_PORT);
	LOG4CXX_INFO(logger, "IP server has started...");

	while (1)
	{
		Socket *socket = server.accept();
		LOG4CXX_INFO(logger, "New incoming connection");
		pthread_create(&thread[thread_ix], &attr, foo, socket);
		thread_ix = (thread_ix + 1) % THREAD_COUNT;
	}

	return 0;
}
