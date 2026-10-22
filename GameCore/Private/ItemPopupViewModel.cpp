#include "ItemPopupViewModel.h"

void CItemPopupViewModel::Set_Item(const ITEM_POPUP_DATA& Data)
{
    m_ItemData = Data;
    Make_Dirty();
}

void CItemPopupViewModel::Clear_Item()
{
    m_ItemData = {};
    Make_Dirty();
}

CItemPopupViewModel* CItemPopupViewModel::Create()
{
    return new CItemPopupViewModel;
}

void CItemPopupViewModel::Free()
{
    __super::Free();
}