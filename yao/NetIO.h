#ifndef NETIO_H_
#define NETIO_H_

#include "Bytes.h"

class Socket
{
protected:
	int m_socket;

public:
	Socket();
	Socket(int socket) : m_socket(socket) {}
	virtual ~Socket();

	void write_bytes(const Bytes &bytes);
	void write_bytes(char * bytes, int size);
	Bytes read_bytes();
	void read_bytes(unsigned char * bytes, unsigned int * recsize);

	void write_string(const std::string &str);
	std::string read_string();
};

class ClientSocket : public Socket
{
public:
	ClientSocket(const char *host_ip, size_t port);
	virtual ~ClientSocket() {}
};

class ServerSocket : public Socket
{
	std::vector<int> m_sockets;

public:
	ServerSocket(size_t port);
	Socket *accept();
	virtual ~ServerSocket();
};

#endif /* NETIO_H_ */
