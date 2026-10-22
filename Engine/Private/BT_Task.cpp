#include "BT_Task.h"

CBT_Task::CBT_Task(const _wstring& strName)
	: CBT_Node{ strName }	
{
}

CBT_Node::Status CBT_Task::OnUpdate()
{
	if (!m_func)
		return Status::Failure;

	return m_func(m_pBlackBoard);
}

CBT_Task* CBT_Task::Create(const _wstring& strName, TaskFunc func)
{	
	CBT_Task* pInstance = new CBT_Task(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Task");
		Safe_Release(pInstance);
	}

	pInstance->m_func = func;

	return pInstance;
}

void CBT_Task::Free()
{
	m_func = nullptr;
	__super::Free();
}
