#include "LegionArmChangeViewModel.h"
#include "ItemTable.h"

HRESULT CLegionArmChangeViewModel::Initialize(void* pArg)
{
    UNREFERENCED_PARAMETER(pArg);

    // 0번: 프로토타입 리전 암
    m_ArmData[0].bOwned = true;
    m_ArmData[0].bUpgradeable = false;
    m_ArmData[0].iBaseAbilityCount = 1;
    m_ArmData[0].iUpgradeLevel = LEGION_ARM_CHANGE_DEFAULT_LEVEL;
    m_ArmData[0].iItemID = ItemID::LEGION_BASE;
    m_ArmData[0].strName = L"프로토타입 리전 암";
    m_ArmData[0].strIconTexturePath = "UI/Item/Arm/UIT_Item_Arm_1.dds";

    m_ArmData[0].strAbilityNames[0] = L"프로토타입 리전 암";
    m_ArmData[0].strAbilityDescriptions[0] = L"기본형 리전 암.\n별도의 특수 능력이나 강화 단계는 없다.";

    // 1번: 퍼펫 스트링
    m_ArmData[1].bOwned = false;
    m_ArmData[1].bUpgradeable = true;
    m_ArmData[1].iBaseAbilityCount = 2;
    m_ArmData[1].iUpgradeLevel = LEGION_ARM_CHANGE_DEFAULT_LEVEL;
    m_ArmData[1].iItemID = ItemID::LEGION_GRAPPLE;
    m_ArmData[1].strName = L"퍼펫 스트링";
    m_ArmData[1].strIconTexturePath = "UI/Item/Arm/UIT_Item_Arm_GG_01.dds";

    m_ArmData[1].strAbilityNames[0] = L"퍼펫 스트링";
    m_ArmData[1].strAbilityDescriptions[0] = L"와이어를 방출할 수 있는 리전 암.\n적에게 걸어 끌어당기거나 적에게로 이동할 수 있다.";

    m_ArmData[1].strAbilityNames[1] = L"추적";
    m_ArmData[1].strAbilityDescriptions[1] = L"리전 암 버튼을 누르고 있으면 적을 향해 추적할 수 있다.";

    m_ArmData[1].strAbilityNames[2] = L"공격 연계";
    m_ArmData[1].strAbilityDescriptions[2] = L"적을 끌어오거나 와이어 추적 후 리전 암 버튼을 누르고 있으면 연계 공격이 발생한다.";

    // 2번: 팔콘 아이즈
    m_ArmData[2].bOwned = false;
    m_ArmData[2].bUpgradeable = true;
    m_ArmData[2].iBaseAbilityCount = 2;
    m_ArmData[2].iUpgradeLevel = LEGION_ARM_CHANGE_DEFAULT_LEVEL;
    m_ArmData[2].iItemID = ItemID::LEGION_CHARGEGUN;
    m_ArmData[2].strName = L"팔콘 아이즈";
    m_ArmData[2].strIconTexturePath = "UI/Item/Arm/UIT_Item_Arm_SniperCannon.dds";

    m_ArmData[2].strAbilityNames[0] = L"팔콘 아이즈";
    m_ArmData[2].strAbilityDescriptions[0] = L"원거리에 철갑탄을 발사하는 리전 암.\n발사된 탄환은 대상에 꽂힌 후 잠시 뒤 폭발한다.";

    m_ArmData[2].strAbilityNames[1] = L"기본 사격";
    m_ArmData[2].strAbilityDescriptions[1] = L"적을 향해 폭발하는 탄환을 발사할 수 있다.";

    m_ArmData[2].strAbilityNames[2] = L"신소재 화약";
    m_ArmData[2].strAbilityDescriptions[2] = L"탄환의 폭발 범위가 증가한다.";

    return S_OK;
}

void CLegionArmChangeViewModel::Open_LegionArmChange()
{
    if (m_bVisible)
        return;

    m_bVisible = true;
    m_eCurrentPage = LEGION_ARM_CHANGE_PAGE::DETAIL;
    m_iFocusedArmIndex = m_iEquippedArmIndex;
    Make_Dirty();
}

void CLegionArmChangeViewModel::Close_LegionArmChange()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    Make_Dirty();
}

void CLegionArmChangeViewModel::Open_DetailPage()
{
    if (m_eCurrentPage == LEGION_ARM_CHANGE_PAGE::DETAIL)
        return;

    m_eCurrentPage = LEGION_ARM_CHANGE_PAGE::DETAIL;
    m_iFocusedArmIndex = m_iEquippedArmIndex;
    Make_Dirty();
}

