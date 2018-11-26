#ifndef __SEND_QUEUE_H__
#define __SEND_QUEUE_H__

#include <memory>

#include "boost/thread/mutex.hpp"

#include "IocpContext.h"

/*
Send Queue buffer.
*/
class SendQueue
{
	//
	// Members
	//
protected:
	typedef std::map< IocpContext*, std::shared_ptr<IocpContext> > SendContextMap_t;
	SendContextMap_t m_sendContextMap;

	boost::mutex m_mutex;

	//
	// Functions
	//
public:
	SendQueue(void);
	virtual ~SendQueue(void);

	void AddSendContext(std::shared_ptr<IocpContext> sendContext);

	size_t RemoveSendContext(IocpContext *sendContext);

	void CloseAllSends();

	size_t NumOutstandingContext();
};

#endif