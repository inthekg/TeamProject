#include "LegionArmUpgradeViewModel.h"
#include "ItemTable.h"

HRESULT CLegionArmUpgradeViewModel::Initialize(void* pArg)
{
    UNREFERENCED_PARAMETER(pArg);

    m_ArmData[0].bOwned = true;
    m_ArmData[0].bEquipped = true;
    m_ArmData[0].iItemID = ItemID::LEGION_GRAPPLE;
    m_ArmData[0].iUpgradeLevel = LEGION_ARM_DEFAULT_LEVEL;
    m_ArmData[0].strName = L"퍼펫 스트링";
    m_ArmData[0].strIconTexturePath = "UI/Item/Arm/UIT_Item_Arm_GG_01.dds";

    m_ArmData[0].strAbilityNames[0] = L"퍼펫 스트링";
    m_ArmData[0].strAbilityDescriptions[0] = L"와이어를 방출할 수 있는 리전 암.\n적에게 걸어 끌어당기거나 적에게로 이동할 수 있다.";
    m_ArmData[0].strAbilityNames[1] = L"추적";
    m_ArmData[0].strAbilityDescriptions[1] = L"적을 향해 빠르게 이동할 수 있다.";
    m_ArmData[0].strAbilityNames[2] = L"공격 연계";
    m_ArmData[0].strAbilityDescriptions[2] = L"와이어 추적 후 연계 공격을 사용할 수 있다.";

    m_ArmData[0].iUpgradeCosts[2] = 3000;

    m_ArmData[1].bOwned = false;
    m_ArmData[1].bEquipped = false;
    m_ArmData[1].iItemID = ItemID::LEGION_CHARGEGUN;
    m_ArmData[1].iUpgradeLevel = LEGION_ARM_DEFAULT_LEVEL;
    m_ArmData[1].strName = L"팔콘 아이즈";
    m_ArmData[1].strIconTexturePath = "UI/Item/Arm/UIT_Item_Arm_SniperCannon.dds";

    m_ArmData[1].strAbilityNames[0] = L"팔콘 아이즈";
    m_ArmData[1].strAbilityDescriptions[0] = L"원거리에서 폭발하는 탄환을 발사하는 리전 암.";
    m_ArmData[1].strAbilityNames[1] = L"기본 사격";
    m_ArmData[1].strAbilityDescriptions[1] = L"적을 향해 탄환을 발사할 수 있다.";
    m_ArmData[1].strAbilityNames[2] = L"신소재 화약";
    m_ArmData[1].strAbilityDescriptions[2] = L"탄환의 폭발 범위가 증가한다.";

    m_ArmData[1].iUpgradeCosts[2] = 5000;

    return S_OK;
}

