#ifndef __CONNETION_H__
#define __CONNETION_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#include <vector>

#include <memory>

#include "boost/thread/mutex.hpp"

#include "IocpContext.h"
#include "SendQueue.h"

#include "config/conf_net_msg.h"

/*
Connection.
*/
class Connection
{
	//
	// Members
	//
public:
	bool HasOutstandingContext();

	SOCKET m_socket;
	uint64_t m_id;

	long m_disconnectPending;
	long m_sendClosePending;
	long m_rcvClosed;  

	IocpContext m_rcvContext;

	SendQueue m_sendQueue;

	IocpContext m_disconnectContext;

	boost::mutex m_connectionMutex;

	// ÏûÏ¢»º´æ
	struct simutgw::SocketUserMessage m_SocketMsg;

	//
	// Functions
	//
public:
	Connection(SOCKET socket, uint64_t cid, int rcvBufferSize);
	virtual ~Connection(void);

	std::shared_ptr<IocpContext> CreateSendContext();

	bool CloseRcvContext(void);
};

#endif