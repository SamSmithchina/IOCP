#include "IOCP_Thread_Server.h"

#include "util/EzLog.h"

#include "WinSockUtil.h"
#include "IOCPUtil.h"
#include "socket_conn_manage_base/ConnectionInformation.h"
#include "Connection.h"
#include "IocpData_Server.h"

using namespace std;

IOCP_Thread_Server::IOCP_Thread_Server(void)
{
}

IOCP_Thread_Server::~IOCP_Thread_Server(void)
{
}

void IOCP_Thread_Server::SetIocpData(std::shared_ptr<IocpData_Server>& pIocpdata)
{
	m_pIocpdataServer = pIocpdata;
}

/*
启动线程

Return :
0 -- 启动成功
-1 -- 启动失败
*/
int IOCP_Thread_Server::StartThread(void)
{
	static const string ftag("IOCP_Thread_Server::StartThread() ");

	int iRes = ThreadBase::StartThread(&IOCP_Thread_Server::Run);

	return iRes;
}


DWORD WINAPI IOCP_Thread_Server::Run(LPVOID  pParam)
{
	static const string ftag("IOCP_Thread_Server::Run() ");

	IOCP_Thread_Server* pThread = static_cast<IOCP_Thread_Server*>(pParam);

	while ((ThreadPool_Conf::Running == pThread->m_emThreadRunOp) ||
		(ThreadPool_Conf::TillTaskFinish == pThread->m_emThreadRunOp))
	{
		// 处理任务
		void *key = NULL;
		OVERLAPPED *pOverlapped = NULL;
		DWORD dwBytesTransfered = 0;

#ifdef _WIN64
		// _WIN64 __amd64
		BOOL completionStatus = GetQueuedCompletionStatus(
			pThread->m_pIocpdataServer->m_hCompletePort,
			&dwBytesTransfered,
			(PULONG_PTR)&key,
			&pOverlapped,
			INFINITE);
#else
#ifdef WIN32
		BOOL completionStatus = GetQueuedCompletionStatus(
			pThread->m_pIocpdataServer->m_hCompletePort,
			&dwBytesTransfered,
			(LPDWORD)&key,
			&pOverlapped,
			INFINITE);
#else
		"unkown platform!!!"
#endif
#endif

			if (!completionStatus)
			{
				pThread->HandleCompletionFailure(pOverlapped, dwBytesTransfered, GetLastError());
				continue;
			}

		// NULL lpContext packet is a special status that unblocks the worker
		// thread to initial a shutdown sequence. The thread should be going
		// down soon.
		if (NULL == key)
		{
			break;
		}

		//
		IocpContext &iocpContext =
			*reinterpret_cast<IocpContext *>(pOverlapped);

		pThread->HandleIocpContext(iocpContext, dwBytesTransfered);


		if (ThreadPool_Conf::TillTaskFinish == pThread->m_emThreadRunOp)
		{
			// 任务处理完毕，退出
			break;
		}

	}

	/*
	if (!EzLog::LogLvlFilter(trace))
	{
	string strTran;
	string strDebug("thread finished id=");
	strDebug += sof_string::itostr((uint64_t)pThread->m_dThreadId, strTran);
	EzLog::Out(ftag, trace, strDebug );
	}
	*/

	pThread->m_emThreadState = ThreadPool_Conf::STOPPED;
	pThread->m_dThreadId = 0;

	return 0;
}


void IOCP_Thread_Server::HandleCompletionFailure(OVERLAPPED * overlapped,
	DWORD bytesTransferred,
	int error)
{
	static const string ftag("IOCP_Thread_Server::HandleCompletionFailure() ");

	if (NULL != overlapped)
	{
		// Process a failed completed I/O request
		// dwError contains the reason for failure
		IocpContext &iocpContext =
			*reinterpret_cast<IocpContext *>(overlapped);
		HandleIocpContext(iocpContext, bytesTransferred);
	}
	else
	{
		// Currently, GetQueuedCompletionStatus is called with an INFINITE
		// timeout flag, so it should not be possible for the status to 
		// timeout.
		assert(WAIT_TIMEOUT != error);

		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			// GetQueuedCompletionStatus failed. Notify the user on this event.
			m_pIocpdataServer->m_pIocpHandler->OnServerError(error);
		}
	}
}

