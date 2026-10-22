#include "BT_Node.h"

CBT_Node::CBT_Node(const _wstring& strName)
	: m_strName{ strName }
	, m_eStatus{ Status::Failure }
{
}

CBT_Node::Status CBT_Node::Tick()
{
	if (Status::Running != m_eStatus)
		OnEnter();

	m_eStatus = OnUpdate();

	if (Status::Running != m_eStatus)
		OnExit(m_eStatus);

	return m_eStatus;
}

void CBT_Node::Reset()
{
	m_eStatus = Status::Failure;
}

void CBT_Node::Abort()
{
	if (Status::Running == m_eStatus)
		OnExit(Status::Failure);
	
	Reset();
}

const CBT_Node* CBT_Node::Get_RunningLeaf() const
{
	return (Status::Running == m_eStatus) ? this : nullptr;
}

void CBT_Node::Free()
{
	__super::Free();
}
