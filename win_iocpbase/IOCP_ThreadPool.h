#ifndef __IOCP_THREADPOOL_H__
#define __IOCP_THREADPOOL_H__

#include <memory>

#include "thread_pool_base/threadpoolbase.h"
#include "WinIOCPDefine.h"

#include "util/EzLog.h"

/*
IOCP thread pool.
*/
template <typename T_iocpdata, typename T_thread>
class IOCP_ThreadPool :
	public ThreadPoolBase<T_thread>
{
	//
	// Members
	//
protected:
	std::shared_ptr<T_iocpdata> m_pIocpdata;

	//
	// Functions
	//
public:
	IOCP_ThreadPool( const unsigned int uiNum, std::shared_ptr<T_iocpdata>& pIocpData )
		: m_pIocpdata(pIocpData), ThreadPoolBase(uiNum)
	{
	}

	virtual ~IOCP_ThreadPool(void)
	{
	}

	/*
	创建线程池对象

	Return :
	0 -- 成功
	-1 -- 失败
	*/
	virtual int InitPool(void)
	{
		static const string ftag("IOCP_ThreadPool::InitPool() ");

		m_pool_state = ThreadPool_Conf::STOPPED;

		unsigned int i = 0;
		for( i = 0; i < m_uiThreadMaxNums; ++i )
		{
			std::shared_ptr<T_thread> ptrThread = std::shared_ptr<T_thread>( new T_thread() );
			if( NULL == ptrThread )
			{
				EzLog::e(ftag, "new Thread() failed, NULL");

				return -1;
			}

			ptrThread->SetIocpData(m_pIocpdata);

			m_hdThreads.push_back(ptrThread);

			++m_uiCurrThreadNums;
		}

		return 0;
	}

private:
	// 阻止使用默认构造函数
	IOCP_ThreadPool(void);
};

#endif