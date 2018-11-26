#include "WinSockUtil.h"

#include <memory>

#include "util/EzLog.h"

#include "socket_conn_manage_base/ConnectionInformation.h"

#include <IPHlpApi.h>

#pragma comment(lib,"Iphlpapi.lib")

using namespace std;

namespace WinSockUtil
{
	int InitializeWinsock( void )
	{
		static const string ftag( "InitializeWinsock() " );
		// Initialize Winsock
		WSADATA wsaData;
		int iResult = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
		if ( NO_ERROR != iResult )
		{
			string strTran;
			string strDebug = "Error at WSAStartup with error: ";
			strDebug += sof_string::itostr( (uint64_t) GetLastError(), strTran );
			EzLog::e( ftag, strDebug );

			return -1;
		}

		return 0;
	}

	int CreateOverlappedSocket( SOCKET& out_socket )
	{
		static const string ftag( "CreateOverlappedSocket() " );

		out_socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
		if ( INVALID_SOCKET == out_socket )
		{
			string strTran;
			string strDebug = "Create of OVERLAPPED socket failed with error: ";
			strDebug += sof_string::itostr( WSAGetLastError(), strTran );
			EzLog::e( ftag, strDebug );

			WSACleanup();

			return -1;
		}

		return 0;
	}

	int CreateListenSocket( const string& in_ip, const u_short in_port,
		std::shared_ptr<IocpData_Server>& out_pIocpdata )
	{
		static const string ftag( "CreateListenSocket() " );

		if ( NULL == out_pIocpdata )
		{
			return -1;
		}

		string strTran;
		string strDebug;

		sockaddr_in service;

		string strIp = in_ip;
		u_short port = in_port;

		if ( strIp.empty() || 0 == port )
		{
			strDebug = "socket listen error, ip=[";
			strDebug += strIp;
			strDebug += "],port=";
			strDebug += sof_string::itostr( port, strTran );
			strDebug += "]";
			EzLog::e( ftag, strDebug );
			return -1;
		}

		//----------------------------------------
		// Create socket
		SOCKET hListenSocket = INVALID_SOCKET;
		int iResult = WinSockUtil::CreateOverlappedSocket( hListenSocket );
		if ( 0 != iResult )
		{
			return -1;
		}

		//----------------------------------------
		// Bind the listening socket to the local IP address
		//GetLocalEthernetIp(strIp);

		ZeroMemory( (char *) &service, sizeof( service ) );
		service.sin_family = AF_INET;
		//service.sin_addr.s_addr = inet_addr( strIp.c_str() );
		service.sin_addr.s_addr = htonl(INADDR_ANY);
		service.sin_port = htons( port );

		if ( SOCKET_ERROR == ::bind( hListenSocket, (SOCKADDR *) & service, sizeof( service ) ) )
		{
			string strTran;
			string strDebug = "bind failed with error: ";
			strDebug += sof_string::itostr( WSAGetLastError(), strTran );
			EzLog::e( ftag, strDebug );

			closesocket( hListenSocket );
			WSACleanup();
			return 1;
		}

		BOOL bReuseaddr = TRUE;
		// Set Reuse
		if (SOCKET_ERROR == ::setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bReuseaddr, sizeof(bReuseaddr)))
		{
			string strTran;
			string strDebug = "set reuse with error: ";
			strDebug += sof_string::itostr(WSAGetLastError(), strTran);
			EzLog::e(ftag, strDebug);

			closesocket(hListenSocket);
			WSACleanup();
			return 1;
		}

		//----------------------------------------
		// Start listening on the listening socket
		iResult = listen( hListenSocket, SOMAXCONN );
		if ( SOCKET_ERROR == iResult )
		{
			string strTran;
			string strDebug = "listen failed with error: ";
			strDebug += sof_string::itostr( WSAGetLastError(), strTran );
			EzLog::e( ftag, strDebug );

			closesocket( hListenSocket );
			WSACleanup();
			return -1;
		}

		out_pIocpdata->m_lpfnAcceptEx = WinSockUtil::LoadAcceptEx( hListenSocket );
		if ( NULL == out_pIocpdata->m_lpfnAcceptEx )
		{
			return -1;
		}

		out_pIocpdata->m_listenSocket = hListenSocket;

		strDebug = "Listening on address: ";
		strDebug += strIp;
		strDebug += ":";
		strDebug += sof_string::itostr( port, strTran );
		EzLog::i( ftag, strDebug );

