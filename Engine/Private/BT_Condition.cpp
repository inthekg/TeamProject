#include "BT_Condition.h"

CBT_Condition::CBT_Condition(const _wstring& strName)
	: CBT_Node{ strName }	
{
}

CBT_Node::Status CBT_Condition::OnUpdate()
{
	if (!m_func)
		return Status::Failure;

	return m_func(m_pBlackBoard) ? Status::Success : Status::Failure;
}

CBT_Condition* CBT_Condition::Create(const _wstring& strName, ConditionFunc func)
{
	CBT_Condition* pInstance = new CBT_Condition(strName);

	if (nullptr == pInstance || !func)
	{
		MSG_BOX("Failed to Created : CBT_Condition");
		Safe_Release(pInstance);
		return nullptr;
	}

	pInstance->m_func = func;

	return pInstance;
}

void CBT_Condition::Free()
{
	m_func = nullptr;
	__super::Free();
}
