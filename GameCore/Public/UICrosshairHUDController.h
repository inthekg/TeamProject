#pragma once
#include "UIControllerBase.h"
#include "IListener.h"

NS_BEGIN(Engine)
class CUIElement;
NS_END

NS_BEGIN(GameCore)

class CCrosshairHUDViewModel;

class CUICrosshairHUDController final : public CUIControllerBase, public IListener
{
private:
    CUICrosshairHUDController() = default;
    virtual ~CUICrosshairHUDController() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void OnEvent(EVENT_TYPE_UI eType, Event_Desc* pData) override;

protected:
    virtual HRESULT Bind_UI() override;
    virtual void Update_ViewModel(_float fTimeDelta) override;
    virtual void Apply_ViewModel() override;

private:
    CCrosshairHUDViewModel* m_pViewModel = { nullptr };
    CUIElement* m_pCrosshair = { nullptr };
    _wstring m_strCrosshairBindingKey = L"Crosshair";

public:
    static CUICrosshairHUDController* Create();
    virtual void Free() override;
};

NS_END