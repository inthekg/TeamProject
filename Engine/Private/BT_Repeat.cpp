#include "BT_Repeat.h"

CBT_Repeat::CBT_Repeat(const _wstring& strName, CBT_Node* pChild, _int iRepeatCount)
	: CBT_Decorator{ strName, pChild }
	, m_iRepeatCount{ iRepeatCount }	
{
}

CBT_Node::Status CBT_Repeat::Tick()
{
	if (nullptr == m_pChild)
		return m_eStatus = Status::Failure;

	Status eResult = m_pChild->Tick();

	if (Status::Running == eResult)
		return m_eStatus = Status::Running;

	if (Status::Failure == eResult)
		return m_eStatus = Status::Failure;

	++m_iCurrentCount;
	m_pChild->Reset();

	if (-1 == m_iRepeatCount)
		return m_eStatus = Status::Running;

	if (m_iCurrentCount >= m_iRepeatCount)
	{
		m_iCurrentCount = 0;
		return m_eStatus = Status::Success;
	}

	return m_eStatus = Status::Running;
}

void CBT_Repeat::Reset()
{
	m_iCurrentCount = 0;
	__super::Reset();
}

void CBT_Repeat::Abort()
{
	m_iCurrentCount = 0;
	__super::Abort();
}

CBT_Repeat* CBT_Repeat::Create(const _wstring& strName, CBT_Node* pChild, _int iRepeatCount)
{
	CBT_Repeat* pInstance = new CBT_Repeat(strName, pChild, iRepeatCount);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Repeat");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_Repeat::Free()
{
	__super::Free();
}
