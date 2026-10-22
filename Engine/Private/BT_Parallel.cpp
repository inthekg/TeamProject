#include "BT_Parallel.h"

CBT_Parallel::CBT_Parallel(const _wstring& strName, Parallel_Policy eSuccess, Parallel_Policy eFailure)
    : CBT_Composite{ strName }
    , m_eSuccess{ eSuccess }
    , m_eFailure{eFailure}
{
}

CBT_Node::Status CBT_Parallel::Tick()
{
	const _uint iCount = ETOUI(m_Childrens.size());

	if (m_ChildStatus.size() != iCount)
		m_ChildStatus.assign(iCount, Status::Running);

	_uint iSuccess = 0;
	_uint iFailure = 0;

	for (_uint i = 0; i < iCount; ++i)
	{
		if (Status::Running == m_ChildStatus[i])
			m_ChildStatus[i] = m_Childrens[i]->Tick();

		if (Status::Success == m_ChildStatus[i])
			++iSuccess;
		else if (Status::Failure == m_ChildStatus[i])
			++iFailure;
	}

	_bool bSuccess = (Parallel_Policy::Require_One == m_eSuccess) ? (iSuccess >= 1) : (iSuccess == iCount);
	_bool bFailure = (Parallel_Policy::Require_One == m_eFailure) ? (iFailure >= 1) : (iFailure == iCount);

	if(bSuccess || bFailure)
	{ 
		for (_uint i = 0; i < iCount; i++)
		{
			if (Status::Running == m_ChildStatus[i])
				m_Childrens[i]->Abort();
		}

		m_ChildStatus.clear();

		return m_eStatus = bSuccess ? Status::Success : Status::Failure;
	}

	return m_eStatus = Status::Running;
}

void CBT_Parallel::Reset()
{
	m_ChildStatus.clear();
	__super::Reset();
}

void CBT_Parallel::Abort()
{
	m_ChildStatus.clear();
	__super::Abort();
}

CBT_Parallel* CBT_Parallel::Create(const _wstring& strName, Parallel_Policy eSuccess, Parallel_Policy eFailure)
{
	CBT_Parallel* pInstance = new CBT_Parallel(strName, eSuccess, eFailure);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Parallel");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_Parallel::Free()
{
	__super::Free();
}
