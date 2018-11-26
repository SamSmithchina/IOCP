#ifndef __IOCP_DATA_CLIENT_H__
#define __IOCP_DATA_CLIENT_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

#include "boost/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"

#include "win_iocpbase/IocpDataBase.h"
#include "win_iocpbase/IocpContext.h"
#include "win_iocpbase/ConnectionManager.h"

/*
IOCP data for client.
*/
class IocpData_Client : public IocpDataBase
{
	//
	// Members
	//
public:
	SOCKET m_connectSocket;

	//
	// Functions
	//
public:
	IocpData_Client( void )
		: IocpDataBase()
		, m_connectSocket( INVALID_SOCKET )
	{
	}

	virtual ~IocpData_Client( void )
	{ }
};

#endif