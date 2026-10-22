#include "ThreadPool.h"
#include <combaseapi.h>

void CLoadBatch::Wait()
{
	unique_lock<mutex> lock(m_Mutex);
	
	//이 함수는 메인스레드가 호출함 
	//메인스레드는 작업이 비어질때까지 Sleep. m_iRemainingTasks가 0이 되면 notify_all()로 깨어남
	m_Condition.wait(lock, [this]() { return m_iRemainingTasks == 0; });
}

HRESULT CThreadPool::Initialize(_uint iThreadCount)
{
	for(_uint i=0; i< iThreadCount; ++i)
	{
		m_Workers.emplace_back(&CThreadPool::Worker_Main, this);
	}
	return S_OK;
}
void CThreadPool::Enqueue(CLoadBatch* pWorker, function<HRESULT()> Job)
{
	{
		lock_guard<mutex> lock(pWorker->m_Mutex);
		++pWorker->m_iRemainingTasks;
		pWorker->m_iTotalTasks.fetch_add(1, memory_order_relaxed); // 진행률 계산용 전체 작업 수를 기록한다.
	}
	{
		lock_guard<mutex> QueueLock(m_QueueMutex);
		m_Jobs.emplace(pWorker, move(Job));
	}
	//한 명 일어나라 
	m_QueueCondition.notify_one();
}

void CThreadPool::Worker_Main()
{
	//COM객체 사용을 위해 스레드별로 COM 초기화 필요
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	//워커 스레드의 메인 루프 
	while (true)
	{
		pair<CLoadBatch*, function<HRESULT()>> Item;

		{
			unique_lock<mutex> lock(m_QueueMutex);
			//워커 스레드는 작업이 없으면 Sleep
			m_QueueCondition.wait(lock, [this]() { return m_bStop || !m_Jobs.empty(); });

			if (m_bStop && m_Jobs.empty()) break;

			Item = std::move(m_Jobs.front());
			m_Jobs.pop();
		}

		if (FAILED(Item.second()))
			Item.first->Mark_Failed();

		Item.first->m_iCompletedTasks.fetch_add(1, memory_order_relaxed); // 실패한 작업도 실행은 끝났으므로 완료 수에 포함한다.

		{
			std::lock_guard<std::mutex> Lock(Item.first->m_Mutex);
			if (0 == --Item.first->m_iRemainingTasks)
				Item.first->m_Condition.notify_all(); 
		}
	}

	CoUninitialize();
}

CThreadPool* CThreadPool::Create(_uint iThreadCount)
{
	CThreadPool* pInstance = new CThreadPool();
	if (FAILED(pInstance->Initialize(iThreadCount)))
	{
		Safe_Release(pInstance);
		return nullptr;
	}
	return pInstance;
}

void CThreadPool::Free()
{
	__super::Free();
	{
		std::lock_guard<std::mutex> Lock(m_QueueMutex);
		m_bStop = true;
	}
	m_QueueCondition.notify_all();
	for(auto& Worker : m_Workers)
	{
		if (Worker.joinable())
			Worker.join();
	}
}
