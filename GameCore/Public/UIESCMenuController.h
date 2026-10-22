#pragma once
#include "UIControllerBase.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CESCMenuViewModel;

class CUIESCMenuController : public CUIControllerBase
{
private:
    CUIESCMenuController() = default;
    virtual ~CUIESCMenuController() = default;

public:
    virtual HRESULT     Initialize(void* pArg) override;

public:
    virtual void        On_UICommand(const UI_COMMAND_EVENT& Event) override;
    virtual void        On_UIFocusChanged(const UI_FOCUS_EVENT& Event) override;

protected:
    virtual HRESULT     Bind_UI() override;
    virtual void        Update_ViewModel(_float fTimeDelta) override;
    virtual void        Apply_ViewModel() override;

private:
    void                Apply_MainMenuDescription(const string& strCommandKey);
    void                Apply_ItemSlotDescription(const _wstring& strRuntimeElementTag);
    void                Apply_Description();

private:
    void                Begin_CloseAnimation(); // ESC 메뉴 Close 애니메이션을 시작합니다.
    void                Finalize_Close(); // Close 애니메이션 종료 후 실제 UI를 닫습니다.

private:
    CESCMenuViewModel*  m_pViewModel = { nullptr };

private:
    CUIElement*         m_pMenuContent = { nullptr };
    CUIElement*         m_pTopInfoGroup = { nullptr };
    CUIElement*         m_pBottomInfoGroup = { nullptr }; 
    CUIElement*         m_pBottomPrimaryActionGroup = { nullptr };
    CUIElement*         m_pBottomSecondaryActionGroup = { nullptr };

    CUIButton*          m_pEquipmentButton = { nullptr };
    CUIButton*          m_pInventoryButton = { nullptr };
    CUIButton*          m_pCharacterInfoButton = { nullptr };
    CUIButton*          m_pSettingsButton = { nullptr };

    CUIText*            m_pTopInfoTitle = { nullptr };
    CUIText*            m_pTopInfoDescription = { nullptr };
    CUIText*            m_pBottomInfoTitle = { nullptr };
    CUIText*            m_pBottomPrimaryActionText = { nullptr };
    CUIText*            m_pBottomSecondaryActionText = { nullptr };

    _bool               m_bHUDLayerSuppressed = { false };

private:
    _bool m_bCloseAnimationPending = { false }; // Close 애니메이션 완료를 기다리는 상태

public:
    static CUIESCMenuController* Create();
    virtual void Free() override;
};

NS_END