void IOCP_Thread_Server::HandleIocpContext(IocpContext &iocpContext,
	DWORD bytesTransferred)
{
	static const string ftag("IOCP_Thread_Server::HandleIocpContext() ");

	switch (iocpContext.m_type)
	{
	case IocpContext::Accept:
		HandleAccept(iocpContext, bytesTransferred);
		break;

	case IocpContext::Rcv:
		HandleReceive(iocpContext, bytesTransferred);
		break;

	case IocpContext::Send:
		HandleSend(iocpContext, bytesTransferred);
		break;

	case IocpContext::Disconnect:
		HandleDisconnect(iocpContext);
		break;

	default:
		string strTran;
		string strDebug = "unkown type: ";
		strDebug += sof_string::itostr(iocpContext.m_type, strTran);
		EzLog::e(ftag, strDebug);
		break;
	}
}


void IOCP_Thread_Server::HandleAccept(IocpContext &acceptContext, DWORD bytesTransferred)
{
	static const string ftag("IOCP_Thread_Server::HandleAccept() ");

	// We should be accepting immediately without waiting for any data.
	// If this has change, we need to account for that accept buffer and post
	// it to the receive side.
	if (0 == bytesTransferred)
	{
		string strTran;
		string strDebug = "bytesTransferred = ";
		strDebug += sof_string::itostr((uint64_t)bytesTransferred, strTran);
		EzLog::w(ftag, strDebug);
	}

	// Update the socket option with SO_UPDATE_ACCEPT_CONTEXT so that
	// getpeername will work on the accept socket.
	int iRes = setsockopt(acceptContext.m_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		(char *)(&m_pIocpdataServer->m_listenSocket),
		sizeof(m_pIocpdataServer->m_listenSocket));

	if (0 != iRes)
	{
		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			// This shouldn't happen, but if it does, report the error. 
			// Since the connection has not been established, it is not necessary
			// to notify the client to remove any connections.
			m_pIocpdataServer->m_pIocpHandler->OnServerError(WSAGetLastError());
		}
	}
	// If the socket is up, allocate the connection and notify the client.
	else
	{
		struct ConnectionInformation cinfo = WinSockUtil::GetConnectionInformation(acceptContext.m_socket);

		std::shared_ptr<Connection> c(new Connection(
			acceptContext.m_socket,
			m_pIocpdataServer->GetNextId(),
			m_pIocpdataServer->m_rcvBufferSize
			));


		// 接收缓冲区
		int nRecvBuf = 256 * 1024;//设置为32K
		int iSetSocket = setsockopt(acceptContext.m_socket, SOL_SOCKET, SO_RCVBUF, (const char*) &nRecvBuf, sizeof(int));
		if ( 0 != iSetSocket )
		{
			int i = 0;
		}
		//发送缓冲区
		int nSendBuf = 256 * 1024;//设置为32K
		iSetSocket = setsockopt(acceptContext.m_socket, SOL_SOCKET, SO_SNDBUF, (const char*) &nSendBuf, sizeof(int));
		if ( 0 != iSetSocket )
		{
			int i = 0;
		}



		m_pIocpdataServer->m_connectionManager.AddConnection(c);

		IOCPUtil::RegisterIocpHandle((HANDLE)c->m_socket, (std::shared_ptr<IocpDataBase>)m_pIocpdataServer);

		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			m_pIocpdataServer->m_pIocpHandler->OnNewConnection(c->m_id, cinfo);
		}

		int lasterror = WinSockUtil::PostRecv(c->m_rcvContext);

		// Failed to post a queue a receive context. It is likely that the
		// connection is already terminated at this point (by user or client).
		// In such case, just remove the connection.
		if (WSA_IO_PENDING != lasterror)
		{
			if (true == c->CloseRcvContext())
			{
				WinSockUtil::PostDisconnect((std::shared_ptr<IocpDataBase>)m_pIocpdataServer, *c);
			}
		}
	}

	// Post another Accept context to IOCP for another new connection.
	//! @remark
	//! For higher performance, it is possible to preallocate these sockets
	//! and have a pool of accept context waiting. That adds complexity, and
	//! unnecessary for now.
	WinSockUtil::CreateOverlappedSocket(acceptContext.m_socket);

	if (INVALID_SOCKET != acceptContext.m_socket)
	{
		WinSockUtil::PostAccept(m_pIocpdataServer);
	}
	else
	{
		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			m_pIocpdataServer->m_pIocpHandler->OnServerError(WSAGetLastError());
		}
	}
}


