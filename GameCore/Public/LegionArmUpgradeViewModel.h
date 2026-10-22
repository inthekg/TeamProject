#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

constexpr _uint LEGION_ARM_UPGRADE_ARM_COUNT = 2;
constexpr _uint LEGION_ARM_UPGRADE_STAGE_COUNT = 3;
constexpr _uint LEGION_ARM_DEFAULT_LEVEL = 1; 
constexpr _uint LEGION_ARM_UPGRADED_LEVEL = 2; 

struct LEGION_ARM_UPGRADE_UI_DATA
{
    _bool bOwned = { false };                                             // 리전 암 보유 여부
    _bool bEquipped = { false };                                          // 현재 장착 여부
    _uint64 iItemID = { 0 };                                              // 실제 인벤토리 연결용 Item ID
    _uint iUpgradeLevel = { LEGION_ARM_DEFAULT_LEVEL };                   // 1: 기본, 2: 강화 
    _wstring strName;                                                     // 리전 암 이름
    string strIconTexturePath;                                            // 상단 카테고리 아이콘
    _wstring strAbilityNames[LEGION_ARM_UPGRADE_STAGE_COUNT];             // 기본 및 강화 능력 이름
    _wstring strAbilityDescriptions[LEGION_ARM_UPGRADE_STAGE_COUNT];      // 기본 및 강화 능력 설명
    _uint iUpgradeCosts[LEGION_ARM_UPGRADE_STAGE_COUNT] = {};          // 1~2단계 강화 비용
};

class CLegionArmUpgradeViewModel final : public CUIViewModelBase
{
private:
    CLegionArmUpgradeViewModel() = default;
    virtual ~CLegionArmUpgradeViewModel() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    void Open_LegionArmUpgrade();
    void Close_LegionArmUpgrade();

    void Set_FocusedArmIndex(_uint iArmIndex);
    void Move_FocusedArm(_int iDirection);
    void Set_FocusedStageIndex(_uint iStageIndex);

    void Set_CurrentErgo(_uint iCurrentErgo);
    void Set_ArmOwned(_uint iArmIndex, _bool bOwned);
    void Set_ArmEquipped(_uint iArmIndex, _bool bEquipped);
    void Set_UpgradeLevel(_uint iArmIndex, _uint iUpgradeLevel);

    // 실제 게임 로직에서 강화 성공이 확정된 뒤 결과만 반영한다.
    void Apply_UpgradeResult(_uint iNewUpgradeLevel, _uint iRemainingErgo);

public:
    _bool Is_Visible() const { return m_bVisible; }
    _uint Get_FocusedArmIndex() const { return m_iFocusedArmIndex; }
    _uint Get_FocusedStageIndex() const { return m_iFocusedStageIndex; }
    _uint Get_CurrentErgo() const { return m_iCurrentErgo; }

    const LEGION_ARM_UPGRADE_UI_DATA& Get_ArmData(_uint iArmIndex) const;
    const LEGION_ARM_UPGRADE_UI_DATA& Get_FocusedArmData() const;

    _bool Is_StageAcquired(_uint iStageIndex) const;
    _bool Is_StageAvailable(_uint iStageIndex) const;
    _bool Is_StageLocked(_uint iStageIndex) const;
    _bool Is_AllUpgraded() const;
    _bool Can_UpgradeFocusedStage() const;

    _uint Get_FocusedUpgradeCost() const;

private:
    _bool m_bVisible = { false };

    LEGION_ARM_UPGRADE_UI_DATA m_ArmData[LEGION_ARM_UPGRADE_ARM_COUNT] = {};

    _uint m_iFocusedArmIndex = { 0 };
    _uint m_iFocusedStageIndex = { 0 };
    _uint m_iCurrentErgo = { 0 };

public:
    static CLegionArmUpgradeViewModel* Create();
    virtual void Free() override;
};

NS_END