#ifndef __CONNECTION_MANAGER_H__
#define __CONNECTION_MANAGER_H__

#include <memory>

#include "boost/thread/mutex.hpp"

#include "Connection.h"

/*
Connection Manager.
*/
class ConnectionManager
{
	//
	// Members
	//
protected:
	typedef std::map<uint64_t,std::shared_ptr<Connection>> ConnMap_t;

	ConnMap_t m_connMap;

	boost::mutex m_mutex;
	
	//
	// Functions
	//
public:
	ConnectionManager(void);
	virtual ~ConnectionManager(void);

	void AddConnection(std::shared_ptr<Connection> client);

	bool RemoveConnection(uint64_t clientId);

	std::shared_ptr<Connection> GetConnection(uint64_t clientId);

	void CloseAllConnections();

	int GetAllConnectionIds(std::vector<uint64_t>& out_vIds);	
};

#endif