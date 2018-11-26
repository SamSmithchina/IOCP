#include "WinSock_Client.h"

#include "util/EzLog.h"

#include "socket_conn_manage_base/ConnectionInformation.h"

#include <IPHlpApi.h>

#pragma comment(lib,"Iphlpapi.lib")

using namespace std;

namespace WinSock_Client
{
	int CreateClientSocket( const string& in_ip, const u_short in_port,
		SOCKET& out_clientSocket )
	{
		static const string ftag( "CreateClientSocket() " );

		string strTran;
		string strDebug;

		//sockaddr_in service;

		string strIp = in_ip;
		u_short port = in_port;

		//----------------------------------------
		// Create socket
		SOCKET hClientSocket = INVALID_SOCKET;
		int iResult = WinSockUtil::CreateOverlappedSocket( hClientSocket );
		if ( 0 != iResult )
		{
			return -1;
		}

		//----------------------------------------
		// connect to server.
		sockaddr_in serAddr;
		serAddr.sin_family = AF_INET;
		serAddr.sin_port = htons( port );
		serAddr.sin_addr.S_un.S_addr = inet_addr( strIp.c_str() );
		iResult = connect( hClientSocket, (sockaddr *) &serAddr, sizeof( serAddr ) );
		if ( SOCKET_ERROR == iResult )
		{
			//¡¨Ω” ß∞‹
			string strTran;
			std::string strDebug( "connect failed with error:" );
			strDebug += sof_string::itostr( WSAGetLastError(), strTran );
			EzLog::e( ftag, strDebug );

			closesocket( hClientSocket );
			hClientSocket = INVALID_SOCKET;
			WSACleanup();
			return -1;
		}

		out_clientSocket = hClientSocket;

		strDebug = "connect success on address: ";
		strDebug += strIp;
		strDebug += ":";
		strDebug += sof_string::itostr( port, strTran );
		EzLog::i( ftag, strDebug );

		return 0;
	}

};

