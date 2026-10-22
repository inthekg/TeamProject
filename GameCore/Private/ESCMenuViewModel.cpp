#include "ESCMenuViewModel.h"

void CESCMenuViewModel::Set_Visible(_bool bVisible)
{
    if (m_bVisible == bVisible)
        return;

    m_bVisible = bVisible;

    Make_Dirty();
}

void CESCMenuViewModel::Set_Category(ESCAPE_MENU_CATEGORY eCategory)
{
    if (eCategory == ESCAPE_MENU_CATEGORY::END)
        return;

    if (m_eCategory == eCategory)
        return;

    m_eCategory = eCategory;

    Make_Dirty();
}

void CESCMenuViewModel::Set_MainDescription(const _wstring& strTitle, const _wstring& strDescription)
{
    m_Description = {};

    m_Description.eMode = ESCAPE_DESCRIPTION_MODE::MAIN_MENU;

    m_Description.strTitle = strTitle;
    m_Description.strDescription = strDescription;

    Make_Dirty();
}

void CESCMenuViewModel::Set_EmptySlotDescription()
{
    m_Description = {};

    m_Description.eMode = ESCAPE_DESCRIPTION_MODE::ITEM_SLOT;

    m_Description.strTitle = L"빈 슬롯";

    m_Description.bShowPrimaryAction = true;
    m_Description.strPrimaryAction = L"장착";

    Make_Dirty();
}

void CESCMenuViewModel::Set_ItemSlotDescription(const _wstring& strTitle, const _wstring& strDescription)
{
    m_Description = {};

    m_Description.eMode = ESCAPE_DESCRIPTION_MODE::ITEM_SLOT;

    m_Description.strTitle = strTitle;
    m_Description.strDescription = strDescription;

    m_Description.bShowPrimaryAction = true;
    m_Description.strPrimaryAction = L"변경";

    m_Description.bShowSecondaryAction = true;
    m_Description.strSecondaryAction = L"장착 해제";

    Make_Dirty();
}

void CESCMenuViewModel::Clear_Description()
{
    if (m_Description.eMode == ESCAPE_DESCRIPTION_MODE::NONE)
        return;

    m_Description = {};

    Make_Dirty();
}

CESCMenuViewModel* CESCMenuViewModel::Create()
{
    return new CESCMenuViewModel;
}

void CESCMenuViewModel::Free()
{
    __super::Free();
}