void IOCP_Thread_Server::HandleReceive(IocpContext &rcvContext, DWORD bytesTransferred)
{
	static const string ftag("IOCP_Thread_Server::HandleReceive() ");

	std::shared_ptr<Connection> c =
		m_pIocpdataServer->m_connectionManager.GetConnection(rcvContext.m_cid);
	if (NULL == c)
	{
		string strTran;
		string strDebug = "getConnection NULL id= ";
		strDebug += sof_string::itostr(rcvContext.m_cid, strTran);
		EzLog::w(ftag, strDebug);
		return;
	}

	// If nothing is transferred, we are about to be disconnected. In this
	// case, don't notify the client that nothing is received because
	// they are about to get a disconnection callback.
	if (0 != bytesTransferred)
	{
		// Shrink the buffer to fit the byte transferred
		rcvContext.m_data.resize(bytesTransferred);
		assert(rcvContext.m_data.size() == bytesTransferred);

		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			// Invoke the callback for the client
			m_pIocpdataServer->m_pIocpHandler->OnReceiveData(
				rcvContext.m_cid,
				rcvContext.m_data);
		}
	}

	// Resize it back to the original buffer size and prepare to post
	// another completion status.
	rcvContext.m_data.resize(rcvContext.m_rcvBufferSize);
	rcvContext.ResetWsaBuf();

	int lastError = NO_ERROR;

	// 0 bytes transferred, or if a recv context can't be posted to the 
	// IO completion port, that implies the socket at least half-closed.
	if ((0 == bytesTransferred) ||
		WSA_IO_PENDING != (lastError = WinSockUtil::PostRecv(rcvContext)))
	{
		uint64_t cid = rcvContext.m_cid;
		if (c->CloseRcvContext() == true)
		{
			::shutdown(c->m_socket, SD_RECEIVE);

			if (NULL != m_pIocpdataServer->m_pIocpHandler)
			{
				m_pIocpdataServer->m_pIocpHandler->OnClientDisconnect(cid, lastError);
			}
		}
	}
}

void IOCP_Thread_Server::HandleSend(IocpContext &iocpContext, DWORD bytesTransferred)
{
	static const string ftag("IOCP_Thread_Server::HandleSend() ");

	std::shared_ptr<Connection> c =
		m_pIocpdataServer->m_connectionManager.GetConnection(iocpContext.m_cid);
	if (c == NULL)
	{
		assert(false);
		return;
	}

	uint64_t cid = iocpContext.m_cid;

	if (bytesTransferred > 0)
	{
		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			m_pIocpdataServer->m_pIocpHandler->OnSentData(cid, bytesTransferred);
		}
	}
	//No bytes transferred, that means send has failed.
	else
	{
		// what to do here?
	}

	//! @remark
	//! Remove the send context after notifying the user. Otherwise
	//! there is a race condition where a disconnect context maybe waiting 
	//! for the send queue to go to zero at the same time. In this case,
	//! the disconnect notification will come before we notify the user.
	size_t outstandingSend = c->m_sendQueue.RemoveSendContext(&iocpContext);

	// If there is no outstanding send context, that means all sends 
	// are completed for the moment. At this point, if we have a half-closed 
	// socket, and the connection is pending to be disconnected, post a 
	// disconnect context for a graceful shutdown.
	if (0 == outstandingSend)
	{
		if ((::InterlockedExchangeAdd(&c->m_rcvClosed, 0) > 0) &&
			(::InterlockedExchangeAdd(&c->m_disconnectPending, 0) > 0))
		{
			// Disconnect context is special (hacked) because it is not
			// tied to a connection. During graceful shutdown, it is very
			// difficult to determine when exactly is a good time to 
			// remove the connection. For example, a disconnect context 
			// might have already been sent by the user code, and you
			// wouldn't know it unless mutex are used. To keep it as 
			// lock-free as possible, the disconnect handler
			// will gracefully handle redundant disconnect context.
			WinSockUtil::PostDisconnect((std::shared_ptr<IocpDataBase>)m_pIocpdataServer, *c);
		}
	}
}

void IOCP_Thread_Server::HandleDisconnect(IocpContext &iocpContext)
{
	static const string ftag("IOCP_Thread_Server::HandleDisconnect() ");

	uint64_t cid = iocpContext.m_cid;

	// Disconnect context isn't tied to the connection. Therefore, it must
	// be deleted manually at all times.
	delete &iocpContext;

	std::shared_ptr<Connection> c =
		m_pIocpdataServer->m_connectionManager.GetConnection(cid);
	if (c == NULL)
	{
		//! @remark
		//! Disconnect context may come from several different source at 
		//! once, and only the first one to remove the connection. If the
		//! connection is already removed, gracefully handle it.

		//std::cout <<"Extra disconnect packet received" << std::endl;
		return;
	}

	if (c->HasOutstandingContext() == true)
	{
		return;
	}

	if (true == m_pIocpdataServer->m_connectionManager.RemoveConnection(cid))
	{
		if (NULL != m_pIocpdataServer->m_pIocpHandler)
		{
			m_pIocpdataServer->m_pIocpHandler->OnDisconnect(cid, 0);
		}
	}
}
