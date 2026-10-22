#include "BT_ReactiveSequence.h"

CBT_ReactiveSequence::CBT_ReactiveSequence(const _wstring& strName)
	: CBT_Composite{ strName }
{
}

CBT_Node::Status CBT_ReactiveSequence::Tick()
{
	const _uint iCount = ETOUI(m_Childrens.size());

	for (_uint i = 0; i < iCount; ++i)
	{
		Status eResult = m_Childrens[i]->Tick();

		if (Status::Failure == eResult)
		{
			for (_uint j = 0; j < iCount; j++)
			{
				if (Status::Running == m_Childrens[j]->Get_Status())
					m_Childrens[j]->Abort();
			}
			return m_eStatus = Status::Failure;
		}

		if (Status::Running == eResult)
			return m_eStatus = Status::Running;
	}

	return m_eStatus = Status::Success;
}

CBT_ReactiveSequence* CBT_ReactiveSequence::Create(const _wstring& strName)
{
	CBT_ReactiveSequence* pInstance = new CBT_ReactiveSequence(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_ReactiveSequence");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_ReactiveSequence::Free()
{
	__super::Free();
}
