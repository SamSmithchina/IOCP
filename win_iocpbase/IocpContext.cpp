#include "IocpContext.h"

IocpContext::IocpContext(SOCKET socket, uint64_t cid, IocpContextType t, int rcvBufferSize)
: m_socket(socket)
, m_cid(cid)
, m_type(t)
, m_rcvBufferSize(rcvBufferSize)
{
	if(Rcv == t || Accept == t)
	{
		m_data.resize(m_rcvBufferSize);
		ResetWsaBuf();
	}

	// Clear out the overlapped struct. Apparently, you must be do this, 
	// otherwise the overlap object will be rejected.
	memset(this, 0, sizeof(OVERLAPPED));
}

IocpContext::~IocpContext(void)
{
}


void IocpContext::ResetWsaBuf()
{
	m_wsaBuffer.buf = reinterpret_cast<char *>(&m_data[0]);
	m_wsaBuffer.len = static_cast<u_long>(m_data.size() * sizeof(m_data[0]));
}