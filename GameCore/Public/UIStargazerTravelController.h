#pragma once
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(Engine)

class CUIElement;
class CUIButton;
class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CStargazerTravelViewModel;

class CUIStargazerTravelController final : public CUIControllerBase, public IListener
{
private:
    CUIStargazerTravelController() = default;
    virtual ~CUIStargazerTravelController() = default;

public:
    virtual HRESULT                 Initialize(void* pArg) override;
    virtual void                    OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
    virtual void                    OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
    virtual HRESULT                 Bind_UI() override;
    virtual void                    Update_ViewModel(_float fTimeDelta) override;
    virtual void                    Apply_ViewModel() override;
    virtual void                    On_UICommand(const UI_COMMAND_EVENT& Event) override;

private:
    void                            Execute_SelectedTravel();
    void                            Execute_HotelTravel();
    void                            Execute_CurrentSelection();
    void                            Close_Travel();

private:
    CStargazerTravelViewModel*      m_pViewModel = { nullptr };

private:
    CUIButton*                      m_pChapterButtons[3] = {};

    CUIImage*                       m_pLocationPreviewImage = { nullptr };

    CUIElement*                     m_pLocationOptions[3] = {};
    CUIButton*                      m_pLocationButtons[3] = {};
    CUIText*                        m_pLocationLabels[3] = {};
    CUIImage*                       m_pCurrentLocationIcons[3] = {};

    CUIText*                        m_pPrimaryActionText = { nullptr };

    CUIElement*                     m_pHotelShortcutGroup = { nullptr };
    CUIButton*                      m_pHotelShortcutButton = { nullptr };

private:
    string                          m_strAppliedPreviewTexturePath;

public:
    static CUIStargazerTravelController* Create();
    virtual void Free() override;
};

NS_END