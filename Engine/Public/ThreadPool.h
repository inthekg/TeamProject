#pragma once
#include "Base.h"
#include <thread>
#include <mutex>
#include <condition_variable>

NS_BEGIN(Engine)

class ENGINE_DLL CLoadBatch final
{
public:
	void Wait();

	_bool IsFailed() const
	{
		return m_bFailed.load(memory_order_relaxed);
	}

	void Mark_Failed()
	{
		m_bFailed.store(true, memory_order_relaxed);
	}

public:
	// 진행률 추가
	_uint					Get_TotalTaskCount() const { return m_iTotalTasks.load(memory_order_relaxed); }
	_uint					Get_CompletedTaskCount() const { return m_iCompletedTasks.load(memory_order_relaxed); }

private:
	friend class			CThreadPool;

	mutex					m_Mutex;
	condition_variable		m_Condition;
	_uint					m_iRemainingTasks = 0;
	atomic<_bool>			m_bFailed = false;

private:
	atomic<_uint>			m_iTotalTasks = { 0 };       // 이 배치에 등록된 전체 작업 수
	atomic<_uint>			m_iCompletedTasks = { 0 };   // 성공·실패와 관계없이 실행이 끝난 작업 수
};

class ENGINE_DLL CThreadPool : public CBase
{
private:
	CThreadPool() = default;
	virtual ~CThreadPool() = default;

public:
	HRESULT				Initialize(_uint iThreadCount);
	void				Enqueue(CLoadBatch* pWorker, function<HRESULT()> Job);
	_uint				Get_ThreadCount() const { return static_cast<_uint>(m_Workers.size()); }

private:
	void				Worker_Main();

	queue<pair<CLoadBatch*, function<HRESULT()>>> m_Jobs;
	vector<thread>		m_Workers;
	mutex				m_QueueMutex;
	condition_variable	m_QueueCondition;
	_bool				m_bStop = { false };

public:
	static CThreadPool* Create(_uint iThreadCount);
	virtual void		Free() override;
};

NS_END