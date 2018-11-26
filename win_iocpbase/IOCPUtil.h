#ifndef __IOCP_UTIL_H__
#define __IOCP_UTIL_H__

#include <memory>

#include "win_iocpbase/IocpDataBase.h"

/*
IOCP utility.
*/
namespace IOCPUtil
{
	int InitializeIocp(const DWORD dwNumConcurrentThreads, HANDLE& out_hCompPort);

	// Associate the socket with the completion port
	int RegisterIocpHandle( HANDLE h, std::shared_ptr<IocpDataBase>& pIocpdata );
};

#endif