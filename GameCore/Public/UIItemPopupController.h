#pragma once

#include "UIControllerBase.h"
#include "IListener.h"
#include "ItemPopupViewModel.h"
#include "Item.h"
#include <deque>

NS_BEGIN(Engine)

class CUIElement;
class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CUIItemPopupController final : public CUIControllerBase, public IListener
{
private:
    enum class ITEM_POPUP_STATE
    {
        HIDDEN,
        OPENING,
        HOLDING,
        CLOSING
    };

private:
    CUIItemPopupController() = default;
    virtual ~CUIItemPopupController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eEventType, Engine::Event_Desc* pEventDesc) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void Update_ViewModel(_float fTimeDelta) override;
    virtual void Apply_ViewModel() override;

private:
    /* GET_ITEM 이벤트 데이터를 팝업 표시 데이터로 변환한다. */
    _bool Build_PopupData(const GET_ITEM_EVENT_DESC& EventDesc, ITEM_POPUP_DATA& OutData) const;

    /* 아이템 ID에 대응하는 UI 아이콘 경로를 반환한다. */
    string Resolve_ItemIconPath(_uint64 iItemID) const;

    /* 대기열의 다음 아이템 팝업을 시작한다. */
    void Begin_NextPopup();

    /* 현재 팝업의 퇴장 애니메이션을 시작한다. */
    void Begin_Inactive();

    /* 현재 팝업을 닫고 다음 팝업을 받을 수 있는 상태로 만든다. */
    void Finish_CurrentPopup();

    /* 일반 아이템용 이미지 텍스처를 교체한다. */
    HRESULT Set_ItemImageTexture(const string& strTexturePath);

private:
    CItemPopupViewModel* m_pViewModel = { nullptr };

    CUIImage* m_pItemImage = { nullptr };
    CUIText* m_pItemText = { nullptr };
    CUIElement* m_pWeaponImageGroup = { nullptr };

    deque<ITEM_POPUP_DATA> m_PendingItems = {};

    ITEM_POPUP_STATE m_ePopupState = { ITEM_POPUP_STATE::HIDDEN };
    _float m_fHoldTime = { 0.f };
    string m_strAppliedTexturePath = {};

private:
    static constexpr _float POPUP_HOLD_DURATION = 3.f;

public:
    static CUIItemPopupController* Create();
    virtual void Free() override;
};

NS_END