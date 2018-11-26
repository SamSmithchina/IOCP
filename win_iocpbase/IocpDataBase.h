#ifndef __IOCP_DATA_BASE_H__
#define __IOCP_DATA_BASE_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#include <memory>

#include "boost/thread/mutex.hpp"

#include "IocpEventHandler.h"

#include "ConnectionManager.h"

#include "WinIOCPDefine.h"

#include "IocpContext.h"

/*
Base class for IOCP data.
*/
class IocpDataBase
{
	//
	// Members
	//
public:

	HANDLE m_hCompletePort;
	std::shared_ptr<IocpEventHandler> m_pIocpHandler;

	IocpContext m_acceptContext;
	UINT m_rcvBufferSize;
	ConnectionManager m_connectionManager;

	//
	// Functions
	//
public:
	IocpDataBase( void )
		: m_hCompletePort( INVALID_HANDLE_VALUE )
		, m_acceptContext( INVALID_SOCKET, 0, IocpContext::Accept, Win_IOCP::ciDefaultRcvBufferSize )
		, m_rcvBufferSize( Win_IOCP::ciDefaultRcvBufferSize )
		, m_currentId( 0 )
	{
	}

	virtual ~IocpDataBase( void )
	{
	}

	uint64_t GetNextId( void )
	{
		{
			boost::unique_lock<boost::mutex> Locker( m_cidMutex );
			++m_currentId;
		}

		return m_currentId;
	}

private:
	boost::mutex m_cidMutex;
	uint64_t m_currentId;
};

#endif