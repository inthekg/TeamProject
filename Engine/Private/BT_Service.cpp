#include "BT_Service.h"
#include "BT_BlackBoard.h"

CBT_Service::CBT_Service(const _wstring& strName, CBT_Node* pChild, ServiceFunc func, _float fInterval)
    : CBT_Decorator{ strName, pChild }
    , m_func(func)
    , m_fInterval{fInterval}
{
}

CBT_Node::Status CBT_Service::Tick()
{
    if (nullptr == m_pChild)
        return m_eStatus = Status::Failure;

    _bool bEnter = (Status::Running != m_eStatus);
    if (true == bEnter)
    {
        if (nullptr != m_func)
            m_func(m_pBlackBoard);
        m_fAccum = 0.f;
    }

    _float fTimeDelta = (nullptr != m_pBlackBoard) ? m_pBlackBoard->Get<_float>(L"fTimeDelta") : 0.f;
    m_fAccum += fTimeDelta;

    if (false == bEnter && m_fInterval > 0.f && m_fAccum >= m_fInterval)
    {
        if (nullptr != m_func)
            m_func(m_pBlackBoard);
        m_fAccum -= m_fInterval;
    }

    return m_eStatus = m_pChild->Tick();
}

void CBT_Service::Reset()
{
    m_fAccum = 0.f;
    __super::Reset();
}

void CBT_Service::Abort()
{
    m_fAccum = 0.f;
    __super::Abort();
}

CBT_Service* CBT_Service::Create(const _wstring& strName, CBT_Node* pChild, ServiceFunc func, _float fInterval)
{
    CBT_Service* pInstance = new CBT_Service(strName, pChild, func, fInterval);

    if (nullptr == pInstance)
    {
        MSG_BOX("Failed to Created : CBT_Service");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBT_Service::Free()
{
    __super::Free();
}
