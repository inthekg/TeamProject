#include "BT_Selector.h"

CBT_Selector::CBT_Selector(const _wstring& strName)
	: CBT_Composite{ strName }
{
}

CBT_Node::Status CBT_Selector::Tick()
{
	while (m_iCurrentIndex < ETOUI(m_Childrens.size()))
	{
		Status eChildStaus = m_Childrens[m_iCurrentIndex]->Tick();

		if (Status::Success == eChildStaus)
		{
			// 하나라도 성공하면 전체 성공, 인덱스 리셋
			m_iCurrentIndex = 0;
			return m_eStatus = Status::Success;
		}
		else if (Status::Running == eChildStaus)
		{
			return m_eStatus = Status::Running;
		}

		// Failure면 다음 자식 시도
		++m_iCurrentIndex;
	}

	m_iCurrentIndex = 0;
	return m_eStatus = Status::Failure;
}

CBT_Selector* CBT_Selector::Create(const _wstring& strName)
{
	CBT_Selector* pInstance = new CBT_Selector(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Selector");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_Selector::Free()
{
	__super::Free();
}
