#include "Connection.h"

#include "IocpContext.h"

#include "tool_net/PacketAssembler.h"

using namespace std;

Connection::Connection(SOCKET socket, uint64_t cid, int rcvBufferSize)
: m_socket(socket)
, m_id(cid)
, m_disconnectPending(false)
, m_sendClosePending(false)
, m_rcvClosed(false)
, m_rcvContext(m_socket, m_id, IocpContext::Rcv, rcvBufferSize)
, m_disconnectContext(m_socket, m_id, IocpContext::Disconnect, 0)
{

}

Connection::~Connection(void)
{
	closesocket(m_socket);
}

std::shared_ptr<IocpContext> Connection::CreateSendContext()
{
	std::shared_ptr<IocpContext> c( new IocpContext(m_socket, m_id, IocpContext::Send,0)	);

	m_sendQueue.AddSendContext(c);

	return c;
}

bool Connection::HasOutstandingContext()
{
	if(::InterlockedExchangeAdd(&m_rcvClosed, 0) == 0)
	{
		return true;
	}

	if(m_sendQueue.NumOutstandingContext() > 0)
	{
		return true;
	}

	return false;
}


bool Connection::CloseRcvContext()
{
	if (0 == ::InterlockedExchange(&m_rcvClosed, 1))
	{
		if(INVALID_HANDLE_VALUE != m_rcvContext.hEvent )
		{
			CloseHandle(m_rcvContext.hEvent);
			m_rcvContext.hEvent = INVALID_HANDLE_VALUE;
		}

		return true;
	}

	return false;
}