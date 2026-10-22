#include "BT_ForceFailure.h"

CBT_ForceFailure::CBT_ForceFailure(const _wstring& strName, CBT_Node* pChild)
	: CBT_Decorator{ strName, pChild }
{
}

CBT_Node::Status CBT_ForceFailure::Tick()
{
	if (nullptr == m_pChild)
		return m_eStatus = Status::Failure;

	Status eResult = m_pChild->Tick();

	if (Status::Running == eResult)
		return m_eStatus = Status::Running;

	return m_eStatus = Status::Failure;
}

CBT_ForceFailure* CBT_ForceFailure::Create(const _wstring& strName, CBT_Node* pChild)
{
	CBT_ForceFailure* pInstance = new CBT_ForceFailure(strName, pChild);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_ForceFailure");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_ForceFailure::Free()
{
	__super::Free();
}
