#ifndef __WIN_IOCP_DEFINE_H__
#define __WIN_IOCP_DEFINE_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

#include "boost/shared_ptr.hpp"

#include "IocpContext.h"

/*
Windows IOCP define.
*/
namespace Win_IOCP
{
	static const int ciDefaultRcvBufferSize = 4096;
}

#endif