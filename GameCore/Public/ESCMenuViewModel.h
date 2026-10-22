#pragma once
#include "UIViewModelBase.h"

NS_BEGIN(GameCore)

enum class ESCAPE_MENU_CATEGORY
{
    EQUIPMENT,
    INVENTORY,
    CHARACTER_INFO,
    SETTINGS,
    END
};

enum class ESCAPE_DESCRIPTION_MODE
{
    NONE,
    MAIN_MENU,
    ITEM_SLOT,
    END
};

struct ESCAPE_DESCRIPTION_DATA
{
    ESCAPE_DESCRIPTION_MODE eMode = { ESCAPE_DESCRIPTION_MODE::NONE };

    _wstring strTitle;
    _wstring strDescription;

    // 슬롯에서 사용하는 첫 번째 행동
    _bool bShowPrimaryAction = { false };
    _wstring strPrimaryAction;

    // 슬롯에서 사용하는 두 번째 행동
    _bool bShowSecondaryAction = { false };
    _wstring strSecondaryAction;
};

class CESCMenuViewModel final : public CUIViewModelBase
{
private:
    CESCMenuViewModel() = default;
    virtual ~CESCMenuViewModel() = default;

public:
    void                            Set_Visible(_bool bVisible);
    void                            Set_Category(ESCAPE_MENU_CATEGORY eCategory);
    void                            Set_MainDescription(const _wstring& strTitle, const _wstring& strDescription);
    void                            Set_EmptySlotDescription();
    void                            Set_ItemSlotDescription(const _wstring& strTitle, const _wstring& strDescription);

public:
    ESCAPE_MENU_CATEGORY            Get_Category() const { return m_eCategory; }
    const ESCAPE_DESCRIPTION_DATA&  Get_Description() const { return m_Description; }

public:
    _bool                           Is_Visible() const { return m_bVisible; }

public:
    void                            Clear_Description();

private:

    // Element의 단순 Visible이 아니라 EscapeMenu가 Open 상태여야 하는지를 의미한다.
    _bool                           m_bVisible = { false };

    ESCAPE_MENU_CATEGORY            m_eCategory = { ESCAPE_MENU_CATEGORY::EQUIPMENT };
    ESCAPE_DESCRIPTION_DATA         m_Description = {};

public:
    static CESCMenuViewModel* Create();
    virtual void Free() override;
};

NS_END