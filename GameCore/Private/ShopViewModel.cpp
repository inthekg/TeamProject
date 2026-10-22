#include "ShopViewModel.h"

void CShopViewModel::Open_Shop()
{
    if (m_bVisible)
        return;

    m_bVisible = true;
    m_eCategory = SHOP_CATEGORY::MASK;
    m_iSelectedSlotIndex = -1;

    for (SHOP_SLOT_DATA& SlotData : m_Slots)
        SlotData = {};

    Make_Dirty();
}

void CShopViewModel::Close_Shop()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    m_iSelectedSlotIndex = -1;

    for (SHOP_SLOT_DATA& SlotData : m_Slots)
        SlotData = {};

    Make_Dirty();
}

void CShopViewModel::Set_Category(SHOP_CATEGORY eCategory)
{
    if (eCategory == SHOP_CATEGORY::END)
        return;

    if (m_eCategory == eCategory)
        return;

    m_eCategory = eCategory;

    // 카테고리가 바뀌면 이전 카테고리에서 선택한 상품은 유지하지 않는다.
    m_iSelectedSlotIndex = -1;

    for (SHOP_SLOT_DATA& SlotData : m_Slots)
        SlotData = {};

    Make_Dirty();
}

void CShopViewModel::Move_Category(_int iDirection)
{
    const _int iCategoryCount = static_cast<_int>(SHOP_CATEGORY::END);
    _int iCategoryIndex = static_cast<_int>(m_eCategory);

    // 음수 방향도 정상적으로 순환하도록 카테고리 개수를 더한다.
    iCategoryIndex = (iCategoryIndex + iDirection + iCategoryCount) % iCategoryCount;

    Set_Category(static_cast<SHOP_CATEGORY>(iCategoryIndex));
}

void CShopViewModel::Clear_Slots()
{
    for (SHOP_SLOT_DATA& SlotData : m_Slots)
        SlotData = {};

    m_iSelectedSlotIndex = -1;
    Make_Dirty();
}

void CShopViewModel::Set_SlotData(_uint iSlotIndex, const SHOP_SLOT_DATA& SlotData)
{
    if (iSlotIndex >= SHOP_SLOT_COUNT)
        return;

    m_Slots[iSlotIndex] = SlotData;
    Make_Dirty();
}

void CShopViewModel::Select_Slot(_int iSlotIndex)
{
    if (iSlotIndex < 0 || iSlotIndex >= static_cast<_int>(SHOP_SLOT_COUNT))
        return;

    if (!m_Slots[iSlotIndex].bVisible)
        return;

    if (m_iSelectedSlotIndex == iSlotIndex)
        return;

    // 마우스로 직접 누른 슬롯을 실제 선택 상품으로 확정한다.
    m_iSelectedSlotIndex = iSlotIndex;
    Make_Dirty();
}

void CShopViewModel::Clear_SelectedSlot()
{
    if (m_iSelectedSlotIndex < 0)
        return;

    m_iSelectedSlotIndex = -1;
    Make_Dirty();
}

void CShopViewModel::Set_CurrentErgo(_uint iCurrentErgo)
{
    if (m_iCurrentErgo == iCurrentErgo)
        return;

    m_iCurrentErgo = iCurrentErgo;
    Make_Dirty();
}

const SHOP_SLOT_DATA& CShopViewModel::Get_SlotData(_uint iSlotIndex) const
{
    static const SHOP_SLOT_DATA EmptyData = {};

    if (iSlotIndex >= SHOP_SLOT_COUNT)
        return EmptyData;

    return m_Slots[iSlotIndex];
}

const SHOP_SLOT_DATA* CShopViewModel::Get_SelectedSlotData() const
{
    if (m_iSelectedSlotIndex < 0 || m_iSelectedSlotIndex >= static_cast<_int>(SHOP_SLOT_COUNT))
        return nullptr;

    const SHOP_SLOT_DATA& SlotData = m_Slots[m_iSelectedSlotIndex];

    if (!SlotData.bVisible)
        return nullptr;

    return &SlotData;
}

CShopViewModel* CShopViewModel::Create()
{
    return new CShopViewModel;
}

void CShopViewModel::Free()
{
    __super::Free();
}