		return 0;
	}

	LPFN_ACCEPTEX LoadAcceptEx( SOCKET& s )
	{
		static const string ftag( "LoadAcceptEx() " );

		LPFN_ACCEPTEX lpfnAcceptEx = NULL;
		DWORD dwBytes = 0;

		GUID GuidAcceptEx = WSAID_ACCEPTEX;

		// Load the AcceptEx function into memory using WSAIoctl.
		// The WSAIoctl function is an extension of the ioctlsocket()
		// function that can use overlapped I/O. The function's 3rd
		// through 6th parameters are input and output buffers where
		// we pass the pointer to our AcceptEx function. This is used
		// so that we can call the AcceptEx function directly, rather
		// than refer to the Mswsock.lib library.
		int iResult = WSAIoctl( s, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx, sizeof( GuidAcceptEx ),
			&lpfnAcceptEx, sizeof( lpfnAcceptEx ),
			&dwBytes, NULL, NULL );
		if ( SOCKET_ERROR == iResult )
		{
			string strTran;
			string strDebug = "WSAIoctl failed with error: ";
			strDebug += sof_string::itostr( WSAGetLastError(), strTran );
			EzLog::e( ftag, strDebug );

			closesocket( s );
			s = INVALID_SOCKET;

			WSACleanup();
			return NULL;
		}

		return lpfnAcceptEx;
	}

	//****************************************************************************
	//! @details
	//! This method extracts the Connection Information object in real time.
	//! Therefore, the accept socket must be valid.
	//!
	//! @param[in] socket 
	//! The socket to extract connection information from
	//!
	//! @return ndc::tcpip::ConnectionInformation
	//! Returns the Connection Information based on the accept socket
	//!
	//! @remark
	//! If the returned Connection Information object holds no information,
	//! it implies that the function has failed.
	//!
	//****************************************************************************
	struct ConnectionInformation GetConnectionInformation(SOCKET socket)
	{
		struct ConnectionInformation ci;

		// initialize the sockaddr_in object and its length
		sockaddr_in name;
		memset( &name, 0, sizeof( name ) );
		int namelen = sizeof( name );

		// getpeername to extract the remote party information
		if ( ::getpeername( socket, (sockaddr *) &name, &namelen ) != 0 )
		{
			return ci;
		}

#ifdef UNICODE
		// Unicode version of converting address to a unicode string
		int ansiLen = lstrlenA(inet_ntoa(name.sin_addr));
		LPWSTR unicodeIp = (LPWSTR)malloc((ansiLen) * sizeof(TCHAR));
		if(::MultiByteToWideChar(
			CP_ACP, 
			0, 
			inet_ntoa(name.sin_addr), 
			ansiLen, 
			unicodeIp, 
			ansiLen) != 0)
		{
			// The converted unicode IP address string is not NULL
			// terminated. So we need to append the exact count.
			ci.m_remoteIpAddress.append(unicodeIp, ansiLen);
		}

		free(unicodeIp);

#else
		ci.strRemoteIpAddress = inet_ntoa( name.sin_addr );
#endif
		ci.remotePortNumber = ntohs( name.sin_port );

		// Call getnameinfo to extract the hostname from the IP address
		TCHAR hostname[NI_MAXHOST] = { 0 };
		TCHAR servInfo[NI_MAXSERV] = { 0 };

#ifdef UNICODE
		if(GetNameInfoW(
			(sockaddr *) &name,
			sizeof (sockaddr), 
			hostname, // hostname
			NI_MAXHOST, // size of host name
			servInfo,  // service info = port
			NI_MAXSERV, // size of service info
			NI_NUMERICSERV) != 0) // use numeric port option
		{
			return ci;
	}
#else
		if ( getnameinfo(
			(sockaddr *) &name,
			sizeof( sockaddr ),
			hostname, // hostname
			NI_MAXHOST, // size of host name
			servInfo,  // service info = port
			NI_MAXSERV, // size of service info
			NI_NUMERICSERV ) != 0 ) // use numeric port option
		{
			return ci;
		}
#endif 

		ci.strRemoteHostName = hostname;

		return ci;
}

	void PostAccept( std::shared_ptr<IocpData_Server> &iocpData )
	{
		static const string ftag( "PostAccept() " );

		/*
		see :
		https://msdn.microsoft.com/ZH-CN/library/windows/desktop/ms737524
		dwLocalAddressLength [in]
		The number of bytes reserved for the local address information. This value must be at least 16 bytes more than the maximum address length for the transport protocol in use.
		dwRemoteAddressLength [in]
		The number of bytes reserved for the remote address information. This value must be at least 16 bytes more than the maximum address length for the transport protocol in use. Cannot be zero.
		The buffer size for the local and remote address must be 16 bytes more than the size of the sockaddr structure for the transport protocol in use because the addresses are written in an internal format. For example, the size of a sockaddr_in (the address structure for TCP/IP) is 16 bytes. Therefore, a buffer size of at least 32 bytes must be specified for the local and remote addresses.
		*/

		DWORD bytesReceived_ = 0;
		DWORD addressSize = sizeof( sockaddr_in ) + 16;

		OVERLAPPED ov;
		memset( &ov, 0, sizeof( OVERLAPPED ) );

		BOOL bRetVal = iocpData->m_lpfnAcceptEx( iocpData->m_listenSocket,  // listen socket
			iocpData->m_acceptContext.m_socket, // accept socket
			&iocpData->m_acceptContext.m_data[0], // holds local/remote address
			0, // receive data length = 0 for no receive on accept
			addressSize, // local address length
			addressSize, // remote address length
			&bytesReceived_,
			&( iocpData->m_acceptContext )
			);

		if ( !bRetVal )
		{
			DWORD lastError = WSAGetLastError();
			if ( lastError != ERROR_IO_PENDING )
			{
				string strTran;
				string strDebug = "lpfnAcceptEx failed with error: ";
				strDebug += sof_string::itostr( (uint64_t) lastError, strTran );
				EzLog::e( ftag, strDebug );

				if ( NULL != iocpData->m_pIocpHandler )
				{
					iocpData->m_pIocpHandler->OnServerError( lastError );
				}
			}
		}
		else
		{
			BOOL bRetVal2 = PostQueuedCompletionStatus(
				iocpData->m_hCompletePort,
				0,
				(DWORD)
				(ULONG_PTR) &iocpData,
				&( iocpData->m_acceptContext ) );
			if ( !bRetVal2 )
			{
				string strTran;
				string strDebug = "PostQueuedCompletionStatus failed with error: ";
				strDebug += sof_string::itostr( (uint64_t) GetLastError(), strTran );
				EzLog::e( ftag, strDebug );
			}
		}
	}

	int PostRecv( IocpContext &iocpContext )
	{
		DWORD dwBytes = 0, dwFlags = 0;

		int iRes = WSARecv(
			iocpContext.m_socket,
			&iocpContext.m_wsaBuffer,
			1,
			&dwBytes,
			&dwFlags,
			&iocpContext,
			NULL );

		if ( SOCKET_ERROR == iRes )
		{
			return WSAGetLastError();
		}

		return WSA_IO_PENDING;
	}

	int PostSend( IocpContext &iocpContext )
	{
		DWORD dwBytes = 0;

		if ( WSASend(
			iocpContext.m_socket,
			&iocpContext.m_wsaBuffer,
			1,
			&dwBytes,
			0,
			&iocpContext,
			NULL ) == SOCKET_ERROR )
		{
			return WSAGetLastError();
		}

		return WSA_IO_PENDING;
	}

	int PostDisconnect( std::shared_ptr<IocpDataBase> &iocpData, Connection &c )
	{
		IocpContext * disconnectContext = new IocpContext(
			c.m_socket,
			c.m_id,
			IocpContext::Disconnect,
			0 );

		//Help threads get out of blocking - GetQueuedCompletionStatus()
		BOOL bRes = PostQueuedCompletionStatus(
			iocpData->m_hCompletePort,
			0,
			(ULONG_PTR) &iocpData,
			(LPOVERLAPPED) disconnectContext );

		if ( !bRes )
		{
			return GetLastError();
		}

		return NO_ERROR;
	}

	/*
	取本地以太网IP地址
	*/
	int GetLocalEthernetIp( string& out_strIp )
	{

		//PIP_ADAPTER_INFO结构体指针存储本机网卡信息
		PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
		//得到结构体大小,用于GetAdaptersInfo参数
		unsigned long stSize = sizeof( IP_ADAPTER_INFO );
		//调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量;其中stSize参数既是一个输入量也是一个输出量
		int nRel = GetAdaptersInfo( pIpAdapterInfo, &stSize );
		//记录每张网卡上的IP地址数量
		int IPnumPerNetCard = 0;
		if ( ERROR_BUFFER_OVERFLOW == nRel )
		{
			//如果函数返回的是ERROR_BUFFER_OVERFLOW
			//则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
			//这也是说明为什么stSize既是一个输入量也是一个输出量
			//释放原来的内存空间
			delete pIpAdapterInfo;
			//重新申请内存空间用来存储所有网卡信息
			pIpAdapterInfo = ( PIP_ADAPTER_INFO )new BYTE[stSize];
			//再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
			nRel = GetAdaptersInfo( pIpAdapterInfo, &stSize );
		}
		if ( ERROR_SUCCESS == nRel )
		{
			//输出网卡信息
			//可能有多网卡,因此通过循环去判断
			while ( pIpAdapterInfo )
			{
				if ( pIpAdapterInfo->Type == MIB_IF_TYPE_ETHERNET )
				{
					//可能网卡有多IP,因此通过循环去判断
					IP_ADDR_STRING *pIpAddrString = &( pIpAdapterInfo->IpAddressList );

					if ( pIpAddrString )
					{
						out_strIp = pIpAddrString->IpAddress.String;
						//pIpAddrString = pIpAddrString->Next;
					}

					break;
				}
				pIpAdapterInfo = pIpAdapterInfo->Next;
			}

		}

		while ( pIpAdapterInfo )
		{
			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
		//释放内存空间
		if ( pIpAdapterInfo )
		{
			delete pIpAdapterInfo;
		}

		return 0;
	}
};

