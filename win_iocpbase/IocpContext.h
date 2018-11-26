#ifndef __IOCP_CONTEXT_H__
#define __IOCP_CONTEXT_H__

#include <stdint.h>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

/*
IOCP context, for IOCP callback data.
*/
class IocpContext : public OVERLAPPED
{
	//
	// Members
	//
public:
	enum IocpContextType
	{ 
		Rcv,
		Send,
		Accept,
		Disconnect,
	};

	//OVERLAPPED m_overlapped;

	//! the actual buffer that holds all the data
	std::vector<uint8_t> m_data;

	//! ptr to the winsock buffer (which points to m_data)
	WSABUF m_wsaBuffer;

	//! the socket for this connection
	SOCKET m_socket;

	//! connection id
	uint64_t m_cid;

	//! the type of iocp context
	IocpContextType m_type;

	int m_rcvBufferSize;

	//
	// Functions
	//
public:
	IocpContext(SOCKET socket, uint64_t cid, IocpContextType t, int rcvBufferSize);
	~IocpContext(void);

	//! Reset the WSA buffer. Should be called each time the context is used.
	void ResetWsaBuf();

private:
	// 阻止使用默认构造函数
	IocpContext(void);
};

#endif