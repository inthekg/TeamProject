#pragma once
#include "UIControllerBase.h"
#include "IListener.h"
#include "ShopViewModel.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CInventory;

class CUIShopController final : public CUIControllerBase, public IListener
{
private:
    CUIShopController() = default;
    virtual ~CUIShopController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;

public:
    virtual void    On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void    On_UIFocusChanged(const UI_FOCUS_EVENT& Event) override;
    virtual void    OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
    virtual void    OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void    Update_ViewModel(_float fTimeDelta) override;
    virtual void    Apply_ViewModel() override;

private:
    HRESULT         Bind_PlayerInventory();

    void            Open_Shop();
    void            Close_Shop();

    void            Refresh_Stock();
    void            Apply_Category();
    void            Apply_Slots();
    void            Apply_Detail();
    void            Apply_InputHints();
    void            Apply_ErgoProfile(); // 뷰모델의 현재 에르고를 프로필 컴포넌트에 반영한다.

    void            Request_PurchaseConfirmation();
    _wstring        Format_Number(_uint iValue) const; // 1000을 1,000 형태로 변환한다.

private:
    _int            Resolve_SlotIndex(const _wstring& strRuntimeElementTag) const;
    _wstring        Get_CategoryText(SHOP_CATEGORY eCategory) const;
    string          Resolve_ItemIconPath(_uint64 iItemID) const;
    vector<_uint64> Get_CategoryProductIDs(SHOP_CATEGORY eCategory) const;

    HRESULT         Set_ImageTexture(CUIImage* pImage, const _wstring& strImageTag, const string& strTexturePath, string& strAppliedTexturePath);
    
    void Begin_CloseAnimation(); // 실제 UI를 닫기 전에 Close 애니메이션을 시작한다.
    void Finalize_Close(); // Close 애니메이션이 끝난 뒤 Input Owner와 UI를 제거한다.

private:
    CShopViewModel* m_pViewModel = { nullptr };
    CInventory*     m_pInventory = { nullptr };

private:
    CUIButton*      m_pCategoryButtons[3] = {};

    CUIElement*     m_pSlotRoots[SHOP_SLOT_COUNT] = {};
    CUIButton*      m_pSlotButtons[SHOP_SLOT_COUNT] = {};
    CUIImage*       m_pSlotIcons[SHOP_SLOT_COUNT] = {};
    CUIText*        m_pSlotTexts[SHOP_SLOT_COUNT] = {};
    CUIImage*       m_pSoldOutImages[SHOP_SLOT_COUNT] = {};

    CUIElement*     m_pDetailPanel = { nullptr };
    CUIText*        m_pDetailTitle = { nullptr };
    CUIImage*       m_pDetailIcon = { nullptr };
    CUIText*        m_pDetailCategory = { nullptr };
    CUIText*        m_pDetailBodyText = { nullptr };

    CUIElement*     m_pPurchaseHint = { nullptr };
    CUIText*        m_pPurchaseActionText = { nullptr };
    CUIElement*     m_pCloseHint = { nullptr };
    CUIText*        m_pCloseActionText = { nullptr };

    CUIElement*     m_pErgoProfile = { nullptr };
    CUIText*        m_pErgoCountText = { nullptr };
    CUIText*        m_pErgoGetText = { nullptr };

private:
    string          m_strAppliedSlotIconPaths[SHOP_SLOT_COUNT];
    string          m_strAppliedDetailIconPath;

    _bool m_bCloseAnimationPending = false; // Close 애니메이션 완료를 기다리는 상태인지 나타낸다.

public:
    static CUIShopController* Create();
    virtual void Free() override;
};

NS_END
