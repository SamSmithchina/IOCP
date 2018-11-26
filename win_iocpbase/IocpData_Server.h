#ifndef __IOCP_DATA_SERVER_H__
#define __IOCP_DATA_SERVER_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

#include "boost/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"

#include "socket_conn_manage_base/EventBaseHandler.h"

#include "ConnectionManager.h"

#include "WinIOCPDefine.h"
#include "IocpDataBase.h"
#include "IocpContext.h"

/*
IOCP data for server.
*/
class IocpData_Server : public IocpDataBase
{
	//
	// Members
	//
public:
	SOCKET m_listenSocket;
	LPFN_ACCEPTEX m_lpfnAcceptEx;

	//
	// Functions
	//
public:
	IocpData_Server( void )
		: IocpDataBase()
		, m_listenSocket( INVALID_SOCKET )
		, m_lpfnAcceptEx( NULL )
	{
	}

	virtual ~IocpData_Server( void )
	{
	}

};

#endif