void CLegionArmChangeViewModel::Open_SelectPage()
{
    if (m_eCurrentPage == LEGION_ARM_CHANGE_PAGE::SELECT)
        return;

    m_eCurrentPage = LEGION_ARM_CHANGE_PAGE::SELECT;
    m_iFocusedArmIndex = m_iEquippedArmIndex;
    Make_Dirty();
}

void CLegionArmChangeViewModel::Set_FocusedArmIndex(_uint iArmIndex)
{
    if (iArmIndex >= LEGION_ARM_CHANGE_ARM_COUNT)
        return;

    if (!m_ArmData[iArmIndex].bOwned)
        return;

    if (m_iFocusedArmIndex == iArmIndex)
        return;

    m_iFocusedArmIndex = iArmIndex;
    Make_Dirty();
}

void CLegionArmChangeViewModel::Set_ArmOwned(_uint iArmIndex, _bool bOwned)
{
    if (iArmIndex >= LEGION_ARM_CHANGE_ARM_COUNT)
        return;

    if (m_ArmData[iArmIndex].bOwned == bOwned)
        return;

    m_ArmData[iArmIndex].bOwned = bOwned;

    // 현재 선택 또는 장착 중인 리전 암이 미보유 상태가 되면 첫 번째 리전 암으로 복구한다.
    if (!bOwned && m_iFocusedArmIndex == iArmIndex)
        m_iFocusedArmIndex = 0;

    if (!bOwned && m_iEquippedArmIndex == iArmIndex)
        m_iEquippedArmIndex = 0;

    Make_Dirty();
}

void CLegionArmChangeViewModel::Set_EquippedArmIndex(_uint iArmIndex)
{
    if (iArmIndex >= LEGION_ARM_CHANGE_ARM_COUNT)
        return;

    if (!m_ArmData[iArmIndex].bOwned)
        return;

    if (m_iEquippedArmIndex == iArmIndex && m_iFocusedArmIndex == iArmIndex)
        return;

    m_iEquippedArmIndex = iArmIndex;
    m_iFocusedArmIndex = iArmIndex;
    Make_Dirty();
}

void CLegionArmChangeViewModel::Set_UpgradeLevel(_uint iArmIndex, _uint iUpgradeLevel)
{
    if (iArmIndex >= LEGION_ARM_CHANGE_ARM_COUNT)
        return;

    const _uint iClampedLevel = m_ArmData[iArmIndex].bUpgradeable ? max(LEGION_ARM_CHANGE_DEFAULT_LEVEL, min(iUpgradeLevel, LEGION_ARM_CHANGE_UPGRADED_LEVEL)) : LEGION_ARM_CHANGE_DEFAULT_LEVEL;

    if (m_ArmData[iArmIndex].iUpgradeLevel == iClampedLevel)
        return;

    m_ArmData[iArmIndex].iUpgradeLevel = iClampedLevel;
    Make_Dirty();
}

const LEGION_ARM_UI_DATA& CLegionArmChangeViewModel::Get_ArmData(_uint iArmIndex) const
{
    if (iArmIndex >= LEGION_ARM_CHANGE_ARM_COUNT)
        return m_ArmData[0];

    return m_ArmData[iArmIndex];
}

const LEGION_ARM_UI_DATA& CLegionArmChangeViewModel::Get_DisplayedArmData() const
{
    const _uint iDisplayedArmIndex = Is_DetailPage() ? m_iEquippedArmIndex : m_iFocusedArmIndex;
    return Get_ArmData(iDisplayedArmIndex);
}

_bool CLegionArmChangeViewModel::Is_AbilityVisible(_uint iAbilityIndex) const
{
    if (iAbilityIndex >= LEGION_ARM_CHANGE_ABILITY_COUNT)
        return false;

    const LEGION_ARM_UI_DATA& ArmData = Get_ArmData(m_iEquippedArmIndex);

    // 리전 암마다 지정된 기본 능력 개수까지는 항상 표시한다.
    if (iAbilityIndex < ArmData.iBaseAbilityCount)
        return true;

    // 프로토타입처럼 강화할 수 없는 리전 암은 추가 능력을 표시하지 않는다.
    if (!ArmData.bUpgradeable)
        return false;

    // 기본 능력 다음 항목 하나가 실제 강화 능력이다.
    return iAbilityIndex == ArmData.iBaseAbilityCount && ArmData.iUpgradeLevel >= LEGION_ARM_CHANGE_UPGRADED_LEVEL;
}

CLegionArmChangeViewModel* CLegionArmChangeViewModel::Create()
{
    CLegionArmChangeViewModel* pInstance = new CLegionArmChangeViewModel;

    if (FAILED(pInstance->Initialize(nullptr)))
    {
        MSG_BOX("Failed to Created : CLegionArmChangeViewModel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLegionArmChangeViewModel::Free()
{
    __super::Free();
}