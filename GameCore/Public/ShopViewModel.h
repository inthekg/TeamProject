#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

constexpr _uint SHOP_SLOT_COUNT = 10;

enum class SHOP_CATEGORY
{
    MASK,
    ACCESSORY,
    COSTUME,
    END
};

struct SHOP_SLOT_DATA
{
    _uint64 iItemID = { 0 };                 // ItemTable에서 사용하는 실제 상품 ID
    _uint iPrice = { 0 };                    // 상품 가격
    _bool bVisible = { false };              // 현재 카테고리에서 슬롯을 표시할지 여부
    _bool bSoldOut = { false };              // 이미 보유하여 품절된 상품인지 여부

    _wstring strName;                        // 오른쪽 상세 패널 제목
    _wstring strCategory;                    // 상품 분류
    _wstring strDescription;                 // 상품 설명
    string strIconTexturePath;               // 슬롯 및 상세 패널 아이콘 경로
};

class CShopViewModel final : public CUIViewModelBase
{
private:
    CShopViewModel() = default;
    virtual ~CShopViewModel() = default;

public:
    void                    Open_Shop();
    void                    Close_Shop();

public:
    void                    Set_Category(SHOP_CATEGORY eCategory);
    void                    Move_Category(_int iDirection);

public:
    void                    Clear_Slots();
    void                    Set_SlotData(_uint iSlotIndex, const SHOP_SLOT_DATA& SlotData);
    void                    Select_Slot(_int iSlotIndex);
    void                    Clear_SelectedSlot();

    void                    Set_CurrentErgo(_uint iCurrentErgo); // 플레이어 인벤토리에서 읽은 현재 에르고를 저장한다.
    _uint                   Get_CurrentErgo() const { return m_iCurrentErgo; }

public:
    _bool                   Is_Visible() const { return m_bVisible; }
    SHOP_CATEGORY           Get_Category() const { return m_eCategory; }
    _int                    Get_SelectedSlotIndex() const { return m_iSelectedSlotIndex; }

    const SHOP_SLOT_DATA&   Get_SlotData(_uint iSlotIndex) const;
    const SHOP_SLOT_DATA*   Get_SelectedSlotData() const;

private:
    _bool                   m_bVisible = { false };
    SHOP_CATEGORY           m_eCategory = { SHOP_CATEGORY::MASK };

    _int                    m_iSelectedSlotIndex = { -1 };
    SHOP_SLOT_DATA          m_Slots[SHOP_SLOT_COUNT] = {};

    _uint                   m_iCurrentErgo = { 0 }; // 상점 에르고 프로필에 표시할 현재 보유량

public:
    static CShopViewModel* Create();
    virtual void Free() override;
};

NS_END

