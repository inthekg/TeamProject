#pragma once
#include "UIControllerBase.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CUITitleController final : public CUIControllerBase
{
private:
    CUITitleController() = default;
    virtual ~CUITitleController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void On_UICommand(const UI_COMMAND_EVENT& Event) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void Update_ViewModel(_float fTimeDelta) override;
    virtual void Apply_ViewModel() override;

private:
    CUIElement* m_pTitleMenuRoot = { nullptr };

    CUIButton* m_pContinueButton = { nullptr };
    CUIButton* m_pExitButton = { nullptr };

    CUIText* m_pContinueText = { nullptr };
    CUIText* m_pExitText = { nullptr };

public:
    static CUITitleController* Create();
    virtual void Free() override;
};

NS_END