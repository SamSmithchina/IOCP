#include "SendQueue.h"

#include <memory>

#include "IocpContext.h"

SendQueue::SendQueue(void)
{
}

SendQueue::~SendQueue(void)
{
	// This shouldn't be necessary because the server is programmed to
	// wait for all outstanding context to come back before going down.
	// But just in case...
	CloseAllSends();
}

void SendQueue::AddSendContext( std::shared_ptr<IocpContext> sendContext )
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	bool inserted = m_sendContextMap.insert(
		std::make_pair(sendContext.get(), sendContext)
		).second;
	assert(true == inserted);
}

size_t SendQueue::RemoveSendContext(IocpContext* sendContext)
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);
	m_sendContextMap.erase(sendContext);
	return m_sendContextMap.size();
}

size_t SendQueue::NumOutstandingContext()
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);
	return m_sendContextMap.size();
}

void SendQueue::CloseAllSends()
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	SendContextMap_t::iterator itr = m_sendContextMap.begin();
	while(m_sendContextMap.end() != itr)
	{
		if(INVALID_HANDLE_VALUE != itr->second->hEvent)
		{
			CloseHandle(itr->second->hEvent);
			itr->second->hEvent = INVALID_HANDLE_VALUE;
		}

		++itr;
	}
}