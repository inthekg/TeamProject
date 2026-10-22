#include "BT_Decorator.h"

CBT_Decorator::CBT_Decorator(const _wstring& strName, CBT_Node* pChild)
	: CBT_Node{ strName }
	, m_pChild{ pChild }
{
}

void CBT_Decorator::Set_Child(CBT_Node* pChild)
{
	if (nullptr != m_pChild)
		Safe_Release(m_pChild);

	m_pChild = pChild;

	if (nullptr != m_pChild && nullptr != m_pBlackBoard)
		m_pChild->Set_BlackBoard(m_pBlackBoard);
}

void CBT_Decorator::Set_BlackBoard(CBT_BlackBoard* pBlackBoard)
{
	if (nullptr != m_pChild)
		m_pChild->Set_BlackBoard(pBlackBoard);

	__super::Set_BlackBoard(pBlackBoard);
}

void CBT_Decorator::Reset()
{
	if (nullptr != m_pChild)
		m_pChild->Reset();

	__super::Reset();
}

void CBT_Decorator::Abort()
{
	if (nullptr != m_pChild)
		m_pChild->Abort();
	__super::Reset();
}

const CBT_Node* CBT_Decorator::Get_RunningLeaf() const
{
	if (nullptr != m_pChild && Status::Running == m_pChild->Get_Status())
		return m_pChild->Get_RunningLeaf();

	return nullptr;
}

void CBT_Decorator::Free()
{
	Safe_Release(m_pChild);
	__super::Free();
}

