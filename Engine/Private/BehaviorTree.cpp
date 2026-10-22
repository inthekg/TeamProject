#include "BehaviorTree.h"

CBehaviorTree::CBehaviorTree()
{
}

void CBehaviorTree::Set_Root(CBT_Node* pRoot)
{
	if (nullptr != m_pRoot)
		Safe_Release(m_pRoot);

	m_pRoot = pRoot;

	if (nullptr != m_pRoot)
		m_pRoot->Set_BlackBoard(m_pBlackBoard);
}

void CBehaviorTree::Set_BlackBoard(CBT_BlackBoard* pBlackBoard)
{
	if (m_bOwnsBlackBoard && nullptr != m_pBlackBoard && m_pBlackBoard != pBlackBoard)
		Safe_Release(m_pBlackBoard);

	m_pBlackBoard = pBlackBoard;
	m_bOwnsBlackBoard = false;

	if (nullptr != m_pRoot)
		m_pRoot->Set_BlackBoard(m_pBlackBoard);
}

void CBehaviorTree::Update()
{
	if (nullptr == m_pRoot)
		return;

	m_eLastStatus = m_pRoot->Tick();
	
	if (m_eLastStatus != CBT_Node::Status::Running && m_bAutoReset)
		m_pRoot->Reset();
}

void CBehaviorTree::Reset()
{
	if (nullptr != m_pRoot)
		m_pRoot->Reset();

	m_eLastStatus = CBT_Node::Status::Failure;
}

const _wstring CBehaviorTree::Get_RunningLeafName() const
{
	if (nullptr == m_pRoot)
		return L"";

	const CBT_Node* pLeaf = m_pRoot->Get_RunningLeaf();
	return (nullptr != pLeaf) ? pLeaf->Get_Name() : L"";
}

CBehaviorTree* CBehaviorTree::Create(CBT_BlackBoard* pShared)
{
	CBehaviorTree* pInstance = new CBehaviorTree();
	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBehaviorTree");
		Safe_Release(pInstance);
		return nullptr;
	}

	if (nullptr != pShared)
	{
		pInstance->m_pBlackBoard = pShared;
		pInstance->m_bOwnsBlackBoard = false;
	}
	else
	{
		pInstance->m_pBlackBoard = CBT_BlackBoard::Create();
		pInstance->m_bOwnsBlackBoard = true;

		if (nullptr == pInstance->m_pBlackBoard)
		{
			MSG_BOX("Failed to Created BlackBoard : CBehaviorTree");
			Safe_Release(pInstance);
			return nullptr;
		}
	}

	return pInstance;
}

void CBehaviorTree::Free()
{
	Safe_Release(m_pRoot);

	if (m_bOwnsBlackBoard)
		Safe_Release(m_pBlackBoard);

	__super::Free();
}