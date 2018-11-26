#include "ConnectionManager.h"

#include "util/EzLog.h"

#include "tool_net/PacketAssembler.h"

using namespace std;

ConnectionManager::ConnectionManager(void)
{
}

ConnectionManager::~ConnectionManager(void)
{
}

void ConnectionManager::AddConnection( std::shared_ptr<Connection> client )
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	bool inserted = m_connMap.insert(std::make_pair(
		client->m_id, client
		)).second;

	assert(true == inserted);
}

bool ConnectionManager::RemoveConnection( uint64_t clientId )
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	if( 0 < m_connMap.erase(clientId) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::shared_ptr<Connection> ConnectionManager::GetConnection( uint64_t clientId )
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	ConnMap_t::iterator itr = m_connMap.find(clientId);

	if(m_connMap.end() != itr)
	{
		return itr->second;
	}

	return std::shared_ptr<Connection>();
}

void ConnectionManager::CloseAllConnections()
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	ConnMap_t::iterator itr = m_connMap.begin();
	while(m_connMap.end() != itr)
	{
		CancelIo((HANDLE)itr->second->m_socket);
		++itr;
	}
}

int ConnectionManager::GetAllConnectionIds(std::vector<uint64_t>& out_vIds)
{
	boost::unique_lock<boost::mutex> Locker(m_mutex);

	ConnMap_t::iterator itr = m_connMap.begin();
	while(m_connMap.end() != itr)
	{
		out_vIds.push_back(itr->first);
		++itr;
	}

	return 0;
}
