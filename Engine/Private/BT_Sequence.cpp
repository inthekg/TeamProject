#include "BT_Sequence.h"


CBT_Sequence::CBT_Sequence(const _wstring& strName)
	: CBT_Composite{ strName }
{
}

CBT_Node::Status CBT_Sequence::Tick()
{
	while (m_iCurrentIndex < ETOUI(m_Childrens.size()))
	{
		Status eChildStatus = m_Childrens[m_iCurrentIndex]->Tick();

		if (Status::Failure == eChildStatus)
		{
			m_iCurrentIndex = 0;
			return m_eStatus = Status::Failure;
		}
		else if (Status::Running == eChildStatus)
		{
			return m_eStatus = Status::Running;
		}
		++m_iCurrentIndex;
	}

	m_iCurrentIndex = 0;
	return m_eStatus = Status::Success;
}

CBT_Sequence* CBT_Sequence::Create(const _wstring& strName)
{
	CBT_Sequence* pInstance = new CBT_Sequence(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Sequence");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_Sequence::Free()
{
	__super::Free();
}
