#ifndef __IOCP_THREAD_H__
#define __IOCP_THREAD_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#include <memory>

#include "thread_pool_base/threadbase.h"
#include "WinIOCPDefine.h"
#include "IocpData_Client.h"

/*
IOCP thread for client.
*/
class IOCP_Thread_Client :
	public ThreadBase
{
	//
	// Members
	//
protected:
	std::shared_ptr<IocpData_Client> m_pIocpdata;

	//
	// Functions
	//
public:
	IOCP_Thread_Client( void );
	virtual ~IOCP_Thread_Client( void );

	void SetIocpData( std::shared_ptr<IocpData_Client>& pIocpdata );

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

	void HandleReceive( IocpContext &rcvContext, DWORD bytesTransferred );

	void HandleSend( IocpContext &iocpContext, DWORD bytesTransferred );

	void HandleDisconnect( IocpContext &iocpContext );
};

#endif