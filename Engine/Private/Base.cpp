#include "Base.h"

//Reference Count에서는 mutex쓰면 성능이 떨어지므로 atomic을 사용하	여 lock-free하게 구현해야함 
CBase::CBase()
{
}

CBase::CBase(const CBase& rhs)
	:m_iRefCnt(0)
{
}

_uint CBase::AddRef()
{
	//return ++m_iRefCnt 에는 memory_order_seq_cst (재배치금지)옵션이 걸려있는데 ++때는 보장이 중요치않아서 다음과 같은 오더링 사용 
	return m_iRefCnt.fetch_add(1, memory_order_relaxed) + 1;
}

_uint CBase::Release()
{
	_uint iRefCnt = m_iRefCnt.load(memory_order_relaxed);

	//0일때 fetch_sub하면 언더플로우가 발생하므로 CAS(Compare_and_Swap) 루프가 필요함
	while (iRefCnt != 0)
	{
		//compare_exchange_weak : Compare하고 Swap해라. 그 찰나에	 다른 스레드가 변경하면 실패하고 false를 반환하고 iRefCnt에 최신값을 넣어줌. 
		//실패시 while문으로 다시 CAS시도함.
		if(m_iRefCnt.compare_exchange_weak(iRefCnt, iRefCnt - 1, memory_order_release, memory_order_relaxed))
		{
			return iRefCnt;
		}
	}

	// atomic_thread_fence : 다른 스레드들이 이 객체의 메모리 변경을 내 캐시로 가져오기 전까지 대기한다 
	atomic_thread_fence(memory_order_acquire);

	Free();
	delete this;
	return 0;
	
}

void CBase::Free()
{
}
