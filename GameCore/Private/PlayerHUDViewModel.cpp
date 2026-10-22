#include "PlayerHUDViewModel.h"

void CPlayerHUDViewModel::Set_HPDelayRatio(_float fRatio)
{
    const _float fClampedRatio = Clamp01(fRatio);

    if (m_fHPDelayRatio == fClampedRatio)
        return;

    m_fHPDelayRatio = fClampedRatio;
    Make_Dirty();
}

void CPlayerHUDViewModel::Set_StaminaRatio(_float fRatio)
{
    const _float fClampedRatio = Clamp01(fRatio);

    if (m_fStaminaRatio == fClampedRatio)
        return;

    m_fStaminaRatio = fClampedRatio;
    Make_Dirty();
}

void CPlayerHUDViewModel::Set_FableValue(_float fValue)
{
    const _float fClampedValue = min(3.f, max(0.f, fValue));

    if (m_fFableValue == fClampedValue)
        return;

    m_fFableValue = fClampedValue;

    for (_uint i = 0; i < 3; i++)
    {
        m_fFableSlotRatios[i] = min(1.f, max(0.f, m_fFableValue - static_cast<_float>(i)));
    }

    Make_Dirty();
}

void CPlayerHUDViewModel::Set_DurabilityRatio(_float fRatio)
{
    const _float fClampedRatio = Clamp01(fRatio);

    if (m_fDurabilityRatio == fClampedRatio)
        return;

    m_fDurabilityRatio = fClampedRatio;
    Make_Dirty();
}

void CPlayerHUDViewModel::Set_HPFXVisible(_bool bVisible)
{
    if (m_bHPFXVisible == bVisible)
        return;

    m_bHPFXVisible = bVisible;
    Make_Dirty();
}

void CPlayerHUDViewModel::Set_CurrentErgo(_uint iCurrentErgo)
{
    if (m_iCurrentErgo == iCurrentErgo)
        return;

    m_iCurrentErgo = iCurrentErgo;
    Make_Dirty();
}

void CPlayerHUDViewModel::Set_ActiveWeaponSlot(_int iSlotIndex)
{
    if (m_iActiveWeaponSlot == iSlotIndex)
        return;

    m_iActiveWeaponSlot = iSlotIndex;
    Make_Dirty();
}

void CPlayerHUDViewModel::Set_LegionArmType(LegionArmType eType)
{
    if (m_eLegionArmType == eType)
        return;

    m_eLegionArmType = eType;
    Make_Dirty();
}

void CPlayerHUDViewModel::Show_ErgoGain(_uint iGain)
{
    if (iGain == 0)
        return;

    m_iErgoGain += iGain;       // 표시 중 추가 획득하면 기존 획득량에 합산한다.
    m_fErgoGainRemain = 2.f;    // 마지막 획득 시점부터 2초 동안 표시한다.
    Make_Dirty();
}

void CPlayerHUDViewModel::Update_ErgoGain(_float fTimeDelta)
{
    if (m_fErgoGainRemain <= 0.f)
        return;

    m_fErgoGainRemain = max(0.f, m_fErgoGainRemain - fTimeDelta);

    // 남은 시간이 끝나는 순간에만 UI를 다시 적용한다.
    if (m_fErgoGainRemain == 0.f)
    {
        m_iErgoGain = 0;
        Make_Dirty();
    }
}

void CPlayerHUDViewModel::Set_SelectedItemSlot(ITEM_SLOT eItemSlot)
{
    if (m_eSelectedItemSlot == eItemSlot)
        return;

    m_eSelectedItemSlot = eItemSlot;
    Make_Dirty(); // 선택 슬롯이 변경됐으므로 HUD를 다시 적용한다.
}

void CPlayerHUDViewModel::Set_HealCount(_uint iHealCount)
{
    if (m_iHealCount == iHealCount)
        return;

    m_iHealCount = iHealCount;
    Make_Dirty();
}

_float CPlayerHUDViewModel::Get_FableSlotRatio(_uint iIndex) const
{
    if (iIndex >= 3)
        return 0.f;

    return m_fFableSlotRatios[iIndex];
}


CPlayerHUDViewModel* CPlayerHUDViewModel::Create()
{
    return new CPlayerHUDViewModel;
}

void CPlayerHUDViewModel::Free()
{
    __super::Free();
}
