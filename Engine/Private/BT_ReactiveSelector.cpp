#include "BT_ReactiveSelector.h"

CBT_ReactiveSelector::CBT_ReactiveSelector(const _wstring& strName)
	: CBT_Composite{ strName }
{
}

CBT_Node::Status CBT_ReactiveSelector::Tick()
{
	const _int iCount = ETOI(m_Childrens.size());

	for (_int i = 0; i < iCount; i++)
	{
		Status eResult = m_Childrens[i]->Tick();

		if (Status::Success == eResult)
		{
			if (m_iRunningIndex >= 0 && m_iRunningIndex != i)
				m_Childrens[m_iRunningIndex]->Abort();

			m_iRunningIndex = -1;
			return m_eStatus = Status::Success;
		}

		if (Status::Running == eResult)
		{
			if (m_iRunningIndex >= 0 && m_iRunningIndex != i)
				m_Childrens[m_iRunningIndex]->Abort();

			m_iRunningIndex = i;
			return m_eStatus = Status::Running;
		}
	}

	m_iRunningIndex = -1;
	return m_eStatus = Status::Failure;	
}

void CBT_ReactiveSelector::Reset()
{
	m_iRunningIndex = -1;
	__super::Reset();
}

void CBT_ReactiveSelector::Abort()
{
	m_iRunningIndex = -1;
	__super::Abort();
}

CBT_ReactiveSelector* CBT_ReactiveSelector::Create(const _wstring& strName)
{
	CBT_ReactiveSelector* pInstance = new CBT_ReactiveSelector(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_ReactiveSelector");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_ReactiveSelector::Free()
{
	__super::Free();
}
