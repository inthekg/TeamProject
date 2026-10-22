#pragma once
#include "UIControllerBase.h"
#include "IListener.h"
#include "ConfirmationWindowViewModel.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CUIConfirmationWindowController final : public CUIControllerBase, public IListener
{
private:
    CUIConfirmationWindowController() = default;
    virtual ~CUIConfirmationWindowController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void Update_ViewModel(_float fTimeDelta) override;
    virtual void Apply_ViewModel() override;

private:
    void Apply_Contents();
    void Apply_InputHints();

    // 결과 이벤트를 전송하고 확인창을 닫는다.
    void Resolve_Confirmation(_bool bConfirmed);

    _wstring Format_Number(_uint iValue) const;

private:
    CConfirmationWindowViewModel* m_pViewModel = { nullptr };

private:
    CUIText* m_pConfirmTitle = { nullptr };
    CUIText* m_pGoodsText = { nullptr };
    CUIText* m_pConfirmText = { nullptr };

    CUIElement* m_pPriceRoot = { nullptr };
    CUIText* m_pPriceText = { nullptr };
    CUIText* m_pGoodsPrice = { nullptr };
    CUIText* m_pErgoCount = { nullptr };

    CUIElement* m_pConfirmKeyHint = { nullptr };
    CUIText* m_pConfirmActionText = { nullptr };

    CUIElement* m_pCancelKeyHint = { nullptr };
    CUIText* m_pCancelActionText = { nullptr };

public:
    static CUIConfirmationWindowController* Create();
    virtual void Free() override;
};

NS_END