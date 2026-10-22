#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

constexpr _uint LEGION_ARM_CHANGE_ARM_COUNT = 3;
constexpr _uint LEGION_ARM_CHANGE_ABILITY_COUNT = 3;
constexpr _uint LEGION_ARM_CHANGE_ABILITY_UI_COUNT = 4;
constexpr _uint LEGION_ARM_CHANGE_DEFAULT_LEVEL = 1;
constexpr _uint LEGION_ARM_CHANGE_UPGRADED_LEVEL = 2;

enum class LEGION_ARM_CHANGE_PAGE
{
    DETAIL, // 현재 장착한 리전 암과 능력을 표시한다.
    SELECT  // 보유 중인 리전 암을 선택한다.
};

struct LEGION_ARM_UI_DATA
{
    _bool bOwned = { false };                                      // 해당 리전 암 보유 여부
    _bool bUpgradeable = { true };                                 // 강화 가능한 리전 암인지 여부
    _uint iBaseAbilityCount = { 2 };                               // 강화 없이 표시할 기본 능력 개수
    _uint iUpgradeLevel = { LEGION_ARM_CHANGE_DEFAULT_LEVEL };     // 1: 기본, 2: 강화 완료
    _uint64 iItemID = { 0 };                                       // 인벤토리 연결용 아이템 ID
    _wstring strName;                                              // 리전 암 이름
    string strIconTexturePath;                                     // UI 아이콘 경로
    _wstring strAbilityNames[LEGION_ARM_CHANGE_ABILITY_COUNT];
    _wstring strAbilityDescriptions[LEGION_ARM_CHANGE_ABILITY_COUNT];
};

class CLegionArmChangeViewModel final : public CUIViewModelBase
{
private:
    CLegionArmChangeViewModel() = default;
    virtual ~CLegionArmChangeViewModel() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    void Open_LegionArmChange();
    void Close_LegionArmChange();
    void Open_DetailPage();
    void Open_SelectPage();

    void Set_FocusedArmIndex(_uint iArmIndex);
    void Set_ArmOwned(_uint iArmIndex, _bool bOwned);
    void Set_EquippedArmIndex(_uint iArmIndex);
    void Set_UpgradeLevel(_uint iArmIndex, _uint iUpgradeLevel);

public:
    _bool Is_Visible() const { return m_bVisible; }
    _bool Is_DetailPage() const { return m_eCurrentPage == LEGION_ARM_CHANGE_PAGE::DETAIL; }
    _bool Is_SelectPage() const { return m_eCurrentPage == LEGION_ARM_CHANGE_PAGE::SELECT; }

    _uint Get_EquippedArmIndex() const { return m_iEquippedArmIndex; }
    _uint Get_FocusedArmIndex() const { return m_iFocusedArmIndex; }

    const LEGION_ARM_UI_DATA& Get_ArmData(_uint iArmIndex) const;
    const LEGION_ARM_UI_DATA& Get_DisplayedArmData() const;
    _bool Is_AbilityVisible(_uint iAbilityIndex) const;

private:
    _bool m_bVisible = { false };
    LEGION_ARM_CHANGE_PAGE m_eCurrentPage = { LEGION_ARM_CHANGE_PAGE::DETAIL };

    LEGION_ARM_UI_DATA m_ArmData[LEGION_ARM_CHANGE_ARM_COUNT] = {};

    _uint m_iEquippedArmIndex = { 0 };
    _uint m_iFocusedArmIndex = { 0 };

public:
    static CLegionArmChangeViewModel* Create();
    virtual void Free() override;
};

NS_END
