#include "IocpEventHandler.h"

#include "tool_string/sof_string.h"
#include "util/EzLog.h"

#include "socket_conn_manage_base/ConnectionInformation.h"

using namespace std;

IocpEventHandler::IocpEventHandler(void)
	: m_pServiceOwner(nullptr)
{
}

IocpEventHandler::~IocpEventHandler(void)
{
}

void IocpEventHandler::OnServerError(int errorCode)
{
}

void IocpEventHandler::OnNewConnection(uint64_t cid, struct ConnectionInformation const & c)
{
	static const string ftag("IocpEventHandler::OnNewConnection() ");

	string strTran;
	string strDebug = "new id=";
	strDebug += sof_string::itostr(cid, strTran);
	EzLog::i(ftag, strDebug);
}

void IocpEventHandler::OnReceiveData(uint64_t cid, std::vector<uint8_t> const &data)
{
	static const string ftag("IocpEventHandler::OnReceiveData() ");

	string strTran;
	string strDebug = "id=";
	strDebug += sof_string::itostr(cid, strTran);
	strDebug += " data=[";
	std::vector<uint8_t>::const_iterator it;
	for (it = data.begin(); it != data.end(); ++it)
	{
		strDebug += *it;
		strDebug += ",";
	}
	strDebug += "]";
	EzLog::i(ftag, strDebug);
}

void IocpEventHandler::OnSentData(uint64_t cid, uint64_t byteTransferred)
{
	static const string ftag("IocpEventHandler::OnSentData() ");

	string strTran;
	string strDebug = "id=";
	strDebug += sof_string::itostr(cid, strTran);
	EzLog::i(ftag, strDebug);
}

void IocpEventHandler::OnClientDisconnect(uint64_t cid, int errorcode)
{
	static const string ftag("IocpEventHandler::OnClientDisconnect() ");

	string strTran;
	string strDebug = "id=";
	strDebug += sof_string::itostr(cid, strTran);
	EzLog::i(ftag, strDebug);

	try
	{
		GetServiceOwner().Shutdown(cid, SD_SEND);

		GetServiceOwner().Disconnect(cid);
	}
	catch (exception const & e)
	{
		EzLog::ex(ftag, e);
		return;
	}
}

void IocpEventHandler::OnDisconnect(uint64_t cid, int errorcode)
{
	static const string ftag("IocpEventHandler::OnDisconnect() ");

	string strTran;
	string strDebug = "id=";
	strDebug += sof_string::itostr(cid, strTran);
	strDebug += " errorcode=";
	strDebug += sof_string::itostr(errorcode, strTran);
	EzLog::i(ftag, strDebug);
}

void IocpEventHandler::OnServerClose(int errorCode)
{
	static const string ftag("IocpEventHandler::OnServerClose() ");

	string strTran;
	string strDebug = " errorcode=";
	strDebug += sof_string::itostr(errorCode, strTran);
	EzLog::i(ftag, strDebug);
}