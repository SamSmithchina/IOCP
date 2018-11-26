#ifndef __IOCP_THREAD_SERVER_H__
#define __IOCP_THREAD_SERVER_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#include <memory>

#include "thread_pool_base/threadbase.h"
#include "WinIOCPDefine.h"

class IocpData_Server;

/*
IOCP thread for server.
*/
class IOCP_Thread_Server :
	public ThreadBase
{
	//
	// Members
	//
protected:
	std::shared_ptr<IocpData_Server> m_pIocpdataServer;

	//
	// Functions
	//
public:
	IOCP_Thread_Server( void );
	virtual ~IOCP_Thread_Server( void );

	void SetIocpData( std::shared_ptr<IocpData_Server>& pIocpdata );

	/*
	启动线程

	Return :
	0 -- 启动成功
	-1 -- 启动失败
	*/
	virtual int StartThread(void);

protected:
	static DWORD WINAPI Run(LPVOID  pParam);

	void HandleCompletionFailure(OVERLAPPED * overlapped, 
		DWORD bytesTransferred, 
		int error );

	void HandleIocpContext(IocpContext &iocpContext, DWORD bytesTransferred );

	void HandleAccept( IocpContext &acceptContext, DWORD bytesTransferred );

	void HandleReceive( IocpContext &rcvContext, DWORD bytesTransferred );

	void HandleSend( IocpContext &iocpContext, DWORD bytesTransferred );

	void HandleDisconnect( IocpContext &iocpContext );
};

#endif