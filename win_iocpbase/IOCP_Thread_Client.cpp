#include "IOCP_Thread_Client.h"

#include "util/EzLog.h"

#include "win_iocpbase/WinSockUtil.h"
#include "win_iocpbase/WinSock_Client.h"
#include "IOCPUtil.h"
#include "socket_conn_manage_base/ConnectionInformation.h"
#include "Connection.h"

using namespace std;

IOCP_Thread_Client::IOCP_Thread_Client(void)
{
}

IOCP_Thread_Client::~IOCP_Thread_Client(void)
{
}

void IOCP_Thread_Client::SetIocpData(std::shared_ptr<IocpData_Client>& pIocpdata)
{
	m_pIocpdata = pIocpdata;
}

/*
启动线程

Return :
0 -- 启动成功
-1 -- 启动失败
*/
int IOCP_Thread_Client::StartThread(void)
{
	// static const string ftag("IOCP_Thread_Client::StartThread() ");

	int iRes = ThreadBase::StartThread(&IOCP_Thread_Client::Run);

	return iRes;
}


DWORD WINAPI IOCP_Thread_Client::Run(LPVOID  pParam)
{
	static const string ftag("IOCP_Thread_Client::Run() ");

	IOCP_Thread_Client* pThread = static_cast<IOCP_Thread_Client*>(pParam);

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
			pThread->m_pIocpdata->m_hCompletePort,
			&dwBytesTransfered,
			(PULONG_PTR)&key,
			&pOverlapped,
			INFINITE);
#else
#ifdef WIN32
		BOOL completionStatus = GetQueuedCompletionStatus(
			pThread->m_pIocpdata->m_hCompletePort,
			&dwBytesTransfered,
			(LPDWORD)&key,
			&pOverlapped,
			INFINITE);
#else
		"unkown platform!"
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


void IOCP_Thread_Client::HandleCompletionFailure(OVERLAPPED * overlapped,
	DWORD bytesTransferred,
	int error)
{
	// static const string ftag("IOCP_Thread_Client::HandleCompletionFailure() ");

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

		if (NULL != m_pIocpdata->m_pIocpHandler)
		{
			// GetQueuedCompletionStatus failed. Notify the user on this event.
			m_pIocpdata->m_pIocpHandler->OnServerError(error);
		}
	}
}

void IOCP_Thread_Client::HandleIocpContext(IocpContext &iocpContext,
	DWORD bytesTransferred)
{
	static const string ftag("IOCP_Thread_Client::HandleIocpContext() ");

	switch (iocpContext.m_type)
	{
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

void IOCP_Thread_Client::HandleReceive(IocpContext &rcvContext, DWORD bytesTransferred)
{
	static const string ftag("IOCP_Thread_Client::HandleReceive() ");

	std::shared_ptr<Connection> c =
		m_pIocpdata->m_connectionManager.GetConnection(rcvContext.m_cid);
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

		if (NULL != m_pIocpdata->m_pIocpHandler)
		{
			// Invoke the callback for the client
			m_pIocpdata->m_pIocpHandler->OnReceiveData(
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

			if (NULL != m_pIocpdata->m_pIocpHandler)
			{
				m_pIocpdata->m_pIocpHandler->OnClientDisconnect(cid, lastError);
			}
		}
	}
}

void IOCP_Thread_Client::HandleSend(IocpContext &iocpContext, DWORD bytesTransferred)
{
	// static const string ftag("IOCP_Thread_Client::HandleSend() ");

	std::shared_ptr<Connection> c =
		m_pIocpdata->m_connectionManager.GetConnection(iocpContext.m_cid);
	if (c == NULL)
	{
		assert(false);
		return;
	}

	uint64_t cid = iocpContext.m_cid;

	if (bytesTransferred > 0)
	{
		if (NULL != m_pIocpdata->m_pIocpHandler)
		{
			m_pIocpdata->m_pIocpHandler->OnSentData(cid, bytesTransferred);
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
			WinSockUtil::PostDisconnect((std::shared_ptr<IocpDataBase>)m_pIocpdata, *c);
		}
	}
}

void IOCP_Thread_Client::HandleDisconnect(IocpContext &iocpContext)
{
	// static const string ftag("IOCP_Thread_Client::HandleDisconnect() ");

	uint64_t cid = iocpContext.m_cid;

	// Disconnect context isn't tied to the connection. Therefore, it must
	// be deleted manually at all times.
	delete &iocpContext;

	std::shared_ptr<Connection> c =
		m_pIocpdata->m_connectionManager.GetConnection(cid);
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

	if (true == m_pIocpdata->m_connectionManager.RemoveConnection(cid))
	{
		if (NULL != m_pIocpdata->m_pIocpHandler)
		{
			m_pIocpdata->m_pIocpHandler->OnDisconnect(cid, 0);
		}
	}
}
