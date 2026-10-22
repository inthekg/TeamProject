#include "BT_Cooldown.h"
#include "BT_BlackBoard.h"

CBT_Cooldown::CBT_Cooldown(const _wstring& strName, CBT_Node* pChild, _float fSeconds)
    : CBT_Decorator{ strName, pChild }
    , m_fCooldown{ fSeconds }
{
}

CBT_Node::Status CBT_Cooldown::Tick()
{
    if (nullptr == m_pChild)
        return m_eStatus = Status::Failure;

    _float fTimeDelta = (nullptr != m_pBlackBoard) ? m_pBlackBoard->Get<_float>(L"fTimeDelta") : 0.f;

    if (m_fRemain > 0.f)
        m_fRemain = max(0.f, m_fRemain - fTimeDelta);

    Status eResult = m_pChild->Tick();

    if (Status::Success == eResult)
        m_fRemain = m_fCooldown;
    return m_eStatus = eResult;
}

CBT_Cooldown* CBT_Cooldown::Create(const _wstring& strName, CBT_Node* pChild, _float fSeconds)
{
    CBT_Cooldown* pInstance = new CBT_Cooldown(strName, pChild, fSeconds);

    if (nullptr == pInstance)
    {
        MSG_BOX("Failed to Created : CBT_Cooldown");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBT_Cooldown::Free()
{
    __super::Free();
}
