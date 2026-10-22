#include "EquipmentViewModel.h"

void CEquipmentViewModel::Open_Equipment()
{
    if (m_bVisible)
        return;

    // 장비창을 다시 열 때는 항상 첫 번째 장비 카테고리부터 보여준다.
    m_bVisible = true;
    m_eCategory = EQUIPMENT_CATEGORY::EQUIPMENT;
    m_Detail = {};

    m_bCostumeSelectVisible = false;
    m_eCostumeCategory = COSTUME_CATEGORY::BODY;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

void CEquipmentViewModel::Close_Equipment()
{
    if (!m_bVisible)
        return;

    m_bVisible = false;
    m_bCostumeSelectVisible = false;
    m_iSelectedCostumeSlot = -1;
    m_Detail = {};

    Make_Dirty();
}

void CEquipmentViewModel::Set_Category(EQUIPMENT_CATEGORY eCategory)
{
    if (eCategory == EQUIPMENT_CATEGORY::END || m_eCategory == eCategory)
        return;

    m_eCategory = eCategory;
    
    // 페이지가 바뀌면 이전 페이지 슬롯의 상세 설명을 제거한다.
    m_Detail = {};

    Make_Dirty();
}

void CEquipmentViewModel::Move_Category(_int iDirection)
{
    const _int iCategoryCount = static_cast<_int>(EQUIPMENT_CATEGORY::END);
    _int iCategoryIndex = static_cast<_int>(m_eCategory);

    // 음수 방향도 정상적으로 순환할 수 있도록 카테고리 개수를 더한다.
    iCategoryIndex = (iCategoryIndex + iDirection + iCategoryCount) % iCategoryCount;

    Set_Category(static_cast<EQUIPMENT_CATEGORY>(iCategoryIndex));
}

void CEquipmentViewModel::Set_ItemDetail(const _wstring& strTitle, const _wstring& strCategory, const _wstring& strDescription, const string& strIconTexturePath)
{
    m_Detail = {};

    m_Detail.eMode = EQUIPMENT_DETAIL_MODE::ITEM;
    m_Detail.strTitle = strTitle;
    m_Detail.strCategory = strCategory;
    m_Detail.strDescription = strDescription;
    m_Detail.strIconTexturePath = strIconTexturePath;

    Make_Dirty();
}

void CEquipmentViewModel::Set_WeaponDetail(EQUIPMENT_DETAIL_MODE eMode, const _wstring& strTitle, const _wstring& strWeaponTitle, const _wstring& strDescription, const string& strPrimaryIconTexturePath, const string& strSecondaryIconTexturePath)
{
    if (eMode != EQUIPMENT_DETAIL_MODE::WEAPON_ASSEMBLED && eMode != EQUIPMENT_DETAIL_MODE::WEAPON_SINGLE)
        return;

    // 이전 슬롯의 상세 정보가 남지 않도록 데이터를 초기화한다.
    m_Detail = {};

    m_Detail.eMode = eMode;
    m_Detail.strTitle = strTitle;
    m_Detail.strWeaponTitle = strWeaponTitle;
    m_Detail.strDescription = strDescription;
    m_Detail.strIconTexturePath = strPrimaryIconTexturePath;
    m_Detail.strSubIconTexturePath = strSecondaryIconTexturePath;

    Make_Dirty();
}

void CEquipmentViewModel::Set_SecondaryWeaponOwned(_bool bOwned)
{
    if (m_bSecondaryWeaponOwned == bOwned)
        return;

    m_bSecondaryWeaponOwned = bOwned;

    // 무기를 아직 보유하지 않은 상태라면 2번 무기의 상세 정보도 제거한다.
    if (!m_bSecondaryWeaponOwned && m_Detail.eMode == EQUIPMENT_DETAIL_MODE::WEAPON_SINGLE)
        m_Detail = {};

    Make_Dirty();
}

void CEquipmentViewModel::Set_CurrentErgo(_uint iCurrentErgo)
{
    if (m_iCurrentErgo == iCurrentErgo)
        return;

    m_iCurrentErgo = iCurrentErgo;
    Make_Dirty();
}

void CEquipmentViewModel::Clear_Detail()
{
    if (m_Detail.eMode == EQUIPMENT_DETAIL_MODE::NONE)
        return;

    m_Detail = {};

    Make_Dirty();
}

void CEquipmentViewModel::Open_CostumeSelect(COSTUME_CATEGORY eCategory)
{
    if (eCategory == COSTUME_CATEGORY::END)
        return;

    m_bCostumeSelectVisible = true;
    m_eCostumeCategory = eCategory;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

void CEquipmentViewModel::Close_CostumeSelect()
{
    if (!m_bCostumeSelectVisible)
        return;

    m_bCostumeSelectVisible = false;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

void CEquipmentViewModel::Set_CostumeCategory(COSTUME_CATEGORY eCategory)
{
    if (eCategory == COSTUME_CATEGORY::END || m_eCostumeCategory == eCategory)
        return;

    m_eCostumeCategory = eCategory;
    m_iSelectedCostumeSlot = -1;

    Clear_CostumeSlots();
    Make_Dirty();
}

void CEquipmentViewModel::Move_CostumeCategory(_int iDirection)
{
    const _int iCategoryCount = static_cast<_int>(COSTUME_CATEGORY::END);
    _int iCategoryIndex = static_cast<_int>(m_eCostumeCategory);

    iCategoryIndex = (iCategoryIndex + iDirection + iCategoryCount) % iCategoryCount;

    Set_CostumeCategory(static_cast<COSTUME_CATEGORY>(iCategoryIndex));
}

void CEquipmentViewModel::Clear_CostumeSlots()
{
    for (COSTUME_SLOT_DATA& SlotData : m_CostumeSlots)
        SlotData = {};

    m_iSelectedCostumeSlot = -1;
    Make_Dirty();
}

void CEquipmentViewModel::Set_CostumeSlotData(_uint iIndex, const COSTUME_SLOT_DATA& Data)
{
    if (iIndex >= 3)
        return;

    m_CostumeSlots[iIndex] = Data;
    Make_Dirty();
}

void CEquipmentViewModel::Select_CostumeSlot(_int iIndex)
{
    if (iIndex < 0 || iIndex >= 3)
        return;

    if (!m_CostumeSlots[iIndex].bVisible)
        return;

    if (m_iSelectedCostumeSlot == iIndex)
        return;

    m_iSelectedCostumeSlot = iIndex;
    Make_Dirty();
}

const COSTUME_SLOT_DATA& CEquipmentViewModel::Get_CostumeSlotData(_uint iIndex) const
{
    static const COSTUME_SLOT_DATA EmptyData = {};

    if (iIndex >= 3)
        return EmptyData;

    return m_CostumeSlots[iIndex];
}

CEquipmentViewModel* CEquipmentViewModel::Create()
{
    return new CEquipmentViewModel;
}

void CEquipmentViewModel::Free()
{
    __super::Free();
}
