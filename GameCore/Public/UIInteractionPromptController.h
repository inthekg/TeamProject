#pragma once

#include "UIControllerBase.h"
#include "IListener.h"
#include "Core_Enums.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CInteractionPromptViewModel;

class CUIInteractionPromptController final : public CUIControllerBase, public IListener
{
private:
    CUIInteractionPromptController() = default;
    virtual ~CUIInteractionPromptController() = default;

public:
    virtual HRESULT                 Initialize(void* pArg) override;

protected:
    virtual HRESULT                 Bind_UI() override;

    virtual void                    Update_ViewModel(_float fTimeDelta) override;
    virtual void                    Apply_ViewModel() override;

public:
    virtual void                    OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
    virtual void                    OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
private:
    // OBJECT_TYPE을 실제 출력 문구로 변환한다.
    _wstring                        Resolve_PromptText(OBJECT_TYPE eObjectType) const;

    // 텍스트 실제 길이에 맞춰 안내창 전체 폭을 조절한다.
    void                            Resize_Prompt(const _wstring& strPromptText);

    void Begin_InactiveAnimation(); // 안내창을 숨기기 전에 Inactive 애니메이션을 시작한다.
    void Finalize_Inactive(); // Inactive 종료 후 실제 Visibility를 끈다.

private:
    CInteractionPromptViewModel*    m_pViewModel = { nullptr };

private:
    CUIElement*                     m_pPromptRoot = { nullptr };

    CUIElement*                     m_pBGBlackOuter = { nullptr };
    CUIElement*                     m_pBGMain = { nullptr };
    CUIElement*                     m_pBGBlackInner = { nullptr };
    CUIElement*                     m_pBGLight = { nullptr };
    CUIElement*                     m_pBGHighlight = { nullptr };
    CUIElement*                     m_pDirt = { nullptr };

    CUIElement*                     m_pLineTop = { nullptr };
    CUIElement*                     m_pLineBottom = { nullptr };

    CUIElement*                     m_pKeyGroup = { nullptr };
    CUIText*                        m_pPromptText = { nullptr };
    
    _float                          m_fRuntimeScale = { 1.f };

    _bool m_bInactiveAnimationPending = { false }; // Inactive 애니메이션 종료를 기다리는 상태인지 나타낸다.

public:
    static CUIInteractionPromptController* Create();
    virtual void Free() override;
};

NS_END