#include "UIHUDViewModelBase.h"


HRESULT CUIHUDViewModelBase::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

void CUIHUDViewModelBase::Set_Visible(_bool bVisible)
{
    if (m_bVisible == bVisible)
        return;

    m_bVisible = bVisible;
    Make_Dirty();
}

void CUIHUDViewModelBase::Set_Position(const _float2& vPosition)
{
    if (Is_SamePosition(m_vPosition, vPosition))
        return;

    m_vPosition = vPosition;
    Make_Dirty();
}

void CUIHUDViewModelBase::Set_HPRatio(_float fRatio)
{
    const _float fClampedRatio = Clamp01(fRatio);

    if (m_fHPRatio == fClampedRatio)
        return;

    m_fHPRatio = fClampedRatio;
    Make_Dirty();
}

void CUIHUDViewModelBase::Set_GroggyVisible(_bool bVisible)
{
    if (m_bGroggyVisible == bVisible)
        return;

    m_bGroggyVisible = bVisible;
    Make_Dirty();
}

void CUIHUDViewModelBase::Set_GroggyPosition(const _float2& vPosition)
{
    if (Is_SamePosition(m_vGroggyPosition, vPosition))
        return;

    m_vGroggyPosition = vPosition;
    Make_Dirty();
}

void CUIHUDViewModelBase::Set_FatalVisible(_bool bVisible)
{
    if (m_bFatalVisible == bVisible)
        return;

    m_bFatalVisible = bVisible;
    Make_Dirty();
}

void CUIHUDViewModelBase::Set_FatalPosition(const _float2& vPosition)
{
    if (Is_SamePosition(m_vFatalPosition, vPosition))
        return;

    m_vFatalPosition = vPosition;
    Make_Dirty();
}

void CUIHUDViewModelBase::Free()
{
    __super::Free();
}
