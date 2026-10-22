#include "BT_Wait.h"
#include "BT_BlackBoard.h"

CBT_Wait::CBT_Wait(const _wstring& strName)
    : CBT_Node{ strName }
{
}

void CBT_Wait::OnEnter()
{
    m_fElapsed = 0.f;
}

CBT_Node::Status CBT_Wait::OnUpdate()
{
    _float dt = m_pBlackBoard ? m_pBlackBoard->Get<_float>(L"fTimeDelta") : 0.f;

    m_fElapsed += dt;

    return (m_fElapsed >= m_fDuration) ? Status::Success : Status::Running;
}

CBT_Wait* CBT_Wait::Create(const _wstring strName, _float fSeconds)
{
	CBT_Wait* pInstance = new CBT_Wait(strName);

	if (nullptr == pInstance)
	{
		MSG_BOX("Failed to Created : CBT_Wait");
		Safe_Release(pInstance);
		return nullptr;
	}

	pInstance->m_fDuration = fSeconds;

	return pInstance;
}

void CBT_Wait::Free()
{
	__super::Free();
}
