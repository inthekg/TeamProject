#pragma once
#include "UIControllerBase.h"

NS_BEGIN(Engine)

class CUIImage;
class CUIText;

NS_END

NS_BEGIN(GameCore)

class CLoadingViewModel;

class CUILoadingController final : public CUIControllerBase
{
private:
    CUILoadingController() = default;
    virtual ~CUILoadingController() = default;

public:
    void                    Begin_LoadingSession();
    void                    Set_LoadingProgress(_float fProgress);

public:
    virtual HRESULT         Initialize(void* pArg) override;

protected:
    virtual HRESULT         Bind_UI() override;
    virtual void            Update_ViewModel(_float fTimeDelta) override;
    virtual void            Apply_ViewModel() override;

private:
    CLoadingViewModel*      m_pViewModel = { nullptr };

private:
    CUIImage*               m_pLoadingPhoto = { nullptr };
    CUIImage*               m_pLoadingGear = { nullptr };
    CUIImage*               m_pLoadingNose = { nullptr };

    CUIText*                m_pLoadingTipText = { nullptr };
    CUIText*                m_pLoadingStateText = { nullptr };
    CUIText*                m_pLoadingDotsText = { nullptr };
    CUIText*                m_pLoadingPercentText = { nullptr };

private:
    _float2                 m_vLoadingNoseMaxSize = {};
    string                  m_strAppliedPhotoTexturePath;

    _uint                   m_iLastTipIndex = { UINT_MAX }; // 연속된 로딩에서 같은 팁과 사진이 반복되는 것을 방지한다.
    _uint                   m_iLastPathIndex = { UINT_MAX }; // 연속된 로딩에서 같은 팁과 사진이 반복되는 것을 방지한다.

public:
    static CUILoadingController* Create();
    virtual void Free() override;
};

NS_END