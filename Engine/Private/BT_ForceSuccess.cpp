#include "BT_ForceSuccess.h"

CBT_ForceSuccess::CBT_ForceSuccess(const _wstring& strName, CBT_Node* pChild)
	: CBT_Decorator{ strName, pChild }
{
}

CBT_Node::Status CBT_ForceSuccess::Tick()
{
	if (nullptr == m_pChild)
		return m_eStatus = Status::Success;

	Status eResult = m_pChild->Tick();

	if (Status::Running == eResult)
		return m_eStatus = Status::Running;

	return m_eStatus = Status::Success;
}

CBT_ForceSuccess* CBT_ForceSuccess::Create(const _wstring& strName, CBT_Node* pChild)
{
	CBT_ForceSuccess* pInstance = new CBT_ForceSuccess(strName, pChild);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_ForceSuccess");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBT_ForceSuccess::Free()
{
	__super::Free();
}
