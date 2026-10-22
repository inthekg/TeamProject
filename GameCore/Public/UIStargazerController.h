#pragma once
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CStargazerViewModel;

class CUIStargazerController final : public CUIControllerBase, public IListener
{
private:
    CUIStargazerController() = default;
    virtual ~CUIStargazerController() = default;

public:
    virtual HRESULT         Initialize(void* pArg) override;

public:
    virtual void            On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void            OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

protected:
    virtual HRESULT         Bind_UI() override;
    virtual void            Update_ViewModel(_float fTimeDelta) override;
    virtual void            Apply_ViewModel() override;

private:
    void                    Handle_PrimaryAction();
    void                    Request_TravelUI();
    void                    Close_Stargazer();

    void Begin_CloseAnimation(); // 별바라기 화면의 Close 애니메이션을 시작한다.
    void Finalize_Close(); // 애니메이션 종료 후 Screen과 HUD 억제를 정리한다.

private:
    CStargazerViewModel*    m_pViewModel = { nullptr };

private:
    CUIElement*             m_pServiceMenuGroup = { nullptr };
    CUIButton*              m_pServiceButton = { nullptr };
    CUIText*                m_pServiceLabel = { nullptr };

    CUIButton*              m_pPrimaryInputButton = { nullptr };
    CUIButton*              m_pCloseInputButton = { nullptr };

private:
    _bool m_bHUDLayerSuppressed = { false };

    _bool m_bCloseAnimationPending = { false };

public:
    static CUIStargazerController* Create();
    virtual void Free() override;
};

NS_END