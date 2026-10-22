#include "BT_Composite.h"

CBT_Composite::CBT_Composite(const _wstring& strName)
	: CBT_Node{ strName }
	, m_iCurrentIndex { 0 }
{
}

void CBT_Composite::Add_Child(CBT_Node* pChild)
{
	m_Childrens.push_back(pChild);
}

void CBT_Composite::Set_BlackBoard(CBT_BlackBoard* pBlackBoard)
{
	for (auto* pChild : m_Childrens)
	{
		if (nullptr != pChild)
			pChild->Set_BlackBoard(pBlackBoard);
	}

	__super::Set_BlackBoard(pBlackBoard);
}

void CBT_Composite::Reset()
{
	m_iCurrentIndex = 0;

	for (auto* pChild : m_Childrens)
	{
		if (nullptr != pChild)
			pChild->Reset();
	}		
	__super::Reset();
}

void CBT_Composite::Abort()
{

	for (auto* pChild : m_Childrens)
	{
		if (nullptr != pChild)
			pChild->Abort();
	}

	m_iCurrentIndex = 0;

	__super::Reset();
}

const CBT_Node* CBT_Composite::Get_RunningLeaf() const
{
	for (auto* pChild : m_Childrens)
	{
		if (nullptr != pChild && Status::Running == pChild->Get_Status())
			return pChild->Get_RunningLeaf();
	}
	return nullptr;
}

void CBT_Composite::Free()
{
	for (auto* pChild : m_Childrens)			
		Safe_Release(pChild);
	
	m_Childrens.clear();

	__super::Free();
}
