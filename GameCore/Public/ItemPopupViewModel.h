#pragma once

#include "UIViewModelBase.h"
#include "Core_Enums.h"

NS_BEGIN(GameCore)

/* 아이템 획득 팝업 한 건에 필요한 표시 데이터 */
typedef struct tagItemPopupData final
{
    ITEM_TYPE eItemType = { ITEM_TYPE::END };
    _uint64 iItemID = { 0 };
    _uint iItemCount = { 1 };
    _wstring strDisplayText = {};
    string strIconTexturePath = {};
} ITEM_POPUP_DATA;

class CItemPopupViewModel final : public CUIViewModelBase
{
private:
    CItemPopupViewModel() = default;
    virtual ~CItemPopupViewModel() = default;

public:
    /* 현재 팝업에 표시할 아이템 데이터를 설정한다. */
    void Set_Item(const ITEM_POPUP_DATA& Data);

    /* 현재 표시 데이터를 초기화한다. */
    void Clear_Item();

public:
    const ITEM_POPUP_DATA& Get_ItemData() const { return m_ItemData; }
    _bool Is_Weapon() const { return m_ItemData.eItemType == ITEM_TYPE::WEAPON; }
    _bool Has_Item() const { return m_ItemData.eItemType != ITEM_TYPE::END; }

private:
    ITEM_POPUP_DATA m_ItemData = {};

public:
    static CItemPopupViewModel* Create();
    virtual void Free() override;
};

NS_END