void CLegionArmUpgradeViewModel::Open_LegionArmUpgrade()
{
    if (m_bVisible)
        return;

    m_bVisible = true;
    m_iFocusedArmIndex = 0;
    m_iFocusedStageIndex = m_ArmData[0].iUpgradeLevel;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Close_LegionArmUpgrade()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Set_FocusedArmIndex(_uint iArmIndex)
{
    if (iArmIndex >= LEGION_ARM_UPGRADE_ARM_COUNT || !m_ArmData[iArmIndex].bOwned || m_iFocusedArmIndex == iArmIndex)
        return;

    m_iFocusedArmIndex = iArmIndex;
    m_iFocusedStageIndex = m_ArmData[iArmIndex].iUpgradeLevel;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Move_FocusedArm(_int iDirection)
{
    if (iDirection == 0)
        return;

    const _int iStep = iDirection > 0 ? 1 : -1;
    _int iCandidateIndex = static_cast<_int>(m_iFocusedArmIndex);

    for (_uint iCount = 0; iCount < LEGION_ARM_UPGRADE_ARM_COUNT; ++iCount)
    {
        // 현재 후보에서 다음 또는 이전 리전 암으로 한 칸씩 이동한다.
        iCandidateIndex = (iCandidateIndex + iStep + LEGION_ARM_UPGRADE_ARM_COUNT) % LEGION_ARM_UPGRADE_ARM_COUNT;

        if (m_ArmData[iCandidateIndex].bOwned)
        {
            Set_FocusedArmIndex(static_cast<_uint>(iCandidateIndex));
            return;
        }
    }
}

void CLegionArmUpgradeViewModel::Set_FocusedStageIndex(_uint iStageIndex)
{
    if (iStageIndex >= LEGION_ARM_UPGRADE_STAGE_COUNT || m_iFocusedStageIndex == iStageIndex)
        return;

    m_iFocusedStageIndex = iStageIndex;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Set_CurrentErgo(_uint iCurrentErgo)
{
    if (m_iCurrentErgo == iCurrentErgo)
        return;

    m_iCurrentErgo = iCurrentErgo;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Set_ArmOwned(_uint iArmIndex, _bool bOwned)
{
    if (iArmIndex >= LEGION_ARM_UPGRADE_ARM_COUNT || m_ArmData[iArmIndex].bOwned == bOwned)
        return;

    m_ArmData[iArmIndex].bOwned = bOwned;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Set_ArmEquipped(_uint iArmIndex, _bool bEquipped)
{
    if (iArmIndex >= LEGION_ARM_UPGRADE_ARM_COUNT)
        return;

    _bool bChanged = false;

    if (bEquipped)
    {
        // 하나의 리전 암만 장착 상태가 되도록 기존 장착 상태를 함께 갱신한다.
        for (_uint iIndex = 0; iIndex < LEGION_ARM_UPGRADE_ARM_COUNT; ++iIndex)
        {
            const _bool bNewEquipped = iIndex == iArmIndex;

            if (m_ArmData[iIndex].bEquipped != bNewEquipped)
            {
                m_ArmData[iIndex].bEquipped = bNewEquipped;
                bChanged = true;
            }
        }
    }
    else if (m_ArmData[iArmIndex].bEquipped)
    {
        m_ArmData[iArmIndex].bEquipped = false;
        bChanged = true;
    }

    if (bChanged)
        Make_Dirty();
}

void CLegionArmUpgradeViewModel::Set_UpgradeLevel(_uint iArmIndex, _uint iUpgradeLevel)
{
    if (iArmIndex >= LEGION_ARM_UPGRADE_ARM_COUNT)
        return;

    const _uint iClampedLevel = min(iUpgradeLevel, LEGION_ARM_UPGRADE_STAGE_COUNT - 1);

    if (m_ArmData[iArmIndex].iUpgradeLevel == iClampedLevel)
        return;

    m_ArmData[iArmIndex].iUpgradeLevel = iClampedLevel;
    Make_Dirty();
}

void CLegionArmUpgradeViewModel::Apply_UpgradeResult(_uint iNewUpgradeLevel, _uint iRemainingErgo)
{
    if (m_iFocusedArmIndex >= LEGION_ARM_UPGRADE_ARM_COUNT)
        return;

    // 게임 로직에서 강화 성공이 확정된 결과를 ViewModel에 반영한다.
    m_ArmData[m_iFocusedArmIndex].iUpgradeLevel = min(iNewUpgradeLevel, LEGION_ARM_UPGRADE_STAGE_COUNT - 1);
    m_iCurrentErgo = iRemainingErgo;
    m_iFocusedStageIndex = m_ArmData[m_iFocusedArmIndex].iUpgradeLevel;
    Make_Dirty();
}

const LEGION_ARM_UPGRADE_UI_DATA& CLegionArmUpgradeViewModel::Get_ArmData(_uint iArmIndex) const
{
    if (iArmIndex >= LEGION_ARM_UPGRADE_ARM_COUNT)
        return m_ArmData[0];

    return m_ArmData[iArmIndex];
}

const LEGION_ARM_UPGRADE_UI_DATA& CLegionArmUpgradeViewModel::Get_FocusedArmData() const
{
    return Get_ArmData(m_iFocusedArmIndex);
}

_bool CLegionArmUpgradeViewModel::Is_StageAcquired(_uint iStageIndex) const
{
    if (iStageIndex >= LEGION_ARM_UPGRADE_STAGE_COUNT)
        return false;

    return iStageIndex <= Get_FocusedArmData().iUpgradeLevel;
}

_bool CLegionArmUpgradeViewModel::Is_StageAvailable(_uint iStageIndex) const
{
    const LEGION_ARM_UPGRADE_UI_DATA& ArmData = Get_FocusedArmData();

    return ArmData.bOwned && ArmData.iUpgradeLevel == LEGION_ARM_DEFAULT_LEVEL && iStageIndex == LEGION_ARM_UPGRADED_LEVEL;
}

_bool CLegionArmUpgradeViewModel::Is_StageLocked(_uint iStageIndex) const
{
    if (iStageIndex >= LEGION_ARM_UPGRADE_STAGE_COUNT)
        return true;

    // 획득 단계와 바로 다음 강화 가능 단계를 제외한 이후 단계만 잠금이다.
    return iStageIndex > Get_FocusedArmData().iUpgradeLevel + 1;
}

_bool CLegionArmUpgradeViewModel::Is_AllUpgraded() const
{
    return Get_FocusedArmData().iUpgradeLevel >= LEGION_ARM_UPGRADED_LEVEL;
}

_bool CLegionArmUpgradeViewModel::Can_UpgradeFocusedStage() const
{
    if (!Is_StageAvailable(m_iFocusedStageIndex))
        return false;

    return m_iCurrentErgo >= Get_FocusedUpgradeCost();
}

_uint CLegionArmUpgradeViewModel::Get_FocusedUpgradeCost() const
{
    if (m_iFocusedStageIndex != LEGION_ARM_UPGRADED_LEVEL)
        return 0;

    return Get_FocusedArmData().iUpgradeCosts[LEGION_ARM_UPGRADED_LEVEL];
}

CLegionArmUpgradeViewModel* CLegionArmUpgradeViewModel::Create()
{
    CLegionArmUpgradeViewModel* pInstance = new CLegionArmUpgradeViewModel;

    if (FAILED(pInstance->Initialize(nullptr)))
    {
        MSG_BOX("Failed to Created : CLegionArmUpgradeViewModel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLegionArmUpgradeViewModel::Free()
{
    __super::Free();
}